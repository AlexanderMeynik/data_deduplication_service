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
using verbose_level=unsigned short;//todo enum

static const int SHA256size = 32; //SHA256_DIGEST_LENGTH;
//constexpr int block_size=2;
constexpr int total_block_size=128;
template<unsigned short seg, unsigned short div>
concept is_divisible = seg % div == 0;
using symbol_type=char;
template<unsigned long segment_size> requires is_divisible<segment_size, SHA256size>
using segment = std::array<symbol_type, segment_size>;
template<unsigned long segment_size> requires is_divisible<segment_size, SHA256size>
using segvec = std::vector<segment<segment_size>>;//todo delete
template<unsigned long segment_size,unsigned long segment_count>
using block = std::array<segment<segment_size>,segment_count>;
template<typename str>
//has c_str()->const char* method
concept has_c_srt = requires (str &s) {
    { s.c_str() }->std::same_as<const char*>;
};

template<typename str>
//can be imlicitly converted to std::string and has c_str()->const char* method
concept to_str_to_c_str = requires (str &s) {
    std::is_convertible<str,std::string>::value&&has_c_srt<str>;
};

template<typename Col>
concept Index_size=requires(Col&c,size_t index)
                   {
                       {c[index]}->std::same_as<typename Col::value_type &>;
                   }&&
                   requires(Col&c)
                   {
                       {c.size()}->std::same_as<size_t>;
                   };




//from https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
std::string vformat(const char * const zcFormat, ...) {

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

const char* ws = "\0";

// trim from end of string (right)
inline std::string& rtrim(std::string& s, const char* t = ws)
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from beginning of string (left)
inline std::string& ltrim(std::string& s, const char* t = ws)
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from both ends of string (right then left)
inline std::string& trim(std::string& s, const char* t = ws)
{
    return ltrim(rtrim(s, t), t);
}


#endif //SERVICE_MYCONCEPTS_H
