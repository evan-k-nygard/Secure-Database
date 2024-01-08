#include "sqlite/sqlite3.h"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <map>
#include <set>
#include "dbmanager.h"
#include "cryptowrapper.h"


DB::DB(const char* dbname) {
    // open up a new SQLite3 database by initiating sqlite3* db
    int r = sqlite3_open(dbname, &db);
    if(r != 0) { // couldn't open the database properly
        sqlite3_close(db);
    }
}

DB::~DB() {
    // close the database
    sqlite3_close(db);
}

DBTable* DB::prepared_query(std::string q, const ArgumentList& args) {
    /*
    * Execute a prepared query with respect to the currently active database.
    * @arguments
    * ~ q: contains a prepared query string
    * ~ args: contains a list of arguments, which will be binded to the
        prepared values in the query q
    * @expects args.size() == number of '?'s in q
    * @returns DBTable containing results of query on success, throws
        std::runtime_error on failure
    */
    sqlite3_stmt* pstmt;
    if(sqlite3_prepare_v2(db, q.c_str(), q.size(), &pstmt, NULL) != SQLITE_OK) {
        throw std::runtime_error("unable to prepare statement");
    }
    // bind all arguments in the ArgumentList to the query
    for(size_t i = 0; i < args.size(); i++) {
        if(sqlite3_bind_text(pstmt, i+1, args[i].c_str(), -1, SQLITE_STATIC) != SQLITE_OK) {
            throw std::runtime_error("unable to bind argument");
        }
    }

    // now, add results into the DBTable. 
    DBTable* result = new DBTable;

    int s;
    while((s = sqlite3_step(pstmt)) != SQLITE_DONE) {
        if(s == SQLITE_ROW) {
            // we've hit a new row, create one to add to the DBTable
            std::multiset<std::string> row;
            int colNum = sqlite3_column_count(pstmt);

            // go through every column in the current row, and insert it into
            // the variable "row"
            for(int i = 0; i < colNum; i++) {
                // insert the current column value into our row
                // note: a cast is necessary because the std::string
                // constructor can't interpret a const unsigned char*
                if(sqlite3_column_type(pstmt, i) != SQLITE_NULL) {
                    const unsigned char* uColText = sqlite3_column_text(pstmt, i);
                    const char* colText = reinterpret_cast<const char*>(uColText);
                    row.insert(std::string(colText));
                } else {
                    row.insert(std::string(""));
                }
            }
            // now, add this new row into the DBTable
            result->push_back(row);

        } else if(s == SQLITE_ERROR) {
            sqlite3_finalize(pstmt);
            throw std::runtime_error("error on parsing statement");
        }
    }

    // we've finished the query; clean up and return
    sqlite3_finalize(pstmt);
    return result;

}


AuthenticatedDBUser::AuthenticatedDBUser(const std::string& username_plain, const std::string& password_plain) : DB::DB("records.db") {
    /*
    * Securely log a user into the database, and empower them to perform all record-keeping operations
    * Calculates a hash of the username, and a salted hash of the password, and checks to see
    * if exactly one of these records is in the database, the class finishes the initialization
    * process such that all accesses can be appropriately performed.
    *
    * @arguments 
    * ~ username_plain: The *plaintext* username for the intended user
    * ~ password_plain: The *plaintext* password for the intended user
    * @results Successful initialization on valid authentication; exception on 
    * invalid authentication
    */

    // get hashes
    uname_hash = crypto::hash(username_plain);
    std::string keygenerator = crypto::hash(username_plain + password_plain);
    salted_pwd_hash = crypto::hash(keygenerator);
    // authenticate the user
    // NOTE: this is a first draft. TODO review the security of this authentication method
    DBTable* check = prepared_query("SELECT username FROM Uses WHERE username=? AND password=?",
                                    ArgumentList({uname_hash, salted_pwd_hash}));
    
    // make sure that EXACTLY one record matches these critiera
    if(check->size() != 1) {
        delete check;
        throw std::runtime_error("Could not authenticate");
    }
    delete check;

    // finish valid initialization
    // The master_key is used to retrieve and decrypt individual record keys, so
    // that records can be read.
    lockdown = false;
    master_key = crypto::master_keygen(uname_hash, keygenerator);
}

AuthenticatedDBUser::~AuthenticatedDBUser() {
    /*
    * Zero out all sensitive variables
    */
    uname_hash = "";
    salted_pwd_hash = "";
    lockdown = true;
    // master_key will zero itself out
}

void AuthenticatedDBUser::assert_safe() {
    /*
    * Make sure that we have not entered a locked-down status for any reason.
    * If we have, throw an exception immediately.
    */
    if(lockdown) {
        throw std::runtime_error("Database access invalid");
    }
}