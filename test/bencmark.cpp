#include <vector>
#include <iostream>
#include "FileService.h"
#include "testUtils.h"
#include <common.h>


namespace fs = std::filesystem;
std::string parent_path = "../../testDirectories/";
std::string new_dir_prefix = "../../testDirectoriesRes/";
std::vector<fs::path> from_dirs = {"shakespear",
                                   "test1",
                                   "res"};
std::vector<fs::path> to_dirs(from_dirs.size(), "");
constexpr std::array<int,1> indx={MD_5};
constexpr std::array<int,5> multipliers={4,8,16,32,64};

template<int hashNum,int multip>
void performStuff()
{
    std::ofstream timers("bench_timers.txt",std::ios::app);
    std::ofstream total_size("bench_total_sizes.txt",std::ios::app);
    std::ofstream sizes("bench_sizes.txt",std::ios::app);
    std::ofstream sizes2("bench_sizes2.txt",std::ios::app);
    clk.tik();
    constexpr auto segment_size=multip/**hash_function_size[hashNum]*/;
    FileParsingService<segment_size> fs;

    std::string dbName = std::string("deduplication_bench_")+hash_function_name[hashNum]+"_M"+std::to_string(multip);
    std::cout<<dbName<<'\n';
    fs.template db_load<db_usage_strategy::create,static_cast<hash_function>(hashNum)>(dbName);
    clk.tak();

    for (int i = 0; i < 1; ++i) {
        clk.tik();//test for simialr cases
        fs.template process_directory<preserve_old,static_cast<hash_function>(hashNum)>(from_dirs[i].string());
        clk.tak();
        clk.tik();
        fs.template load_directory<directory_handling_strategy::create_main>(from_dirs[i].string(), to_dirs[i].string());
        clk.tak();
        clk.tik();
        //fs.delete_directory(from_dirs[i].string());
        clk.tak();
    }

    auto total_file_size=fs.execute_in_transaction(&db_services::get_total_file_size);
    auto schemas=fs.execute_in_transaction(&db_services::get_total_schema_sizes);
    total_size<<dbName<<"\t"<<total_file_size.value_or(-1)<<'\n';
    sizes<<dbName<<"\n";
    db_services::print_table(schemas.value(),sizes);
    sizes<<'\n';

    auto dedup_data=fs.execute_in_transaction(&db_services::getDedupCharacteristics,(index_type)segment_size);
    sizes2<<dbName<<"\n";
    db_services::print_table(dedup_data.value(),sizes2);
    sizes2<<'\n';

   /* clk.tik();
    fs.db_drop(dbName);
    clk.tak();*/

    timers<<dbName<<'\n'<<clk<<"\n";



    clk.reset();
    timers.close();
    total_size.close();
    sizes.close();
    sizes2.close();
    //abort();
}
template<std_array container, container array>
requires std_array<typename container::value_type>
         && (typename decltype(array)::value_type{}.size() == 2)//just 2 args for function
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



int main(int argc, char *argv[]) {

    std::ofstream timers("bench_timers.txt");
    std::ofstream total_size("bench_total_sizes.txt");
    std::ofstream sizes("bench_sizes.txt");
    std::ofstream sizes2("bench_sizes2.txt");
    timers<<"\n";
    total_size<<'\n';
    sizes<<'\n';
    sizes2<<'\n';
    timers.close();
    total_size.close();
    sizes.close();
    sizes2.close();

    google::InitGoogleLogging(argv[0]);
    google::SetVLOGLevel("*", 3);

    for (int i = 0; i < from_dirs.size(); i++) {
        to_dirs[i] = get_normal_abs((new_dir_prefix / from_dirs[i]));
        from_dirs[i] = get_normal_abs(parent_path / from_dirs[i]);
    }


    constexpr auto entries2 = cartesian_product_arr(indx, multipliers);
    perform_stuff_on_2_d_array<decltype(entries2), entries2>();

}