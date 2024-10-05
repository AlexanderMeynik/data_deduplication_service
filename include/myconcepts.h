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

static const int SHA256size = 32; //SHA256_DIGEST_LENGTH;

using CLOCK = timing::chrono_clock_template<std::chrono::milliseconds>;
static CLOCK clk;
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
std::string vformat(const char *const zcFormat, ...);


std::string string_to_hex(std::string_view in);

std::string hex_to_string(std::string_view in);


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
