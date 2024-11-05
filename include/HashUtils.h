

#ifndef DATA_DEDUPLICATION_SERVICE_HASHUTILS_H
#define DATA_DEDUPLICATION_SERVICE_HASHUTILS_H

#include <array>
#include <string>
#include <sstream>
#include <iomanip>


#include <openssl/core.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/md4.h>
#include <openssl/md2.h>
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
///hash utils namespace
namespace hash_utils {

    constexpr std::array<unsigned char *(*)(const unsigned char *d, size_t n, unsigned char *md), 6> funcs
            = {&SHA224, &SHA256,
               &MD5, &SHA384, &SHA512,&MD4};

    /**
     * Enum for selected hash function
     */
    enum hash_function {
        SHA_224,
        SHA_256,
        MD_5,
        SHA_384,
        SHA_512,
        MD_4
    };

    /**
     * Generates hash string with specified hash function
     * @tparam hash
     * @param stringView
     */
    template<hash_function hash = MD_5>
    std::string getHashStr(std::string_view stringView);

    /**
     * Lookup table for hash function size
     */
    static constexpr std::array<unsigned short, 6> hashFunctionSize
            {
                    SHA224_DIGEST_LENGTH,
                    SHA256_DIGEST_LENGTH,
                    MD5_DIGEST_LENGTH,
                    SHA384_DIGEST_LENGTH,
                    SHA512_DIGEST_LENGTH,
                    MD4_DIGEST_LENGTH
            };

    /**
     * Lookup table for hash function name
     */
    static constexpr std::array<const char *, 6> hashFunctionName
            {
                    "sha224",
                    "sha256",
                    "md5",
                    "sha384",
                    "sha512",
                    "md4"
            };


    /**
     * Converts string to hexadecimal string
     * @param in
     */
    std::string stringToHex(std::string_view in);

    //todo remake for raw buffers
    /**
     * Converts hexadecimal string to string
     * @param in
     */
    std::string hexToString(std::string_view in);


    template<hash_function hash>
    std::string getHashStr(std::string_view stringView) {
        unsigned char md[hashFunctionSize[hash]];
        funcs[hash](reinterpret_cast<const unsigned char *>(stringView.data()),
                    stringView.size(),
                    md
        );

        std::stringstream ss;
        for (size_t i = 0; i < hashFunctionSize[hash]; ++i) {
            ss << std::hex << std::setw(2)
               << std::setfill('0') << (int) md[i];
        }
        return ss.str();
    }

}

#endif //DATA_DEDUPLICATION_SERVICE_HASHUTILS_H
