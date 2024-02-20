#include "sqlite/sqlite3.h"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <map>
#include <set>
#include "dbmanager.h"
#include "cryptowrapper.h"
#include "cryptopp890/osrng.h"

DB::DB() {
    db = NULL;
}

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

sqlite3* DB::get_db() {
    return db;
}

DBTable DB::prepared_query(std::string q, const ArgumentList& args) {
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
    int e = sqlite3_prepare_v2(db, q.c_str(), q.size(), &pstmt, NULL);
    if(e != SQLITE_OK) {
        std::cerr << "Internal error: " << sqlite3_errmsg(db) << '\n';
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
            std::vector<std::string> row;
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
                    row.push_back(std::string(colText));
                } else {
                    row.push_back(std::string(""));
                }
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

AuthenticatedDBUser::AuthenticatedDBUser() : DB::DB() {
    uname_hash = "";
    salted_pwd_hash = "";
    lockdown = true;
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
    DBTable check = prepared_query("SELECT username FROM Users WHERE username=? AND password=?",
                                    ArgumentList({uname_hash, salted_pwd_hash}));
    
    // make sure that EXACTLY one record matches these critiera
    if(check.size() != 1) {
        throw std::runtime_error("Could not authenticate");
    }

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

void AuthenticatedDBUser::create_record(const std::string& n, const std::string& v) {
    
    // generate secure new key
    CryptoPP::SecByteBlock newKey(CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::AutoSeededRandomPool generator;
    generator.GenerateBlock(newKey, newKey.size());
    // store key in Keys database table
    std::string muser = crypto::hash(uname_hash);
    std::string record_name = crypto::hash(n); // do we salt the record name? In case multiple users have records of same name
    DBTable check = prepared_query("SELECT * FROM Keys WHERE user=? AND record_name=?",
                                    ArgumentList({muser, record_name}));
    if(check.size() > 0) {
        // do we wanna throw an exception?
        throw std::runtime_error("Could not create record: record already exists");
    }

    std::string key_encrypt = crypto::encrypt(crypto::_impl_details::bytes_to_string(newKey), master_key);
    prepared_query("INSERT INTO Keys (user, record_name, key) VALUES (?, ?, ?)", 
                   ArgumentList({muser, record_name, key_encrypt}));
    // encrypt owner, n, and v with new key
    std::string hashedOwner = muser;
    std::string hashedN = crypto::hash(n);
    std::string encryptedV = crypto::encrypt(v, newKey);
    // add encrypted values to Records database table
    prepared_query("INSERT INTO Records (owner, name, record) VALUES (?, ?, ?)",
                    ArgumentList({hashedOwner, hashedN, encryptedV}));
    
}

CryptoPP::SecByteBlock AuthenticatedDBUser::get_record_key(const std::string& muser, const std::string& hashed_record_name) {
    
    DBTable key_info = prepared_query("SELECT * FROM Keys WHERE user=? AND record_name=?",
                                    ArgumentList({muser, hashed_record_name}));
    if(key_info.size() != 1) {
        throw std::runtime_error("could not retrieve record");
    }
    std::string encrypted_key = key_info[0][2];
    
    // decrypt the record key using the master key
    std::string record_key_string = crypto::decrypt(encrypted_key, master_key);
    CryptoPP::SecByteBlock record_key = crypto::_impl_details::string_to_bytes(record_key_string);
    return record_key;
}

void AuthenticatedDBUser::assert_existence(const std::string& muser, const std::string& hashed_record_name) {
    DBTable entry = prepared_query("SELECT owner, name, record FROM Records WHERE owner=? AND name=?",
                                    ArgumentList({muser, hashed_record_name}));
    if(entry.size() != 1) {
        throw std::runtime_error("could not retrieve record");
    }
}

std::string AuthenticatedDBUser::retrieve_record(const std::string& n) {
    // retrieve record key from Keys table (if one exists)
    std::string muser = crypto::hash(uname_hash);
    std::string record_name = crypto::hash(n);
    
    CryptoPP::SecByteBlock record_key = get_record_key(muser, record_name);

    // retrieve the record
    DBTable entry = prepared_query("SELECT owner, name, record FROM Records WHERE owner=? AND name=?",
                                    ArgumentList({muser, record_name}));
    if(entry.size() != 1) {
        throw std::runtime_error("could not retrieve record");
    }
    std::string encrypted_record = entry[0][2];

    // decrypt the record using the record key, and return
    std::string record = crypto::decrypt(encrypted_record, record_key);
    return record;
}

void AuthenticatedDBUser::edit_record(const std::string& n, const std::string& v) {
    std::string muser = crypto::hash(uname_hash);
    std::string record_name = crypto::hash(n);
    CryptoPP::SecByteBlock record_key = get_record_key(muser, record_name);

    assert_existence(muser, record_name);
    std::string new_encrypted_text = crypto::encrypt(v, record_key);
    prepared_query("UPDATE Records SET record=? WHERE owner=? AND name=?",
                   ArgumentList({new_encrypted_text, muser, record_name}));
}

void AuthenticatedDBUser::delete_record(const std::string& n) {
    std::string muser = crypto::hash(uname_hash);
    std::string record_name = crypto::hash(n);

    assert_existence(muser, record_name);

    DB::prepared_query("DELETE FROM Records WHERE owner=? AND name=?",
                        ArgumentList({muser, record_name}));
    DB::prepared_query("DELETE FROM Keys WHERE user=? AND record_name=?",
                        ArgumentList({muser, record_name}));
}

DBTable AuthenticatedDBUser::debug_prepared_query(std::string q, const ArgumentList& args) {
    return prepared_query(q, args);
}


/* NOTE: All LockedDB functionality is purely experimental at this time */

LockedDB::LockedDB(const char* dbname) : DB::DB(dbname) {
    DB::prepared_query("BEGIN EXCLUSIVE TRANSACTION", ArgumentList({}));
    transaction_number = 1;
    savepoint_start = crypto::random_token();
}

LockedDB::~LockedDB() {
    std::cout << "destruting locked db\n";
    DB::prepared_query("COMMIT TRANSACTION", ArgumentList({}));
}

DBTable LockedDB::prepared_query(std::string q, const ArgumentList& args) {
    DBTable result = DB::prepared_query(q, args);
    std::ostringstream transaction_point;
    transaction_point << "SAVEPOINT " << savepoint_start << '_' << transaction_number;
    std::cout << "about to set savepoint\n";
    DB::prepared_query(transaction_point.str(), ArgumentList({}));
    transaction_number++;
    return result;
}

void LockedDB::rollback(size_t n) {
    std::ostringstream transaction_point;
    transaction_point << "ROLLBACK TRANSACTION TO SAVEPOINT " << savepoint_start << '_' << n;

    std::cout << "about to rollback to savepoint\n";
    DB::prepared_query(transaction_point.str(), ArgumentList({}));
}