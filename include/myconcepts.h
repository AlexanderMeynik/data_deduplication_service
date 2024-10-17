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

///concepts namespace
namespace my_concepts {

    /**
     * Global clock used for time measurement
     */
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






    template<typename T>
    concept printable=requires(T &elem, std::ofstream &out){
        { out << elem }->std::same_as<std::ostream &>;
    };

    constexpr int total_block_size = 128;
    template<unsigned short seg, unsigned short div>
    concept is_divisible = true;//todo remove

    using symbol_type = char;


    template<typename str>
    concept has_c_srt = requires(str &s) {
        { s.c_str() }->std::same_as<const char *>;
    };

    template<typename str>
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


    enum return_codes {
        warning_message = -3,
        already_exists = -2,
        error_occured = -1,
        return_sucess = 0
    };

    enum index_vals {
        empty_parameter_value = -1
    };
}
#endif //SERVICE_MYCONCEPTS_H
