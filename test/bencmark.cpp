#include <vector>
#include <iostream>
#include <unordered_set>

#include "FileService.h"
#include "testUtils.h"


namespace fs = std::filesystem;

fs::path pparent="../..";
std::string parent_path = pparent/"testDirectories/";
std::string new_dir_prefix = pparent/"testDirectoriesRes/";
std::vector<fs::path> from_dirs = {"images",
                                   "res",
                                   "res"};
std::vector<fs::path> to_dirs(from_dirs.size(), "");
constexpr std::array<int,6> indx= {0,1,2,3,4,5};
constexpr std::array<int,10> multipliers={2,4,8,16,64,256,512,1024,2048,4096};
std::filesystem::path dir("becnhmarkData");

std::unordered_map<std::string,std::shared_ptr<std::ofstream>>timerOstreams;

bool upsert(const std::string& filename,std::unordered_map<std::string,std::shared_ptr<std::ofstream>>&map)
{
    if(!map.contains(filename))
    {
        map[filename]=std::make_shared<std::ofstream>();
        map[filename]->open(filename);
    }
    return map[filename]->is_open();
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


void tablePrint(const std::string &name, const std::string &value, int multip, size_t hashNum);

int main(int argc, char *argv[]) {


    for (int i = 0; i < from_dirs.size(); i++) {
        to_dirs[i] = getNormalAbs((new_dir_prefix / from_dirs[i]));
        from_dirs[i] = getNormalAbs(parent_path / from_dirs[i]);
    }

    if(!fs::exists(dir))
    {
        fs::create_directories(dir);
    }
    std::ofstream timers(ss[0]);
    std::ofstream totalSize(ss[1]);
    std::ofstream sizes(ss[2]);
    std::ofstream sizes2(ss[3]);
    timers<<"\n";
    totalSize<<'\n';
    sizes<<'\n';
    sizes2<<'\n';


    google::InitGoogleLogging(argv[0]);
    google::SetVLOGLevel("*", 3);


    constexpr auto entries2 = cartesian_product_arr(indx, multipliers);

    for (auto& elem:entries2) {
        auto hashNum=elem[0];
        auto multip=elem[1];

        gClk.tik();
        auto segment_size=multip;
        FileService fs;

        std::string dbName = std::string("deduplication_bench_ss") + hashFunctionName[hashNum] + "_M" + std::to_string(multip);
        std::cout<<dbName<<'\n';
        fs.template dbLoad<dbUsageStrategy::create>(dbName);
        gClk.tak();

        for (int i = ii; i < 2; ++i) {
            gClk.tik();
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


        std::string nn1=dir/("coefficient");
        tablePrint(nn1,std::to_string(fs.getCoefficient().value_or(-1)),multip,hashNum);


        auto nn3=dir/("dedupProcent");
        auto res=fs.getDataD();
        if(!res.has_value())
        {
            for (int i=0;i<res->size();i++) {
                std::string nn2=dir/("dedupProcent"+std::to_string(i));
                tablePrint(nn2,std::to_string(-1),multip,hashNum);
            }
            tablePrint(nn3,std::to_string(-1),multip,hashNum);
        }
        else {

            for (int i = 0; i < res->size(); i++) {
                std::string nn2 = dir / ("dedupProcent" + std::to_string(i));
                tablePrint(nn2, std::to_string(res.value()[i]), multip, hashNum);
            }
            tablePrint(nn3,std::to_string((res.value()[0]*100.0)/((res.value()[1]+res.value()[2]))),multip,hashNum);
        }



        for (int i = ii; i < 2; ++i) {
            gClk.tik();
            fs.deleteDirectory(from_dirs[i].string());
            gClk.tak();
        }


        gClk.tik();
        fs.dbDrop(dbName);
        gClk.tak();


        std::array<size_t, 4> comparisonRes{0};
        for (int i = ii; i < 2; ++i) {
            gClk.tik();
            auto temp=compareDirectories(from_dirs[i].string(), to_dirs[i].string(),segment_size);
            for (int j = 0; j < comparisonRes.size(); ++j) {
                comparisonRes[j]+=temp[j];
            }
            gClk.tak();
        }


        for (int j = 0; j < comparisonRes.size(); ++j) {
            std::string name=dir/("comparisonResult"+std::to_string(j));
            tablePrint(name,std::to_string(comparisonRes[j]),multip,hashNum);
        }


        timers << dbName << '\n' << gClk << "\n";

        for (auto &val:gClk) {
            std::string name=dir/(val.first[3] + "_" + val.first[1]);
            tablePrint(name,std::to_string((int)val.second.time),multip,hashNum);
        }

        gClk.reset();

    }

    for (auto &ptr:timerOstreams) {
        ptr.second->close();
    }

    timers.close();
    totalSize.close();
    sizes.close();
    sizes2.close();

}

void tablePrint(const std::string &name, const std::string &value, int multip, size_t hashNum) {
    auto res=upsert(name,timerOstreams);
    auto osPtr=timerOstreams[name];


    if(multip==multipliers[0])
    {
        if(hashNum==0) {
            const char * aa="Hash\\Segment size";
            osPtr->write(aa, strlen(aa));
            osPtr->write(lineTab,1);
            for (auto &i2: multipliers) {
                osPtr->operator<<(/*hash_function_size[hashNum]*/  i2);
                osPtr->write(lineTab,1);
            }
        }
        osPtr->write(lineEnd,1);
        osPtr->write(hashFunctionName[hashNum], strlen(hashFunctionName[hashNum]));
        osPtr->write(lineTab,1);
    }
    osPtr->write(value.c_str(),value.size());
    osPtr->write(lineTab,1);
}
