#include "sqlite/sqlite3.h"
#include <cstdlib>
#include <string>
#include <vector>
#include <stdexcept>
#include <map>
#include <set>
#include "dbmanager.h"

DBManager::DBManager(const char* dbname, const char* k) {
    int r = sqlite3_open(dbname, &db);
    if(r != 0) {
        sqlite3_close(db);
    }
    key = k;
}

DBManager::~DBManager() {
    sqlite3_close(db);
}

void DBManager::store(std::string tbl, std::vector<std::string> args) {
    std::string query("INSERT INTO" );
    query += tbl;
    query += " VALUES (";
    for(size_t i = 0; i < args.size(); i++) {
        query += args[i]; // NEED TO ENCRYPT THIS
        if(i < args.size() - 1) {
            query += ", ";
        }
    }
    query += ")";
    sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
}

DBTable DBManager::prepared_query(std::string& q, const std::vector<std::string>& args) {
    sqlite3_stmt* pstmt;
    if(sqlite3_prepare_v2(db, q.c_str(), q.size(), &pstmt, NULL) != SQLITE_OK) {
        throw std::exception("unable to prepare statement");
    }
    for(size_t i = 0; i < args.size(); i++) {
        if(sqlite3_bind(pstmt, i+1, args[i].c_str(), -1, SQLITE_STATIC) != SQLITE_OK) {
            throw std::exception("unable to bind argument");
        }
    }

    // now, add results into the DBTable. 
    DBTable result;


}