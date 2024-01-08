#include <iostream>
#include <string>
#include "dbmanager.h"
#include "cryptowrapper.h"

int main(int argc, const char* argv[]) {
    if(argc != 3) {
        std::cerr << "Usage: newuser <username> <password>\n";
        return 1;
    }

    DB db("records.db");
    std::string uname(argv[1]);
    std::string pwd(argv[2]);
    std::string hashed_uname = crypto::hash(uname);
    std::string salted_pwd = crypto::hash(crypto::hash(uname + pwd));
    std::cout << "Creating account...\n";
    DBTable* check = db.prepared_query("SELECT username FROM Users WHERE username=?", ArgumentList({hashed_uname}));
    if(check->size() > 0) {
        std::cout << "Cannot create account: account already exists\n";
        delete check;
        return 0;
    }
    delete check;
    db.prepared_query("INSERT INTO Users (username, password) VALUES (?, ?)", ArgumentList({hashed_uname, salted_pwd}));
    std::cout << "Done!\n";
    return 0;
}