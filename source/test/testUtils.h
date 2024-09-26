//
// Created by Lenovo on 26.09.2024.
//

#ifndef SOURCE_SERVICE_TESTUTILS_H
#define SOURCE_SERVICE_TESTUTILS_H
#include <tuple>
#include <array>
#include <type_traits>
#include <iostream>
template<typename>
struct is_std_array : std::false_type {};

template<typename T, std::size_t N>
struct is_std_array<std::array<T,N>> : std::true_type {};

template<typename T, std::size_t N>
struct is_std_array<const std::array<T,N>> : std::true_type {};

template<typename T>
concept std_array = is_std_array<T>::value;

template<typename T,typename...ARRAYS>
constexpr auto cartesian_product_arr_impl (ARRAYS...arrays)
requires (std_array<ARRAYS>&&...)&&
    (std::is_same_v<typename ARRAYS::value_type,T>&&...)//type equality checks
{
    using type =std::array<T,sizeof...(ARRAYS)>;//since I decided to have arrays with
    // same value type, we can use array
    constexpr std::size_t N = (1 * ... * arrays.size());
    constexpr std::size_t outer_arr_size=sizeof...(arrays);

    std::array<std::size_t,outer_arr_size> dims { arrays.size()... };
    for (std::size_t i=1; i<dims.size(); ++i)  { dims[i] *= dims[i-1]; }

    return [&] ()
    {
        std::array<type, N> result{};

        for (std::size_t i=0; i<result.size(); ++i)
        {
            [&]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                std::array<std::size_t,outer_arr_size> idx = { ( (i*dims[Is]) / N) % arrays.size() ...};
                result[i] ={arrays[idx[Is]]...};

            }(std::make_index_sequence<outer_arr_size>{});
        }

        return result;
    }();
}


template<typename>
struct is_std_array : std::false_type {};

template<typename T, std::size_t N>
struct is_std_array<std::array<T,N>> : std::true_type {};

template<typename T, std::size_t N>
struct is_std_array<const std::array<T,N>> : std::true_type {};

template<typename T>
concept std_array = is_std_array<T>::value;

template<typename T,typename...ARRAYS>
constexpr auto cartesian_product_arr_impl (ARRAYS...arrays)
requires (std_array<ARRAYS>&&...)&&
(std::is_same_v<typename ARRAYS::value_type,T>&&...)//type equality checks
{
using type =std::array<T,sizeof...(ARRAYS)>;//since I decided to have arrays with
// same value type, we can use array
constexpr std::size_t N = (1 * ... * arrays.size());
constexpr std::size_t outer_arr_size=sizeof...(arrays);

std::array<std::size_t,outer_arr_size> dims { arrays.size()... };
for (std::size_t i=1; i<dims.size(); ++i)  { dims[i] *= dims[i-1]; }

return [&] ()
{
std::array<type, N> result{};

for (std::size_t i=0; i<result.size(); ++i)
{
[&]<std::size_t... Is>(std::index_sequence<Is...>)
{
std::array<std::size_t,outer_arr_size> idx = { ( (i*dims[Is]) / N) % arrays.size() ...};
result[i] ={arrays[idx[Is]]...};

}(std::make_index_sequence<outer_arr_size>{});
}

return result;
}();
}

template<typename...ARRAYS>
constexpr auto cartesian_product_arr (ARRAYS...arrays)//
{
    //from https://stackoverflow.com/questions/45578484/is-it-possible-to-get-the-first-type-of-a-parameter-pack-in-a-one-liner/45578527#45578527
    return cartesian_product_arr_impl<typename std::tuple_element_t<0, std::tuple<ARRAYS...>>::value_type>(arrays...);
}

template<typename T, T ... Args>
void print()
{
    ((std::cout<<Args<<'\t'),...);
    std::cout<<'\n';
}


template<std_array container, container array>
requires std_array<typename container::value_type>
void print_args() {
    using inner_elem_type = decltype(array)::value_type::value_type;
    constexpr size_t outer_size = decltype(array){}.size();
    constexpr size_t inner_size = typename decltype(array)::value_type{}.size();

    []<typename T, size_t N, size_t N2, std::array<std::array<T, N2>, N> out, std::size_t ... Is>(
            std::index_sequence<Is...>) {
        using out_elem_t = decltype(out)::value_type;
        ([]<out_elem_t inner_array, std::size_t... Is2>(std::index_sequence<Is2...>) {
            print<inner_elem_type, inner_array[Is2]...>();
        }.template operator()<out[Is]>(std::make_index_sequence<inner_size>{}), ...);

    }.template operator()<inner_elem_type, outer_size, inner_size, array>(std::make_index_sequence<outer_size>{});
}

template<std_array container, container array>
requires std_array<typename container::value_type>
         &&(typename decltype(array)::value_type{}.size()==2)//just 2 args for function
void perform_stuff_on_2_d_array() {
    using inner_elem_type = decltype(array)::value_type::value_type;
    constexpr size_t outer_size = decltype(array){}.size();
    constexpr size_t inner_size = typename decltype(array)::value_type{}.size();

    []<typename T, size_t N, size_t N2, std::array<std::array<T, N2>, N> out, std::size_t ... Is>(
            std::index_sequence<Is...>) {
        using out_elem_t = decltype(out)::value_type;
        ([]<out_elem_t inner_array, std::size_t... Is2>(std::index_sequence<Is2...>) {
            performStuff<inner_array[Is2]...>();//it seems to me that in order to insert function name here I need to wrap the whole thing in macro
        }.template operator()<out[Is]>(std::make_index_sequence<inner_size>{}), ...);

    }.template operator()<inner_elem_type, outer_size, inner_size, array>(std::make_index_sequence<outer_size>{});
}
/*
int main() {

    //print_args<decltype(entries), entries>();

    constexpr auto entries2 = cartesian_product_arr(a1, a2);
    print_args<decltype(entries2), entries2>();
    perform_stuff_on_2_d_array<decltype(entries2), entries2>();

    //perform_stuff_on_2_d_array<decltype(entries), entries>();//sfinae check
}*/
#endif //SOURCE_SERVICE_TESTUTILS_H
