#include "sqlite/sqlite3.h"
#include <cstdlib>
#include <string>
#include <vector>
#include <stdexcept>
#include <map>
#include <set>
#include "dbmanager.h"

DBManager::DBManager(const char* dbname, const char* k) {
    // open up a new SQLite3 database by initiating sqlite3* db
    // also initiate const char* key with k
    int r = sqlite3_open(dbname, &db);
    if(r != 0) { // couldn't open the database properly
        sqlite3_close(db);
    }
    key = k;
}

DBManager::~DBManager() {
    // close the database
    sqlite3_close(db);
}

DBTable DBManager::prepared_query(std::string q, const ArgumentList& args) {
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
    DBTable result;

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
                row.insert(std::string((const char*) sqlite3_column_text(pstmt, i)));
            }
            // now, add this new row into the DBTable
            result.push_back(row);

        } else if(s == SQLITE_ERROR) {
            sqlite3_finalize(pstmt);
            throw std::runtime_error("error on parsing statement");
        }
    }

    // we've finished the query; clean up and return
    sqlite3_finalize(pstmt);
    return result;

}

void DBManager::store(std::string tbl, std::vector<std::string> args) {
    /*
    * Stores values contained in "args" within the table specified by "tbl"
    * WARNING: this is an old function, vulnerable to SQL Injection attacks. 
    * Will require major edits, or possibly future deletion. For now, do not
    * use; replace with prepared_query instead
    * 
    * @arguments
    * ~ tbl: a string containing the table name
    * ~ args: a list of values to insert into the table
    * @expects tbl and args contain sanitized input; tbl refers to an existing
        table in the database; args.size() == number of columns in the table
        that expect user input
    * @returns void; the associated table now contains values specified in args
    */
    std::string query("INSERT INTO ");
    query += tbl;
    query += " VALUES (";
    // add the values to the query
    for(size_t i = 0; i < args.size(); i++) {
        query += args[i]; // TODO: encrypt args[i]
        if(i < args.size() - 1) {
            query += ", ";
        }
    }
    query += ")";
    sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
}