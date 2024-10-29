

#ifndef SOURCE_SERVICE_TESTUTILS_H
#define SOURCE_SERVICE_TESTUTILS_H

#include <tuple>
#include <array>
#include <type_traits>
#include <iostream>
#include <FileService.h>
#include <gtest/gtest.h>

template<typename>
struct is_std_array : std::false_type {
};

template<typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {
};

template<typename T, std::size_t N>
struct is_std_array<const std::array<T, N>> : std::true_type {
};

template<typename T>
concept std_array = is_std_array<T>::value;

template<typename T, typename...ARRAYS>
constexpr auto cartesian_product_arr_impl(ARRAYS...arrays)requires (std_array<ARRAYS> &&

...)&&
(std::is_same_v<typename ARRAYS::value_type, T>&&...)//type equality checks
{
    using type = std::array<T, sizeof...(ARRAYS)>;//since I decided to have arrays with
    // same value type, we can use array
    constexpr std::size_t N = (1 * ... * arrays.size());
    constexpr std::size_t outer_arr_size = sizeof...(arrays);

    std::array<std::size_t, outer_arr_size> dims{arrays.size()...};
    for (std::size_t i = 1; i<dims.size();++i)
    {
        dims[i] *= dims[i-1];
    }
    return [&] ()
    {
        std::array<type, N> result{};

        for (std::size_t i = 0;i<result.size();++i)
        {
            [&]<std::size_t... Is>(std::index_sequence<Is...>)
            {
                std::array<std::size_t, outer_arr_size> idx = {((i * dims[Is]) / N) % arrays.size() ...};
                result[i] ={ arrays[idx[Is]]...};
            }(std::make_index_sequence<outer_arr_size>{ });
    }
    return result;
    }();
}

using namespace file_services;
template<typename...ARRAYS>
constexpr auto cartesian_product_arr(ARRAYS...arrays)//
{
    //from https://stackoverflow.com/questions/45578484/is-it-possible-to-get-the-first-type-of-a-parameter-pack-in-a-one-liner/45578527#45578527
    return cartesian_product_arr_impl<typename std::tuple_element_t<0, std::tuple<ARRAYS...>>::value_type>(arrays...);
}

template<typename T, T ... Args>
void print() {
    ((std::cout << Args << '\t'), ...);
    std::cout << '\n';
}

inline void create_hierarhy_for_file(std::string_view file_name, std::ofstream &out) {
    auto pp = getNormalAbs(file_name);
    if (!fs::exists(pp.parent_path())) {
        fs::create_directories(pp.parent_path());

    }
    out.open(file_name.data());
}


template<typename T, typename A>
requires std::is_same_v<T, A> ||
         std::is_same_v<T, typename std::remove_const<A>::type> ||
         std::is_same_v<A, typename std::remove_const<T>::type>
void compareArr(A *arr2, T *arr, size_t size) {
    for (int arr_elem = 0; arr_elem < size; ++arr_elem) {
        EXPECT_EQ(arr[arr_elem], arr2[arr_elem]);
        SCOPED_TRACE(arr_elem);
    }
}

enum check_from {
    temporary_table,
    concolidate_from_saved
};
using namespace db_services;
template< check_from cs>
void inline get_file_from_temp_table(trasnactionType &txn, fs::path &original_file,size_t size = 64) {
    char buff[size];
    std::ifstream in(original_file.c_str());
    auto res = db_services::checkFileExistence(txn, original_file.c_str());

    auto file_size = res.one_row()["size_in_bytes"].as<indexType>();

    ASSERT_EQ(fs::file_size(original_file), file_size);

    auto hash_str = getHashStr(original_file.c_str());
    std::string table_name = vformat("temp_file_%s", hash_str.c_str());


    int index_num = 0;
    auto block_count = file_size / size;
    std::string query;
    if constexpr (cs == temporary_table) {
        query = vformat("select t.data from \"%s\" t "
                        "ORDER by t.pos", table_name.c_str());
    } else {
        query = vformat("select s.segment_data "
                        "from public.data"
                        "         inner join public.segments s on s.segment_hash = public.data.segment_hash "
                        "        inner join public.files f on f.file_id = public.data.file_id "
                        "where file_name=\'%s\' "
                        "order by segment_num", db_services::toSpacedPath(original_file.c_str()).c_str());
    }
    for (auto [name]: txn.stream<pqxx::binarystring>(query)) {
        auto size_1 = in.readsome(buff, size);
        if (index_num < block_count) {
            compareArr(name.str().c_str(), buff, size);
        } else {
            compareArr(name.str().c_str(), buff, size_1);
        }
        SCOPED_TRACE(index_num);
        index_num++;
    }
}



void inline compare_files(fs::path &f1, fs::path &f2,size_t size = 64) {
    ASSERT_TRUE(exists(f1));
    ASSERT_TRUE(exists(f2));
    auto fs = file_size(f1);
    ASSERT_EQ(fs, file_size(f2));
    size_t seg_count = fs / size;
    //https://stackoverflow.com/questions/50491833/how-do-you-read-n-bytes-from-a-file-and-put-them-into-a-vectoruint8-t-using-it


    std::ifstream i1(f1), i2(f2);
    char a1[size];
    char a2[size];
    int j = 0;
    for (; j < seg_count; ++j) {

        i1.readsome(a1, size);
        i2.readsome(a2, size);

        SCOPED_TRACE(j);
        compareArr(a2, a1, size);
    }

    auto sz = i1.readsome(a1, size);
    i2.readsome(a2, size);
    SCOPED_TRACE(j);
    compareArr(a2, a1, sz);


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


#endif //SOURCE_SERVICE_TESTUTILS_H
