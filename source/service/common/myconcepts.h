#ifndef SERVICE_MYCONCEPTS_H
#define SERVICE_MYCONCEPTS_H

#include <concepts>
#include <array>
#include <vector>
#include <cstdarg>
#include <string>
#include <type_traits>
#include <glog/logging.h>
#include <iomanip>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include "ClockArray.h"
using verbose_level = unsigned short;//todo use VLOG for verbose logging

static const int SHA256size = 32; //SHA256_DIGEST_LENGTH;

using CLOCK =timing::chrono_clock_template<std::chrono::milliseconds>; //todo clock fails to execute when nested
static CLOCK  clk;
#define COMMA ,
//crutch
#define MEASURE_TIME(block) \
    clk.tik();              \
    block                   \
    clk.tak();
/*template<typename F, typename... Args>
double funcTime(F func, Args&&... args){
    clk.tik();
    func(std::forward<Args>(args)...);
    clk.tak();
    return duration(timeNow()-t1);
}*/

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


//constexpr int block_size=2;
constexpr int total_block_size = 128;
template<unsigned short seg, unsigned short div>
concept is_divisible = seg % div == 0;

using symbol_type = char;


template<typename str>
//has c_str()->const char* method
concept has_c_srt = requires(str &s) {
    { s.c_str() }->std::same_as<const char *>;
};

template<typename str>
//can be imlicitly converted to std::string and has c_str()->const char* method
concept to_str_to_c_str = requires(str &s) {
    std::is_convertible<str, std::string>::value &&has_c_srt<str>;
};

template<typename Col>
concept Index_size=requires(Col &c, size_t index)
                   {
                       { c[index] }->std::same_as<typename Col::value_type &>;
                   } && requires(Col &c)
                   {
                       { c.size() }->std::same_as<size_t>;
                   };


//from https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
std::string vformat(const char *const zcFormat, ...) {

    // initialize use of the variable argument array
    va_list vaArgs;
    va_start(vaArgs, zcFormat);

    // reliably acquire the size
    // from a copy of the variable argument array
    // and a functionally reliable call to mock the formatting
    va_list vaArgsCopy;
    va_copy(vaArgsCopy, vaArgs);
    const int iLen = std::vsnprintf(NULL, 0, zcFormat, vaArgsCopy);
    va_end(vaArgsCopy);

    // return a formatted string without risking memory mismanagement
    // and without assuming any compiler or platform specific behavior
    std::vector<char> zc(iLen + 1);
    std::vsnprintf(zc.data(), zc.size(), zcFormat, vaArgs);
    va_end(vaArgs);
    return std::string(zc.data(), iLen);
}


std::string string_to_hex(std::string_view in) {
    std::stringstream ss;

    ss << std::hex << std::setfill('0');
    for (size_t i = 0; in.length() > i; ++i) {
        ss << std::setw(2) << static_cast<unsigned int>(static_cast<unsigned char>(in[i]));
    }

    return ss.str();
}

std::string hex_to_string(std::string_view in) {
    std::string output;

    if ((in.length() % 2) != 0) {
        throw std::runtime_error("String is not valid length ...");
    }

    size_t cnt = in.length() / 2;

    for (size_t i = 0; cnt > i; ++i) {
        uint32_t s = 0;
        std::stringstream ss;
        ss << std::hex << in.substr(i * 2, 2);
        ss >> s;

        output.push_back(static_cast<unsigned char>(s));
    }

    return output;
}


enum return_codes {
    warning_message = -3,
    already_exists = -2,
    error_occured = -1,
    return_sucess = 0
};

enum index_vals {
    empty_parameter_value = -1
};

#endif //SERVICE_MYCONCEPTS_H
