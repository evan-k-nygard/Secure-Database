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

DB::DB(DB&& database) {
    // move database's db pointer into current object
    sqlite3_close(db);
    db = database.db;
    database.db = NULL;
}

DB& DB::operator=(DB&& database) {
    // move database's db pointer into current object
    sqlite3_close(db);
    db = database.db;
    database.db = NULL;
    return *this;
}

DB::~DB() {
    // close the database
    sqlite3_close(db);
}

sqlite3* DB::get_db() {
    // for debugging only
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


void AuthenticatedDBUser::authenticate(const std::string& username_plain, const std::string& password_plain) {
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

AuthenticatedDBUser::AuthenticatedDBUser() : DB::DB() {
    /*
    * Initialize an AuthenticatedDBUser. At this point, no operations will
    * work; the object must be fully populated using the move constructor
    * and assignment operator
    */
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

    authenticate(username_plain, password_plain);
}

AuthenticatedDBUser::AuthenticatedDBUser(const std::string& username_plain, const std::string& password_plain, const std::string& dbname) : DB::DB(dbname.c_str()) {
    /*
    * Same as above, but logs a user into a different database
    * THIS FUNCTION IS FOR TEST PURPOSES ONLY
    */

    authenticate(username_plain, password_plain);
}

AuthenticatedDBUser::AuthenticatedDBUser(AuthenticatedDBUser&& database) : DB::DB(std::move(database)) {
    uname_hash = database.uname_hash;
    salted_pwd_hash = database.salted_pwd_hash;
    master_key = database.master_key;
    lockdown = database.lockdown;

    database.uname_hash = "";
    database.salted_pwd_hash = "";
    database.lockdown = true;
}

AuthenticatedDBUser& AuthenticatedDBUser::operator=(AuthenticatedDBUser&& database) {
    DB::operator=(std::move(database));
    uname_hash = database.uname_hash;
    salted_pwd_hash = database.salted_pwd_hash;
    master_key = database.master_key;
    lockdown = database.lockdown;

    database.uname_hash = "";
    database.salted_pwd_hash = "";
    database.lockdown = true;

    return *this;
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
    /*
    * Create a new record with the current user as the owner. The new record
    * will have a name n and will contain the string v.
    */
    // generate secure new key
    CryptoPP::SecByteBlock newKey(CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::AutoSeededRandomPool generator;
    generator.GenerateBlock(newKey, newKey.size());

    // store key in Keys database table
    std::string muser = crypto::hash(uname_hash);
    std::string record_name = crypto::hash(n); // TODO ensure no conflicts if multiple users have same record name
    DBTable check = prepared_query("SELECT * FROM Keys WHERE user=? AND record_name=?",
                                    ArgumentList({muser, record_name}));
    
    // confirm that no records under user's name currently exist
    if(check.size() > 0) {
        throw std::runtime_error("Could not create record: record already exists");
    }

    // encrypt the record key newKey with the user's master_key and place
    // it in the Keys table
    std::string key_encrypt = crypto::encrypt(crypto::_impl_details::bytes_to_string(newKey), master_key);
    prepared_query("INSERT INTO Keys (user, record_name, key) VALUES (?, ?, ?)", 
                   ArgumentList({muser, record_name, key_encrypt}));
    
    // encrypt owner, n, and v with newKey before adding to the Records table
    std::string hashedOwner = muser;
    std::string hashedN = crypto::hash(n);
    std::string encryptedV = crypto::encrypt(v, newKey);

    // add encrypted values to Records database table
    prepared_query("INSERT INTO Records (owner, name, record) VALUES (?, ?, ?)",
                    ArgumentList({hashedOwner, hashedN, encryptedV}));
    
}

CryptoPP::SecByteBlock AuthenticatedDBUser::get_record_key(const std::string& muser, const std::string& hashed_record_name) {
    /*
    * Retrieves the record key for hashed_record_name from the Keys database,
    * decrypts it, and returns it ready for use
    */

    // retrieve the encrypted key
    DBTable key_info = prepared_query("SELECT * FROM Keys WHERE user=? AND record_name=?",
                                    ArgumentList({muser, hashed_record_name}));
    
    // ensure that only one such key exists
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

bool AuthenticatedDBUser::record_exists(const std::string& n) {
    try {
        assert_existence(crypto::hash(uname_hash), crypto::hash(n));
        return true;
    } catch(...) {
        return false;
    }
}

std::string AuthenticatedDBUser::retrieve_record(const std::string& n) {
    /*
    * Access and decrypt the record n from the Records database, returning
    * it as a string
    */

    // retrieve record key from Keys table (if one exists)
    std::string muser = crypto::hash(uname_hash);
    std::string record_name = crypto::hash(n);
    
    CryptoPP::SecByteBlock record_key = get_record_key(muser, record_name);

    // retrieve the record
    DBTable entry = prepared_query("SELECT owner, name, record FROM Records WHERE owner=? AND name=?",
                                    ArgumentList({muser, record_name}));
    
    // ensure that only one such record exists
    if(entry.size() != 1) {
        throw std::runtime_error("could not retrieve record");
    }
    std::string encrypted_record = entry[0][2];

    // decrypt the record using the record key, and return
    std::string record = crypto::decrypt(encrypted_record, record_key);
    return record;
}

void AuthenticatedDBUser::edit_record(const std::string& n, const std::string& v) {
    /*
    * Edit an already existing record n, replacing its existing data with v
    */
    // retrieve the record key
    std::string muser = crypto::hash(uname_hash);
    std::string record_name = crypto::hash(n);
    CryptoPP::SecByteBlock record_key = get_record_key(muser, record_name);

    // ensure that the record actually exists
    assert_existence(muser, record_name);

    // encrypt the text v and update the record
    std::string new_encrypted_text = crypto::encrypt(v, record_key);
    prepared_query("UPDATE Records SET record=? WHERE owner=? AND name=?",
                   ArgumentList({new_encrypted_text, muser, record_name}));
}

void AuthenticatedDBUser::delete_record(const std::string& n) {
    /*
    * Delete the record n
    * Requires that record n exists and that the current user is n's owner
    */
    std::string muser = crypto::hash(uname_hash);
    std::string record_name = crypto::hash(n);

    assert_existence(muser, record_name);

    // Delete both the record itself and the owner's record key
    DB::prepared_query("DELETE FROM Records WHERE owner=? AND name=?",
                        ArgumentList({muser, record_name}));
    DB::prepared_query("DELETE FROM Keys WHERE user=? AND record_name=?",
                        ArgumentList({muser, record_name}));
}

DBTable AuthenticatedDBUser::debug_prepared_query(std::string q, const ArgumentList& args) {
    // For debugging only - call the parent's prepared_query from the child class
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