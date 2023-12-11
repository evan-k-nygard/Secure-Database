#ifndef __DBMANAGER_H
#define __DBMANAGER_H

#include "sqlite/sqlite3.h"
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <set>

typedef std::vector< std::multiset<std::string> > DBTable;
typedef std::vector<std::string> ArgumentList;

class DBManager {
    private:
        sqlite3* db;
        const char* key; // encryption key
    public:
        DBManager(const char* dbname, const char* k);
        ~DBManager();

        void store(std::string tbl, std::vector<std::string> args);

        // do a "raw" SQL query. Must be a prepared query to ensure security
        DBTable prepared_query(std::string q, const ArgumentList& args);

        // wrapper functions for records management
        void create_record(const std::string& n, const std::string& v);
        std::string retrieve_record(const std::string& n);
        void edit_record(const std::string& n, const std::string& v);

        // used to change the user password. Stringent security is ensured.
        // As part of this process, all user records are decrypted,
        // re-encrypted, and updated using the new password key.
        void change_user_password(const std::string& old, const std::string& updated);
};

#endif