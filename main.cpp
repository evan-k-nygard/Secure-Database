#include <iostream>
#include <string>
#include <cstring>
#include "dbmanager.h"
#include "cryptowrapper.h"

int main(int argc, char** argv) {

    std::string keystr("mypasswd");
    CryptoPP::SecByteBlock key = crypto::keygen(keystr);
    std::string ctext = crypto::encrypt("Hello, Raven! Tiger strength, speed, and power\n", key);
    std::cout << "encryption: " << ctext << '\n';
    std::cout << "decryption: " << crypto::decrypt(ctext, key) << '\n';
}