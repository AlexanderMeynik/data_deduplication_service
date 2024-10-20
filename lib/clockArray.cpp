#include "clockArray.h"

namespace timing
{

    locationType getFileState(std::source_location location) {
        std::string name = location.function_name();
        auto id = name.find(' ');
        auto id2 = name.find('(');

        std::string fname = location.file_name();
        std::stringstream ss;
        ss<<std::this_thread::get_id();
        return {name.substr(id + 1, id2 - id - 1),
                std::to_string(location.line()),
                std::to_string(location.column()),
                fname.substr(fname.rfind('/') + 1),
                ss.str()};
    }

    constexpr const char *getFunctionName(const std::source_location &location) {
        return location.function_name();
    }





}

