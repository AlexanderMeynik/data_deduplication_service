#include <vector>
#include <iostream>
#include <unordered_set>

#include "FileService.h"
#include "testUtils.h"


namespace fs = std::filesystem;
template<typename T, typename A>
requires std::is_same_v<T, A> ||
         std::is_same_v<T, typename std::remove_const<A>::type> ||
         std::is_same_v<A, typename std::remove_const<T>::type>
int compareArraa(size_t size, T *arr, A *arr2) {
    int error = 0;
    for (int arr_elem = 0; arr_elem < size; ++arr_elem) {
        error += arr[arr_elem] != arr2[arr_elem];
    }
    return error;
}

std::array<size_t,3> inline compareFiles(fs::path &f1, fs::path &f2, size_t size) {
    size_t e1=0;//todo add size checks
    size_t e2=0;
    auto fs = file_size(f1);
    size_t seg_count = fs / size;
    size_t last=fs-seg_count*size;

    std::ifstream i1(f1), i2(f2);
    char a1[size];
    char a2[size];
    int j = 0;
    for (; j < seg_count; ++j) {

        i1.readsome(a1, size);
        i2.readsome(a2, size);
        auto res=compareArraa(size, a1, a2);
        e1+=res;
        e2+=(res!=0);
    }

    if(last>0)
    {
        auto sz = i1.readsome(a1, size);
        i2.readsome(a2, size);
        auto res = compareArraa(sz, a1, a2);
        e1 += res;
        e2 += (res != 0);
        seg_count++;
    }
    return {e1,e2,seg_count};

}


std::array<size_t,3> inline compareDirectories(fs::path &f1, fs::path &f2, size_t size) {
    size_t e1=0;
    size_t e2=0;
    size_t seg_count = 0;

    std::unordered_set<std::string> f1_s;
    std::unordered_set<std::string> f2_s;

    for (auto&entry:fs::recursive_directory_iterator(f1)) {
        if(!fs::is_directory(entry))
        {
            auto path=entry.path().lexically_relative(f1).string();
            f1_s.insert(path);
        }
    }

    for (auto&entry:fs::recursive_directory_iterator(f2)) {
        if(!fs::is_directory(entry))
        {
            auto path=entry.path().lexically_relative(f2).string();
            f2_s.insert(path);
        }
    }

    bool aa=std::all_of(f2_s.begin(), f2_s.end(), [&](const auto &item) {
        return f1_s.contains(item);
    });

    for (auto elem:f1_s) {
        auto in=f1/elem;
        auto out=f2/elem;
        auto res= compareFiles(in,out,size);
        e1+=res[0];
        e2+=res[1];
        seg_count+=res[2];
    }

    return {e1,e2,seg_count};

}



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
    FileParsingService fs;

    std::string dbName = std::string("deduplication_bench_ss")+hash_function_name[hashNum]+"_M"+std::to_string(multip);
    std::cout<<dbName<<'\n';
    fs.template dbLoad<dbUsageStrategy::create>(dbName);
    gClk.tak();

    for (int i = ii; i < 2; ++i) {
        gClk.tik();//test for simialr cases
        fs.template processDirectory<PreserveOld, static_cast<hash_function>(hashNum)>(from_dirs[i].string(),segment_size);
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
    //todo add benchmark job
    //todo unique segment count dont work porperly(it only counts segments unque to current file)

    //todo later add
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
                const char * aa="Hash\\Segemnt count";
                timerOstreams[i]->write(aa, strlen(aa));
                timerOstreams[i]->write(lineTab,1);
                for (auto &i: multipliers) {
                    timerOstreams[i]->operator<<(hash_function_size[hashNum] * i);
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

   /* compareDirectories(from_dirs[1],to_dirs[1],64);*/


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