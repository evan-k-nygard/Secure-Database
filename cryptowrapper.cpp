#include <iostream>
#include <string>
#include "cryptowrapper.h"
#include "cryptopp890/sha3.h"
#include "cryptopp890/filters.h"
#include "cryptopp890/hex.h"
#include "cryptopp890/cryptlib.h"
#include "cryptopp890/secblock.h"
#include "cryptopp890/osrng.h"
#include "cryptopp890/aes.h"
#include "cryptopp890/modes.h"
#include "cryptopp890/hkdf.h"

// Convert bytes to string, or vice versa.
// The algorithm for these functions was taken from a suggestion in the 
// Crypto++ library:
// https://www.cryptopp.com/wiki/SecBlock 

std::string crypto::_impl_details::bytes_to_string(const CryptoPP::SecByteBlock bytes) {
    if(bytes.size() == 0) return std::string("");
    std::string result(reinterpret_cast<const char*>(&bytes[0]), bytes.size());
    return result;
}

CryptoPP::SecByteBlock crypto::_impl_details::string_to_bytes(const std::string& str) {
    // note: we take the memory of this string and cast it - if the string's
    // memory is deleted, does this result in a loss of memory?
    CryptoPP::SecByteBlock result(reinterpret_cast<const CryptoPP::byte*>(&str[0]), str.size());
    return result;
}

// Implementations of cryptography functions in cryptowrapper.h
// NOTE: Many of the functions were modified from this site:
// https://petanode.com/posts/brief-introduction-to-cryptopp/
// This site provided a general introduction to how the Crypto++ library
// works, and the example routines, or their overall structure, proved
// to be usable here.

std::string crypto::_impl_details::sha3_hash(const std::string& str) {
    auto sha3_machine = CryptoPP::SHA3_512();
    std::string result;

    CryptoPP::StringSource transformer(
        str, true,
        new CryptoPP::HashFilter(
            sha3_machine,
            new CryptoPP::HexEncoder(
                new CryptoPP::StringSink(result)
            )
        )
    );

    return result;
}

std::string crypto::_impl_details::aes_cbc_encrypt(const std::string& str, const CryptoPP::SecByteBlock key) {
    // Create the machines to perform encryption, encoding, and IV generation
    auto aes_start = CryptoPP::AES::Encryption(key, key.size());
    std::string result;
    CryptoPP::HexEncoder encoder(new CryptoPP::StringSink(result));

    // Generate the IV
    CryptoPP::SecByteBlock iv(CryptoPP::AES::BLOCKSIZE);
    CryptoPP::AutoSeededRandomPool rgen;
    rgen.GenerateBlock(iv, iv.size());

    auto aes_cbc_machine = CryptoPP::CBC_Mode_ExternalCipher::Encryption(aes_start, iv.data());

    // Encrypt the text using the IV
    std::string ctext;
    CryptoPP::StringSource transformer(
        str, true,
        new CryptoPP::StreamTransformationFilter(
            aes_cbc_machine,
            new CryptoPP::StringSink(ctext)
        )
    );
    
    // Fill the result string with the iv and encryption, and encode it
    encoder.Put(iv, iv.size());
    encoder.Put(reinterpret_cast<const CryptoPP::byte*>(ctext.data()), ctext.size());

    // return
    return result;
}

std::string crypto::_impl_details::aes_cbc_decrypt(const std::string& str, const CryptoPP::SecByteBlock key) {
    auto aes_start = CryptoPP::AES::Decryption(key.data(), key.size());
    std::string result;
    CryptoPP::HexDecoder decoder(new CryptoPP::StringSink(result));

    // decode the string
    std::string decoded;
    CryptoPP::StringSource decodemachine(
        str, true,
        new CryptoPP::HexDecoder(
            new CryptoPP::StringSink(decoded)
        )
    );

    // split IV from the actual encryption
    std::string ivstr = decoded.substr(0, CryptoPP::AES::BLOCKSIZE);
    std::string ciphertext = decoded.substr(CryptoPP::AES::BLOCKSIZE, std::string::npos);

    // use IV and key to decrypt the ciphertext
    CryptoPP::SecByteBlock iv = crypto::_impl_details::string_to_bytes(ivstr);
    auto aes_cbc_machine = CryptoPP::CBC_Mode_ExternalCipher::Decryption(aes_start, iv.data());
    CryptoPP::StringSource decryptmachine(
        ciphertext, true,
        new CryptoPP::StreamTransformationFilter(
            aes_cbc_machine,
            new CryptoPP::StringSink(result)
        )
    );

    // return
    return result;
}

CryptoPP::SecByteBlock crypto::_impl_details::keygen_hkdf_sha3(const std::string& str, const std::string& salt) {
    // note: the construction of this function significantly relied on the Crypto++ wiki here:
    // https://www.cryptopp.com/wiki/HKDF

    CryptoPP::byte result[CryptoPP::AES::DEFAULT_KEYLENGTH];
    CryptoPP::HKDF<CryptoPP::SHA3_512> hkdf_machine;

    hkdf_machine.DeriveKey(result, sizeof(result), crypto::_impl_details::string_to_bytes(str), str.size(),
                           crypto::_impl_details::string_to_bytes(salt), salt.size(), NULL, 0);
    return CryptoPP::SecByteBlock(result, sizeof(result));
}



std::string crypto::hash(const std::string& str) {
    return crypto::_impl_details::sha3_hash(str);
}

std::string crypto::encrypt(const std::string& str, const CryptoPP::SecByteBlock key) {
    return crypto::_impl_details::aes_cbc_encrypt(str, key);
}

std::string crypto::decrypt(const std::string& ct, const CryptoPP::SecByteBlock key) {
    return crypto::_impl_details::aes_cbc_decrypt(ct, key);
}

CryptoPP::SecByteBlock crypto::master_keygen(const std::string& uname, const std::string& pwd) {
    /*
    * generate a master key for the user with username "uname", using password
    * "pwd" and "uname" as the salt
    */
    return crypto::_impl_details::keygen_hkdf_sha3(pwd, uname);
}

std::string crypto::random_token() {
    /*
    * Generate a cryptographically secure random hash
    */
    CryptoPP::SecByteBlock token(CryptoPP::AES::BLOCKSIZE);
    CryptoPP::AutoSeededRandomPool rgen;
    rgen.GenerateBlock(token, token.size());
    return crypto::hash(crypto::_impl_details::bytes_to_string(token));
}