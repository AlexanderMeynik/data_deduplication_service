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
#include <cassert>

using location_type = std::array<std::string, 4>;

template<>
struct ::std::hash<location_type> {
    std::size_t operator()(const location_type &s) const noexcept {
        return std::hash<std::string>{}(s[0] + s[1] + s[2] + s[3]);
    }
};

template<typename T, size_t sz>
requires std::is_convertible_v<T, std::string>
std::ostream &operator<<(std::ostream &out, std::array<T, sz> &arr) {
    int i = 0;
    for (; i < sz - 1; ++i) {
        out << arr[i] << '\t';
    }
    out << arr[i] << '\n';
    return out;
}

template<typename T, size_t sz>
bool operator==(const std::array<T, sz> &arr1, const std::array<T, sz> &arr2) {
    return std::equal(arr1.begin(), arr1.end(), arr2);
}
///timing namespace
namespace timing {


    using timepoint_type = std::chrono::system_clock::time_point;

    using location_type = std::array<std::string, 4>;

    template<typename T, typename T2, T2(*timeGetter)(), location_type (*src_to_loc_type)(
            std::source_location location),
            T(*double_cast)(T2 curr, T2 prev)> requires std::is_floating_point_v<T>
    class ClockArray;


    inline location_type get_file_state(std::source_location location
    = std::source_location::current()) {
        std::string name = location.function_name();
        auto id = name.find(' ');
        auto id2 = name.find('(');

        std::string fname = location.file_name();
        return {name.substr(id + 1, id2 - id - 1), std::to_string(location.line()), std::to_string(location.column()),
                fname.substr(fname.rfind('/') + 1)};

    }


    template<typename to_dur=std::chrono::nanoseconds, typename T>
    T double_cat_chrono(timepoint_type curr, timepoint_type prev) {
        return std::chrono::duration_cast<to_dur>(curr - prev).count();
    }

    constexpr const char *get_function_name(const std::source_location &location
    = std::source_location::current()) {
        return location.function_name();
    }

    /**
     * @tparam chrono_duration_type
     */
    template<typename chrono_duration_type>
    using chrono_clock_template = timing::ClockArray<double, timepoint_type,
            &std::chrono::high_resolution_clock::now, &get_file_state, &double_cat_chrono<chrono_duration_type>>;

    /**
     * @ingroup timing
     * @tparam OutType double type tha will be printed
     * @tparam inType Type that timeGetter return
     * @tparam timeGetter function that return current time
     * @tparam sourceTypeConverter function that converts source location to inner representation of it
     * @tparam timeConverter a function that casts time difference to type
     */
    template<typename OutType, typename inType, inType(*timeGetter)(), location_type (*sourceTypeConverter)(
            std::source_location location),
            OutType(*timeConverter)(inType curr, inType prev)> requires std::is_floating_point_v<OutType>
    class ClockArray {
    public:


        struct time_store {
            OutType time;
            size_t count;

            friend std::ostream &operator<<(std::ostream &out, const time_store &ts) {
                out << ts.time;///ts.count;
                return out;
            }
        };

        /**
         * Resets timers and converted double values
         */
        void reset() {
            assert(to_tak.empty());
            this->timers.clear();
            this->startIngTimers.clear();
        }

        /**
         * Applies Binary Binary operation to all clocks
         * @tparam ReturnType
         * @tparam BinaryOperation
         * @param starting_value
         * @param binary_op
         * @return accumulated value
         */
        template<typename ReturnType, typename BinaryOperation>
        ReturnType aggregate(ReturnType starting_value,
                             BinaryOperation binary_op) {
            return std::accumulate(timers.begin(), timers.end(), starting_value, binary_op);
        }

        /**
         * Finishes timing for specified section and calculation double value for time
         * @param location
         */
        void tak(const std::source_location &location
        = std::source_location::current()) {
            auto id = (*sourceTypeConverter)(location);
            if (to_tak.empty() || to_tak.top()[0] != id[0]) {
                std::string msg = "No paired tik statement found in queue\t"
                                  "Tak values" + id[3] + ":" + id[1] + "\t"
                                  + id[0] + '\t' + id[2];
                throw std::logic_error(msg);
            }
            id[1] = to_tak.top()[1];
            id[2] = to_tak.top()[2];
            to_tak.pop();
            auto res = timeConverter((*timeGetter)(), startIngTimers[id]);
            if (!timers.contains(id)) {
                timers[id] = {res, 1};
            } else {
                timers[id].time += res;
                timers[id].count++;
            }
        }

        /**
         * This function returns it's source location to chain several
         * compute sections into one.
         * @details Example:
         * @details auto source =clk.tik_loc();
         * @details some_func();
         * @details clk.tak();
         * @details someOtherFunc();
         * @details clk.tik(source);
         * @details some_func();
         * @details clk.tak();
         * @param location
         * @return source location
         */
        location_type tik_loc(const std::source_location &location
        = std::source_location::current()) {
            tik(location);
            return sourceTypeConverter(location);

        }

        /**
         * @param location
         * @return pair of std::source_location, location_type
         */
        std::pair<std::source_location, location_type> tik_loc_(const std::source_location &location
        = std::source_location::current()) {
            return std::make_pair(location, tik_loc(location));
        }

        /**
         * This function starts new calculation section
         * @param location source location of calle
         * @attention You must mirror every tik like call with
         * @ref timing::ClockArray< T, T2, timeGetter, src_to_loc_type, double_cast >::tak "tak()"
         */
        void tik(const std::source_location &location
        = std::source_location::current()) {
            auto id = sourceTypeConverter(location);
            startIngTimers[id] = timeGetter();

            to_tak.push(id);
        }

        decltype(auto) begin() const {
            return timers.begin();
        }

        decltype(auto) end() const {
            return timers.end();
        }

        auto cbegin() const {
            return timers.cbegin();
        }

        auto cend() const {
            return timers.cend();
        }

        friend std::ostream &
        operator<<(std::ostream &out,
                   const ClockArray<OutType, inType, timeGetter, sourceTypeConverter, timeConverter> &ts) {
            out << "Function name\tLine\tTime\n";
            for (auto &val: ts) {
                out << val.first[0] << '\t' << val.first[3] << ":" << val.first[1] << '\t' << val.second << '\n';
            }
            return out;
        }

        auto &operator[](location_type &loc) {
            return timers[loc];
        }

        bool contains(location_type &loc) {
            return timers.contains(loc);
        }

    private:
        std::unordered_map<location_type, time_store> timers;

        std::unordered_map<location_type, inType> startIngTimers;
        std::stack<location_type> to_tak;
    };

}


#endif //DIPLOM_OPENMPPARRALELCLOCK_H
