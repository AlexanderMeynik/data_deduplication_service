#ifndef SERVICE_MYCONCEPTS_H
#define SERVICE_MYCONCEPTS_H
#include <concepts>
#include <array>
#include <vector>
#include <cstdarg>
#include <string>
#include <type_traits>
static const int SHA256size = 32; //SHA256_DIGEST_LENGTH;


template<unsigned short seg, unsigned short div>
concept IsDiv = seg % div == 0;

template<unsigned long segment_size> requires IsDiv<segment_size, SHA256size>
using segment = std::array<char, segment_size>;
template<unsigned long segment_size> requires IsDiv<segment_size, SHA256size>
using segvec = std::vector<segment<segment_size>>;

template<typename str>
//has c_str()->const char* method
concept has_c_srt = requires (str &s) {
    { s.c_str() }->std::same_as<const char*>;
};

template<typename str>
//can be imlicitly converted to std::string and has c_str()->const char* method
concept is_twice_string_convertible = requires (str &s) {
    std::is_convertible<str,std::string>::value&&has_c_srt<str>;
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


#endif //SERVICE_MYCONCEPTS_H
