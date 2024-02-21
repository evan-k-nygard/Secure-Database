#include "cryptopp890/secblock.h"

namespace crypto {
    namespace _impl_details {
        // used to store specific cryptographic implementations of
        // various algorithms
        // This way, we can quickly change the underlying algorithm of the
        // wrapper functions, without needing to rewrite them or get rid of
        // our old work
        std::string bytes_to_string(const CryptoPP::SecByteBlock bytes);
        CryptoPP::SecByteBlock string_to_bytes(const std::string& str);

        std::string sha3_hash(const std::string& str);
        std::string aes_cbc_encrypt(const std::string& str, const CryptoPP::SecByteBlock key);
        std::string aes_cbc_decrypt(const std::string& str, const CryptoPP::SecByteBlock key);
        std::string aes_cbc_auth_encrypt(const std::string& str, const CryptoPP::SecByteBlock key);
        std::string aes_cbc_auth_decrypt(const std::string& str, const CryptoPP::SecByteBlock key);
        CryptoPP::SecByteBlock keygen_hkdf_sha3(const std::string& str, const std::string& salt);
    }

    std::string hash(const std::string& str);
    std::string encrypt(const std::string& str, const CryptoPP::SecByteBlock key);
    std::string decrypt(const std::string& ct, const CryptoPP::SecByteBlock key);
    CryptoPP::SecByteBlock master_keygen(const std::string& uname, const std::string& pwd);

    std::string random_token();
}