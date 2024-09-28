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

using location_type=std::array<std::string,4>;
template<typename T, size_t sz>
requires std::is_convertible_v<T,std::string>
std::ostream& operator<< (std::ostream&out,std::array<T,sz>&arr)
{
    int i = 0;
    for (; i < sz-1; ++i) {
        out<<arr[i]<<'\t';
    }
    out<<arr[i]<<'\n';
    return out;
}
template<>
struct std::hash<location_type>
{
    std::size_t operator()(const location_type & s) const noexcept
    {
        return std::hash<std::string>{}(s[0]+s[1]+s[2]+s[3]);
    }
};
namespace timing {

    struct location_wrapper
    {
        const std::source_location& ref{};
    };


    inline location_type get_file_state(std::source_location location
    = std::source_location::current())
    {
        std::string name=location.function_name();
        auto id=name.find(' ');
        auto id2=name.find('(');

        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now()-std::chrono::high_resolution_clock::now());
        std::string fname=location.file_name();
        return {name.substr(id+1,id2-id-1),std::to_string(location.line()),std::to_string(location.column()),fname.substr(fname.rfind('/')+1)};

    }

    template<typename to_dur=std::chrono::nanoseconds,typename T>
    T double_cat_chrono(std::chrono::system_clock::time_point curr, std::chrono::system_clock::time_point prev)
    {
        return  std::chrono::duration_cast<to_dur>(curr-prev).count();
    }

    template<typename T,typename T2, T2(*timeGetter)(), location_type (*func_name)(std::source_location location),
    T(*double_cast)(T2 curr,T2 prev)>
    class ClockArray {
    public:

        struct time_store
        {
            T time;
            size_t count;
            friend std::ostream &operator<<(std::ostream &out,const time_store&ts)
            {
                out<<ts.time;///ts.count;
                return out;
            }
        };
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
                std::string msg="No paired tik statement found in queue\t"
                                "Tak values"+id[3]+":"+id[1]+"\t"
                                +id[0]+'\t'+id[2];
                throw std::logic_error(msg);
            }
            id[1]=to_tak.top()[1];
            id[2]=to_tak.top()[2];
            to_tak.pop();
            auto res=double_cast((*timeGetter)() , startIngTimers[id]);
            if(!timers.contains(id))
            {
                timers[id] ={res,1};
            }
            else
            {
                timers[id].time+=res;
                timers[id].count++;
            }
        }

        location_type tik_loc(const std::source_location& location
        = std::source_location::current())
        {
            tik(location);
            return func_name(location);

        }

        std::pair<std::source_location,location_type> tik_loc_(const std::source_location& location
        = std::source_location::current())
        {
            return std::make_pair(location,tik_loc(location));
        }
        void tik(const std::source_location& location
        = std::source_location::current()) {
            auto id=func_name(location);
            startIngTimers[id] = timeGetter();

            to_tak.push(id);
        }

        decltype(auto) begin() const
        {
            return timers.begin();
        }

        decltype(auto)  end() const
        {
            return timers.end();
        }

        auto cbegin() const
        {
            return timers.cbegin();
        }

        auto cend() const
        {
            return timers.cend();
        }

        friend std::ostream &operator<<(std::ostream &out,const ClockArray<T,T2,timeGetter,func_name,double_cast>&ts)
        {
            out<<"Function name\tLine\tTime\n";
            for (auto& val:ts) {
                std::cout<<val.first[0]<<'\t'<<val.first[3]<<":"<<val.first[1]<<'\t'<<val.second<<'\n';
            }
            return out;
        }
        auto& operator[](location_type&loc)
        {
            return timers[loc];
        }
        bool contains(location_type&loc)
        {
            return timers.contains(loc);
        }


        //decltype()
    private:
        std::unordered_map<location_type ,time_store> timers;

        std::unordered_map<location_type ,T2>startIngTimers;
        std::stack<location_type> to_tak;
    };

    constexpr const char* get_function_name(const std::source_location& location
    = std::source_location::current())
    {
        return location.function_name();
    }


    template<typename dur>
    using chrono_clock_template = timing::ClockArray<double,std::chrono::system_clock::time_point
            , &std::chrono::high_resolution_clock::now, &get_file_state,&double_cat_chrono<dur>>;

}


#endif //DIPLOM_OPENMPPARRALELCLOCK_H
