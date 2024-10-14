

#ifndef DATA_DEDUPLICATION_SERVICE_HASHUTILS_H
#define DATA_DEDUPLICATION_SERVICE_HASHUTILS_H
#include <openssl/core.h>
#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/md2.h>
#include <array>

constexpr std::array<unsigned char* (*)(const unsigned char *d, size_t n, unsigned char *md),5>funcs
        ={&SHA224,&SHA256,
          &MD5, &SHA384,&SHA512};

enum hash_function {
    SHA_224,
    SHA_256,
    MD_5,
    SHA_384,
    SHA_512

};

static constexpr std::array<const char *, 5> hash_function_name
        {
                "sha224",
                "sha256",
                "md5",
                "sha384",
                "sha512"
        };

static constexpr std::array<unsigned short, 5> hash_function_size
        {
                28,
                32,
                32,
                48,
                64
        };
#endif //DATA_DEDUPLICATION_SERVICE_HASHUTILS_H
