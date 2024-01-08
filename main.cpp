#include <iostream>
#include <string>
#include <cstring>
#include "dbmanager.h"
#include "cryptowrapper.h"

int main(int argc, char** argv) {

    std::string uname;
    std::string pwd;
    std::cout << "Username: ";
    std::cin >> uname;
    std::cout << "Password: ";
    std::cin >> pwd;

    CryptoPP::SecByteBlock key = crypto::master_keygen(uname, crypto::hash(pwd));
    std::string ctext = crypto::encrypt("Test encryption\n", key);
    std::cout << "encryption: " << ctext << '\n';
    std::cout << "decryption: " << crypto::decrypt(ctext, key) << '\n';
}