

#ifndef SOURCE_SERVICE_TESTUTILS_H
#define SOURCE_SERVICE_TESTUTILS_H

#include <tuple>
#include <array>
#include <type_traits>
#include <iostream>
#include <FileService.h>
#include <gtest/gtest.h>

//namespace testUtils
namespace testUtils {

/**
 * @brief Struct for std::array checks
 */
    template<typename>
    struct is_std_array : std::false_type {
    };
/**
 * @brief Struct for std::array checks
 */
    template<typename T, std::size_t N>
    struct is_std_array<std::array<T, N>> : std::true_type {
    };
/**
 * @brief Struct for std::array checks
 */
    template<typename T, std::size_t N>
    struct is_std_array<const std::array<T, N>> : std::true_type {
    };

    /**
     *  concept std_array
     */
    template<typename T>
    concept std_array = is_std_array<T>::value;

/**
 * @brief Returns cartessian product on the ARRAYS of same types
 * @tparam T 
 * @tparam ARRAYS 
 * @param arrays 
 */
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
    for (
    std::size_t i = 1;
    i<dims.

    size();

    ++i) {
    dims[i] *= dims[i-1];
}
return [&] ()
{
std::array<type, N> result{};

for (
std::size_t i = 0;
i<result.

size();

++i)
{
[&]<
std::size_t... Is
>(std::index_sequence<Is...>)
{
std::array<std::size_t, outer_arr_size> idx = {((i * dims[Is]) / N) % arrays.size() ...};
result[i] ={ arrays[idx[Is]]...};
}(std::make_index_sequence<outer_arr_size>{ });
}
return
result;
}();
}

using namespace file_services;

template<typename...ARRAYS>
constexpr auto cartesianProductArr(ARRAYS...arrays) {
    //from https://stackoverflow.com/questions/45578484/is-it-possible-to-get-the-first-type-of-a-parameter-pack-in-a-one-liner/45578527#45578527
    return cartesian_product_arr_impl<typename std::tuple_element_t<0, std::tuple<ARRAYS...>>::value_type>(arrays...);
}

/**
 * Prints parameter pack fields
 * @tparam T 
 * @tparam Args 
 */
template<typename T, T ... Args>
void print() {
    ((std::cout << Args << '\t'), ...);
    std::cout << '\n';
}

/**
 * Will create parent directory for file if it doesn't exist
 * @param fileName 
 * @param out 
 */
inline void createHierarhyForFile(std::string_view fileName, std::ofstream &out) {
    auto pp = getNormalAbs(fileName);
    if (!fs::exists(pp.parent_path())) {
        fs::create_directories(pp.parent_path());
    }
    out.open(fileName.data());
}

/**
 * 
 * @tparam T 
 * @tparam A 
 * @param arr2 
 * @param arr 
 * @param size 
 */
//todo use concept for collections(size+index)
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

/// Enum to select where file data will come from
enum checkFrom {
///will retrieve data from temporary table(available before finish)
    TemporaryTable,

//will perform join inside database to retrieve file data
    ConsolidateFromSaved
};


using namespace db_services;

/**
 * Compares file from loacla storage to file from database.
 * @tparam compareWith
 * @param txn
 * @param originalFile
 * @param size
 */
template<checkFrom compareWith>
void inline getFileFromTempTable(trasnactionType &txn, fs::path &originalFile, size_t size = 64) {
    char buff[size];
    std::ifstream in(originalFile.c_str());
    auto res = db_services::checkFileExistence(txn, originalFile.c_str());

    auto fileSize = res.one_row()["size_in_bytes"].as<indexType>();

    ASSERT_EQ(fs::file_size(originalFile), fileSize);

    auto hashStr = getHashStr(originalFile.c_str());
    std::string tableName = vformat("temp_file_%s", hashStr.c_str());


    int indexNum = 0;
    auto blockCount = fileSize / size;
    std::string query;
    if constexpr (compareWith == TemporaryTable) {
        query = vformat("select t.data from \"%s\" t "
                        "ORDER by t.pos", tableName.c_str());
    } else {
        query = vformat("select s.segment_data "
                        "from public.data"
                        "         inner join public.segments s on s.segment_hash = public.data.segment_hash "
                        "        inner join public.files f on f.file_id = public.data.file_id "
                        "where file_name=\'%s\' "
                        "order by segment_num", db_services::toSpacedPath(originalFile.c_str()).c_str());
    }
    for (auto [name]: txn.stream<pqxx::binarystring>(query)) {
        auto size_1 = in.readsome(buff, size);
        if (indexNum < blockCount) {
            compareArr(name.str().c_str(), buff, size);
        } else {
            compareArr(name.str().c_str(), buff, size_1);
        }
        SCOPED_TRACE(indexNum);
        indexNum++;
    }
}


/**
 * Compares 2 files with EXPECT statements
 * @param f1 
 * @param f2 
 * @param size 
 */
void inline compareFiles(fs::path &f1, fs::path &f2, size_t size = 64) {
    ASSERT_TRUE(exists(f1));
    ASSERT_TRUE(exists(f2));
    auto fs = file_size(f1);
    ASSERT_EQ(fs, file_size(f2));
    size_t seg_count = fs / size;

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

/**
 * Prints template cartessian product element
 * @tparam container
 * @tparam array
 */
template<std_array container, container array>
requires std_array<typename container::value_type>
void printArgs() {
    using innerElemType = decltype(array)::value_type::value_type;
    constexpr size_t outerSize = decltype(array){}.size();
    constexpr size_t innerSize = typename decltype(array)::value_type{}.size();

    []<typename T, size_t N, size_t N2, std::array<std::array<T, N2>, N> out, std::size_t ... Is>(
            std::index_sequence<Is...>) {
        using out_elem_t = decltype(out)::value_type;
        ([]<out_elem_t inner_array, std::size_t... Is2>(std::index_sequence<Is2...>) {
            print<innerElemType, inner_array[Is2]...>();
        }.template operator()<out[Is]>(std::make_index_sequence<innerSize>{}), ...);

    }.template operator()<innerElemType, outerSize, innerSize, array>(std::make_index_sequence<outerSize>{});
}

}
using namespace testUtils;

#endif //SOURCE_SERVICE_TESTUTILS_H
