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

DBTable DBManager::prepared_query(std::string q, const ArgumentList& args) {
    sqlite3_stmt* pstmt;
    if(sqlite3_prepare_v2(db, q.c_str(), q.size(), &pstmt, NULL) != SQLITE_OK) {
        throw std::runtime_error("unable to prepare statement");
    }
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

    sqlite3_finalize(pstmt);
    return result;

}