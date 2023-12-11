#include <iostream>
#include <string>
#include <cstring>
#include "dbmanager.h"

int main(int argc, char** argv) {
    std::string uname;
    std::string pwd;

    std::cout << "Username: ";
    std::cin >> uname;
    std::cout << "Password: ";
    std::cin >> pwd;

    DBManager db("records.db", pwd.c_str());

    db.prepared_query("INSERT INTO TEST VALUES(?, ?, ?)", ArgumentList({"two", "five", "one"}));
    // double check (double?) hash of password against hash of username
    // if okay, allow access to the database
    // if not okay, print error and exit (or ask for uname/pwd combo again)

    // storage: one completely shared database?
    // or two 

    /*if(argc == 2 && strncmp(argv[1], "--create", strlen(argv[1])) == 0) {
        std::string encrypt_key = HASH(pwd);
        std::string pwd_storage_hash = HASH(encrypt_key);
        std::string uname_encrypt = ENCRYPT(uname, encrypt_key);
        
    }*/


    return 0;
}