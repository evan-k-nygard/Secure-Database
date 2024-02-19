#ifndef __DBMANAGER_H
#define __DBMANAGER_H

#include "sqlite/sqlite3.h"
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <set>
#include "cryptopp890/secblock.h"

typedef std::vector< std::vector<std::string> > DBTable;
typedef std::vector<std::string> ArgumentList;

/*
* DB: A bare-bones C++ wrapper over the SQLite C library
* Provides the under-the-hood database access functionality for the
* AuthenticatedDBUser class to perform its operations securely.
* Currently, the only operation provided is a generic prepared query
* operation. More operations will be added in the future as needed.
*/
class DB {
    private:
        sqlite3* db;
    public:
        DB(const char* dbname);
        ~DB();

        DBTable prepared_query(std::string q, const ArgumentList& args);
};

/*
* AuthenticatedDBUser: Provides secure record access, performing all necessary
* security and encryption/decryption operations under the hood to properly
* access records.
*/
class AuthenticatedDBUser : private DB {
    private:
        std::string uname_hash; 
        std::string salted_pwd_hash;
        CryptoPP::SecByteBlock master_key;
        bool lockdown; // tested by assert_safe, set to true if we enter an insecure state
        // Upcoming design decision: do we keep lockdown, or simply throw an exception
        // if there's a security problem?

        void assert_safe();

        CryptoPP::SecByteBlock get_record_key(const std::string& muser, const std::string& hashed_record_name);
        void assert_existence(const std::string& muser, const std::string& hashed_record_name);
    public:
        AuthenticatedDBUser(const std::string& username_plain, const std::string& password_plain);
        ~AuthenticatedDBUser();

        void create_record(const std::string& n, const std::string& v);
        std::string retrieve_record(const std::string& n);
        void edit_record(const std::string& n, const std::string& v);
        void delete_record(const std::string& n);

        void share_record(const std::string& n, const std::string& user);

        void change_user_password(const std::string& old, const std::string& updated);
};

class LockedDB : private DB {
    private:
        size_t transaction_number;
        std::string savepoint_start;
    public:
        LockedDB(const char* dbname);
        ~LockedDB();

        DBTable prepared_query(std::string q, const ArgumentList& args);
        void rollback(size_t n);
};

#endif