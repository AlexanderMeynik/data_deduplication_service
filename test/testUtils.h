

#ifndef SOURCE_SERVICE_TESTUTILS_H
#define SOURCE_SERVICE_TESTUTILS_H
#include <tuple>
#include <array>
#include <type_traits>
#include <iostream>
#include <ServiceFileInterface.h>


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

inline std::ofstream & get_file_dir(std::string_view file_name)
{
    auto pp=get_normal_abs(file_name);
    if(!fs::exists(pp.parent_path()))
    {
        fs::create_directories(pp.parent_path());

    }
    std::ofstream out(pp);
    return std::ref(out);//todo check
}

template<size_t N>
std::string inline trimNsymbols(std::string&&trimmee)
{
    if(trimmee.size()<=2*N) {
        VLOG(1) << vformat("Trim Value %d is to large for string %s!", N, trimmee.c_str());
        return "";
    }
    return trimmee.substr(N,trimmee.length()-(1+N));
}
template<>
std::string inline trimNsymbols<0>(std::string&&trimmee)
{
    return trimmee;
}

template<typename T,typename A>
requires std::is_same_v<T,A>||
         std::is_same_v<T,typename std::remove_const<A>::type>||
         std::is_same_v<A,typename std::remove_const<T>::type>
void compareArr(size_t size,T *arr,A* arr2)
{
    for (int arr_elem = 0; arr_elem < size; ++arr_elem) {
        EXPECT_EQ(arr[arr_elem], arr2[arr_elem]);
        SCOPED_TRACE(arr_elem);
    }
}
enum check_from
{
    temporary_table,
    concolidate_from_saved
};
using namespace db_services;//todo
template<size_t size=64,check_from cs>
void inline get_file_from_temp_table(trasnactionType &txn,fs::path & original_file)
{
    char buff[size];
    std::ifstream in(original_file.c_str());
    auto res=db_services::check_file_existence(txn,original_file.c_str());

    auto file_size=res.one_row()["size_in_bytes"].as<index_type>();

    ASSERT_EQ(fs::file_size(original_file),file_size);

    auto hash_str=get_table_name(txn,original_file.c_str());
    std::string table_name=vformat("temp_file_%s",hash_str.c_str());



    int index_num=0;
    auto block_count=file_size/size;
    std::string query;
    if constexpr (cs==temporary_table) {
        query = vformat("select t.data from \"%s\" t "
                        "ORDER by t.pos", table_name.c_str());
    }
    else
    {
        query= vformat("select s.segment_data "
                       "from public.data"
                       "         inner join public.segments s on s.segment_hash = public.data.segment_hash "
                       "        inner join public.files f on f.file_id = public.data.file_id "
                       "where file_name=\'%s\'::tsvector "
                       "order by segment_num", original_file.c_str());
    }
    for (auto [name]: txn.stream<pqxx::binarystring>(query)) {
        //out<<name;
        auto size_1=in.readsome(buff, size);
        if(index_num<block_count) {
            compareArr(size,buff,name.str().c_str());
        }
        else
        {
            compareArr(size_1,buff,name.str().c_str());
        }
        SCOPED_TRACE(index_num);
        index_num++;
    }
}






template<size_t size=64>
void inline compare_files(fs::path &f1,fs::path& f2)
{
    ASSERT_TRUE(exists(f1));
    ASSERT_TRUE(exists(f2));
    auto fs=file_size(f1);
    ASSERT_EQ(fs,file_size(f2));
    size_t seg_count= fs / size;
    //https://stackoverflow.com/questions/50491833/how-do-you-read-n-bytes-from-a-file-and-put-them-into-a-vectoruint8-t-using-it


    std::ifstream i1(f1),i2(f2);
    char a1[size];
    char a2[size];
    int j = 0;
    for (; j < seg_count; ++j) {

        i1.readsome(a1, size);
        i2.readsome(a2, size);

        SCOPED_TRACE(j);
        compareArr(size,a1, a2);
    }

    auto sz=i1.readsome(a1, size);
    i2.readsome(a2, size);
    SCOPED_TRACE(j);
    compareArr(sz,a1, a2);



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
