#ifndef DATA_DEDUPLICATION_SERVICE_CLOCKARRAY_H
#define DATA_DEDUPLICATION_SERVICE_CLOCKARRAY_H

#include <array>
#include <numeric>
#include <iostream>
#include <chrono>
#include <map>
#include <stack>
#include <source_location>
#include <cassert>
#include <utility>
#include <thread>
#include <sstream>

template<typename T, std::size_t N>
std::array<T, N> constexpr makeArray(T val) {
    std::array<T, N> tempArray{};
    for (T &elem: tempArray)
        elem = val;
    return tempArray;
}

//for unordered maps
template<size_t N>
struct ::std::hash<std::array<std::string, N>> {
    std::size_t operator()(const std::array<std::string, N> &s) const noexcept {

        return std::hash<std::string>{}([&s]<std::size_t... Is>(std::index_sequence<Is...>) { return (s[Is]+...); }
                                                (std::make_index_sequence<N>{}));
    }
};


template<typename T, size_t sz>
requires std::is_convertible_v<T, std::string>
std::ostream &operator<<(std::ostream &out, std::array<T, sz> &arr);


template<typename T, size_t sz>
bool operator==(const std::array<T, sz> &arr1, const std::array<T, sz> &arr2) {
    return std::equal(arr1.begin(), arr1.end(), arr2);
}
/// timing namespace
namespace timing {

    auto cmpArrays = []<size_t N>(const std::array<std::string, N> &a, const std::array<std::string, N> &b) {
        for (int i = 0; i < N; i++) {
            if (a[i] != b[i])
                return b[i] > a[i];
        }
        return false;
    };

    using timepointType = std::chrono::system_clock::time_point;

    using locationType = std::array<std::string, 5>;

    template<typename T, typename T2, T2(*timeGetter)(), locationType (*src_to_loc_type)(
            std::source_location location),
            T(*double_cast)(T2 curr, T2 prev)> requires std::is_floating_point_v<T>
    class clockArray;


    locationType getFileState(std::source_location location
    = std::source_location::current());


    template<typename to_dur = std::chrono::nanoseconds, typename T>
    T doubleCastChrono(timepointType curr, timepointType prev) {
        return std::chrono::duration_cast<to_dur>(curr - prev).count();
    }

    constexpr const char *getFunctionName(const std::source_location &location
    = std::source_location::current());

    /**
     * @tparam chrono_duration_type
     */
    template<typename chrono_duration_type>
    using chronoClockTemplate = timing::clockArray<double, timepointType,
            &std::chrono::high_resolution_clock::now, &getFileState, &doubleCastChrono<chrono_duration_type>>;

    /**
     * @ingroup timing
     * @tparam OutType double type tha will be printed
     * @tparam inType Type that timeGetter return
     * @tparam timeGetter function that return current time
     * @tparam sourceTypeConverter function that converts source location to inner representation of it
     * @tparam timeConverter a function that casts time difference to type
     */
    template<typename OutType, typename inType, inType(*timeGetter)(), locationType (*sourceTypeConverter)(
            std::source_location location),
            OutType(*timeConverter)(inType curr, inType prev)> requires std::is_floating_point_v<OutType>
    class clockArray {
    public:
        struct timeStore {
            OutType time;
            size_t count;

            friend std::ostream &operator<<(std::ostream &out, const timeStore &ts) {
                out << ts.time;
                return out;
            }
        };

        /**
         * Resets timers and converted double values
         */
        void reset();


        /**
         * This function starts new calculation section
         * @param location source location of calle
         * @attention You must mirror every tik like call with
         * @ref timing::ClockArray< T, T2, timeGetter, src_to_loc_type, double_cast >::tak "tak()"
         */
        void tik(const std::source_location &location
        = std::source_location::current());


        /**
         * Finishes timing for specified section and calculation double value for time
         * @param location
         */
        void tak(const std::source_location &location
        = std::source_location::current());

        /**
         * This function returns it's source location to chain several
         * compute sections into one.
         * @details Example:
         * @details auto source =clk.tikLoc();
         * @details some_func();
         * @details clk.tak();
         * @details someOtherFunc();
         * @details clk.tik(source);
         * @details some_func();
         * @details clk.tak();
         * @param location
         * @return source location
         */
        locationType tikLoc(const std::source_location &location
        = std::source_location::current()) {
            tik(location);
            return sourceTypeConverter(location);
        }

        /**
         * @param location
         * @return pair of std::source_location, location_type
         */
        std::pair<std::source_location, locationType> tikPair(const std::source_location &location
        = std::source_location::current()) {
            return std::make_pair(location, tikLoc(location));
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
                   const clockArray<OutType, inType, timeGetter, sourceTypeConverter, timeConverter> &ts) {
            out << "Function name\tThread\tLine\tTime\n";
            for (auto &val: ts)
                out << val.first[0] << '\t' << val.first[4] << '\t'
                    << val.first[3] << ":" << val.first[1]
                    << '\t' << val.second << '\n';
            return out;
        }

        auto &operator[](const locationType &loc) {
            return timers[loc];
        }

        [[nodiscard]] bool contains(const locationType &loc) const {
            return timers.contains(loc);
        }

    private:
        std::map<locationType, timeStore, decltype(cmpArrays)> timers;

        std::map<locationType, inType, decltype(cmpArrays)> startIngTimers;
        std::stack<locationType> toTak;
        static inline std::mutex s_mutex;
        using guardType = std::lock_guard<std::mutex>;
    };

}


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


namespace timing {
    template<typename OutType, typename inType, inType (*timeGetter)(), locationType (*sourceTypeConverter)(
            std::source_location), OutType (*timeConverter)(inType, inType)>
    requires std::is_floating_point_v<OutType>void
    clockArray<OutType, inType, timeGetter, sourceTypeConverter, timeConverter>::tak(
            const std::source_location &location) {
        const guardType guard{s_mutex};
        auto id = (*sourceTypeConverter)(location);
        if (toTak.empty() || toTak.top()[0] != id[0]) {
            std::string msg = "No paired tik statement found in queue\t"
                              "Tak values" + id[3] + ":" + id[1] + "\t"
                              + id[0] + '\t' + id[2];
            throw std::logic_error(msg);
        }
        id[1] = toTak.top()[1];
        id[2] = toTak.top()[2];
        toTak.pop();
        auto res = timeConverter((*timeGetter)(), startIngTimers[id]);
        if (!timers.contains(id)) {
            timers[id] = {res, 1};
        } else {
            timers[id].time += res;
            timers[id].count++;
        }
    }

    template<typename OutType, typename inType, inType (*timeGetter)(), locationType (*sourceTypeConverter)(
            std::source_location), OutType (*timeConverter)(inType, inType)>
    requires std::is_floating_point_v<OutType>void
    clockArray<OutType, inType, timeGetter, sourceTypeConverter, timeConverter>::tik(
            const std::source_location &location) {
        const guardType guard{s_mutex};
        auto id = sourceTypeConverter(location);
        startIngTimers[id] = timeGetter();

        toTak.push(id);
    }

    template<typename OutType, typename inType, inType (*timeGetter)(), locationType (*sourceTypeConverter)(
            std::source_location), OutType (*timeConverter)(inType, inType)>
    requires std::is_floating_point_v<OutType>void
    clockArray<OutType, inType, timeGetter, sourceTypeConverter, timeConverter>::reset() {
        assert(toTak.empty());
        this->timers.clear();
        this->startIngTimers.clear();
    }
}


#endif // DATA_DEDUPLICATION_SERVICE_CLOCKARRAY_H
