#include <vector>
#include <iostream>
#include "ServiceFileInterface.h"
namespace fs=std::filesystem;
std::string parent_path="../../testDirectories/";
std::string new_dir_prefix="../../testDirectoriesRes/";
std::vector<fs::path> from_dirs = {"documentation",
                                      "test1",
                                      "res"};
std::vector<fs::path> to_dirs(from_dirs.size(),"");

int main(int argc, char *argv[]) {

    for (int i=0;i<from_dirs.size();i++) {
        to_dirs[i]=new_dir_prefix/from_dirs[i];
        from_dirs[i]=parent_path/from_dirs[i];
    }


    google::InitGoogleLogging(argv[0]);
    google::SetVLOGLevel("*",3);//todo proper vlog handling from terminal

    FileParsingService<64> fs;
    std::string dbName = "deduplication4";
    clk.tik();

    fs.db_load<db_usage_strategy::create>(dbName);

    for (int i =0  ; i < from_dirs.size(); ++i) {
        clk.tik();//test for simialr cases
        fs.process_directory(from_dirs[i].string());
        clk.tak();
        clk.tik();
        fs.load_directory<directory_handling_strategy::create_main>(from_dirs[i].string(), to_dirs[i].string());
        clk.tak();
        //fs.delete_directory(from_dirs[0]);
        //fs.delete_file(from_dirs[0]);
    }
    clk.tak();
    clk.tik();

    //fs.delete_directory<2>(from_dirs[0]);//kinda slow find culprit

    fs.process_file("../build.ninja");
    fs.load_file<directory_handling_strategy::create_main>("../build.ninja", "ninja");
    fs.db_drop(dbName);
    clk.tak();
    std::cout<<clk;

}