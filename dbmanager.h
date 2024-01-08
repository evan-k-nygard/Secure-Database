#ifndef __DBMANAGER_H
#define __DBMANAGER_H

#include "sqlite/sqlite3.h"
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <set>
#include "cryptopp890/secblock.h"

typedef std::vector< std::multiset<std::string> > DBTable;
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

        DBTable* prepared_query(std::string q, const ArgumentList& args);
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

    public:
        AuthenticatedDBUser(const std::string& username_plain, const std::string& password_plain);
        ~AuthenticatedDBUser();

        void create_record(const std::string& n, const std::string& v);
        std::string retrieve_record(const std::string& n);
        void edit_record(const std::string& n, const std::string& v);

        void share_record(const std::string& n, const std::string& user);

        void change_user_password(const std::string& old, const std::string& updated);
};



#endif