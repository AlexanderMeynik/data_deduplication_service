#include <vector>
#include <iostream>
#include <unordered_set>

#include "FileService.h"
#include "testUtils.h"




namespace fs = std::filesystem;
std::string parent_path = "../../testDirectories/";
std::string new_dir_prefix = "../../testDirectoriesRes/";
std::vector<fs::path> from_dirs = {"images",
                                   "res",
                                   "res"};
std::vector<fs::path> to_dirs(from_dirs.size(), "");
constexpr std::array<int,5> indx={0,1,2,3,4};
constexpr std::array<int,8> multipliers={2,4,8,16,64,256,512,1024};
std::filesystem::path dir("becnhmarkData");


std::vector<std::shared_ptr<std::ofstream>> timerOstreams=std::vector<std::shared_ptr<std::ofstream>>(11,std::make_shared<
        std::ofstream>());
std::vector<std::string> timerPAths(11,"");
void createOrAppend(std::ofstream &out, std::string_view path)
{
    if(fs::exists(path))
    {
        out.open(path.data(),std::ios::app);
    }
    else
    {
        out.open(path.data());
    }
}

std::vector<std::string>ss
        {
                dir/"bench_timers.txt",
                dir/"bench_total_sizes.txt",
                dir/"bench_sizes.txt",
                dir/"bench_sizes2.txt"
        };
using namespace file_services;
const char *lineEnd="\n";
const char * lineTab="\t";
int ii=0;
template<int hashNum,int multip>
void performStuff()
{

    std::ofstream timers(ss[0],std::ios::app);
    std::ofstream totalSize(ss[1], std::ios::app);
    std::ofstream sizes(ss[2],std::ios::app);
    std::ofstream sizes2(ss[3],std::ios::app);
    gClk.tik();
    constexpr auto segment_size=multip*hash_function_size[hashNum];
    FileService fs;

    std::string dbName = std::string("deduplication_bench_ss")+hash_function_name[hashNum]+"_M"+std::to_string(multip);
    std::cout<<dbName<<'\n';
    fs.template dbLoad<dbUsageStrategy::create>(dbName);
    gClk.tak();

    for (int i = ii; i < 2; ++i) {
        gClk.tik();//test for simialr cases
        fs.template processDirectory<PreserveOld>(from_dirs[i].string(), segment_size,
                                                  static_cast<const hash_function>(hashNum));
        gClk.tak();
    }


    for (int i = ii; i < 2; ++i) {
        gClk.tik();
        fs.template loadDirectory<rootDirectoryHandlingStrategy::CreateMain>(from_dirs[i].string(), to_dirs[i].string());
        gClk.tak();
    }

    auto total_file_size= fs.executeInTransaction(&db_services::getTotalFileSize);
    auto schemas= fs.executeInTransaction(&db_services::getTotalSchemaSizes);
    totalSize << dbName << "\t" << total_file_size.value_or(-1).value_or(-1) << '\n';
    sizes<<dbName<<"\n";
    db_services::printRes(schemas.value(), sizes);
    sizes<<'\n';
    auto dedup_data= fs.executeInTransaction(&db_services::getDedupCharacteristics);

    sizes2<<dbName<<"\n";
    db_services::printRes(dedup_data.value(), sizes2);
    sizes2<<'\n';


    for (int i = ii; i < 2; ++i) {
        gClk.tik();
        fs.deleteDirectory(from_dirs[i].string());
        gClk.tak();
    }


    gClk.tik();
    fs.dbDrop(dbName);
    gClk.tak();




    timers << dbName << '\n' << gClk << "\n";

    int i=0;
    for (auto &val:gClk) {
        std::string name;
        name=(timerPAths[i].empty())?(dir/(val.first[3] + "_" + val.first[1])).string():timerPAths[i];
        createOrAppend(*timerOstreams[i].get(),name);


        if(multip==multipliers[0])
        {
            if(hashNum==0) {
                const char * aa="Hash\\Segment count";
                timerOstreams[i]->write(aa, strlen(aa));
                timerOstreams[i]->write(lineTab,1);
                for (auto &i2: multipliers) {
                    timerOstreams[i]->operator<<(hash_function_size[hashNum] * i2);
                    timerOstreams[i]->write(lineTab,1);
                }
            }
            timerOstreams[i]->write(lineEnd,1);
            timerOstreams[i]->write(hash_function_name[hashNum],strlen(hash_function_name[hashNum]));
            timerOstreams[i]->write(lineTab,1);
        }

        timerOstreams[i]->operator<<(val.second.count)<<lineTab;


        timerOstreams[i]->close();
        timerPAths[i]=name;
        i++;
    }

    gClk.reset();
    timers.close();
    totalSize.close();
    sizes.close();
    sizes2.close();
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


    for (int i = 0; i < from_dirs.size(); i++) {
        to_dirs[i] = getNormalAbs((new_dir_prefix / from_dirs[i]));
        from_dirs[i] = getNormalAbs(parent_path / from_dirs[i]);
    }

    /*auto res=compareDirectories(from_dirs[1],to_dirs[1],64);

    return 0;*/
    if(!fs::exists(dir))
    {
        fs::create_directories(dir);
    }
    std::ofstream timers(ss[0]);
    std::ofstream total_size(ss[1]);
    std::ofstream sizes(ss[2]);
    std::ofstream sizes2(ss[3]);
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


    constexpr auto entries2 = cartesian_product_arr(indx, multipliers);
    perform_stuff_on_2_d_array<decltype(entries2), entries2>();

}