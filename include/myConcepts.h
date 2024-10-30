
#ifndef DATA_DEDUPLICATION_SERVICE_MYCONCEPTS_H
#define DATA_DEDUPLICATION_SERVICE_MYCONCEPTS_H

#include <glog/logging.h>

#include <cstdarg>
#include <concepts>
#include <array>

#include <type_traits>
#include "clockArray.h"

/// concepts namespace
namespace myConcepts {
    /**
    * Global clock used for time measurement
    */
    using clockType = timing::chronoClockTemplate<std::chrono::milliseconds>;
    extern clockType gClk;


    template<typename T>
    concept printable = requires(const T &elem, std::ofstream &out){
        { out << elem } -> std::same_as<std::ostream &>;
    };

    using SymbolType = char;


    // from https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
    std::string vformat(const char *zcFormat, ...);

    /**
     * Return codes enum
     */
    enum returnCodes {
        WarningMessage = -3,
        AlreadyExists = -2,
        ErrorOccured = -1,
        ReturnSucess = 0
    };

    /**
     * 
     */
    enum paramType {
        EmptyParameterValue = -1
    };


} // namespace myConcepts

#endif  // DATA_DEDUPLICATION_SERVICE_MYCONCEPTS_H
