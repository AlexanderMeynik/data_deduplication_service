#ifndef DIPLOM_OPENMPPARRALELCLOCK_H
#define DIPLOM_OPENMPPARRALELCLOCK_H
#include <array>
#include <ranges>
#include <limits>
#include <algorithm>
#include <numeric>
#include <list>
#include <iostream>
#include <cmath>
#include <chrono>
#include <unordered_map>
#include <stack>

#include <source_location>

using location_type=std::array<std::string,3>;
template<>
struct std::hash<location_type>
{
    std::size_t operator()(const location_type & s) const noexcept
    {
        return std::hash<std::string>{}(s[0]+s[1]+s[2]);
    }
};
namespace timing {



    location_type get_file_state(const std::source_location& location
    = std::source_location::current())
    {
        std::string name=location.function_name();
        auto id=name.find(' ');
        auto id2=name.find('(');

        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now()-std::chrono::high_resolution_clock::now());
        return {name.substr(id+1,id2-id-1),std::to_string(location.line()),std::to_string(location.column())};

    }

    template<typename to_dur=std::chrono::nanoseconds,typename T>
    T double_cat_chrono(std::chrono::system_clock::time_point curr, std::chrono::system_clock::time_point prev)
    {
        return  std::chrono::duration_cast<to_dur>(curr-prev).count();
    }

    template<typename T,typename T2, T2(*timeGetter)(), location_type (*func_name)(const std::source_location& location),
    T(*double_cast)(std::chrono::system_clock::time_point curr,std::chrono::system_clock::time_point prev)>
    class ClockArray {
    public:
        template<typename Tp, typename BinaryOperation>
        Tp aggregate(Tp init,
                     BinaryOperation binary_op) {
            return std::accumulate(timers.begin(), timers.end(), init, binary_op);
        }

        void tak(const std::source_location& location
        = std::source_location::current()) {
            auto id = (*func_name)(location);
            if(to_tak.empty()||to_tak.top()[0]!=id[0])
            {
                return;
            }
            id[1]=to_tak.top()[1];
            id[2]=to_tak.top()[2];
            to_tak.pop();
            timers[id] =double_cast((*timeGetter)() , startIngTimers[id]);
        }

        void tik(const std::source_location& location
        = std::source_location::current()) {
            auto id=(*func_name)(location);
            startIngTimers[id] = (*timeGetter)();

            std::string name=location.function_name();
            auto iddd=name.find(' ');
            auto id2=name.find('(');

            to_tak.push({name.substr(iddd+1,id2-iddd-1),std::to_string(location.line()),std::to_string(location.column())});

        }

        auto begin()
        {
            return timers.begin();
        }

        auto end()
        {
            return timers.end();
        }

        //decltype()
    private:
        std::unordered_map<location_type ,T> timers;

        std::unordered_map<location_type ,T2>startIngTimers;
        std::stack<location_type> to_tak;
    };

    constexpr const char* get_function_name(const std::source_location& location
    = std::source_location::current())
    {
        return location.function_name();
    }


    auto chrono_dur()
    {
      //  auto res=std::chrono::high_resolution_clock::now();
        return std::chrono::high_resolution_clock::now();
    }

    template<typename dur>
    using chrono_clock_template = timing::ClockArray<double,std::chrono::system_clock::time_point
            , &std::chrono::high_resolution_clock::now, &get_file_state,&double_cat_chrono<dur>>;

}


#endif //DIPLOM_OPENMPPARRALELCLOCK_H
