#include <vector>
#include <iostream>
#include "ServiceFileInterface.h"


std::vector<std::string> from_dirs = {"../../../documentation", "../../../test1", "../../../res"};
std::vector<std::string> to_dirs = {from_dirs[0] + "_", from_dirs[1] + "_", from_dirs[2] + "_"};

int main(int argc, char *argv[]) {

    google::InitGoogleLogging(argv[0]);
    google::SetVLOGLevel("*",3);//todo proper vlog handling from terminal

    FileParsingService<64> fs;
    std::string dbName = "deduplication3";
    clk.tik();

    fs.db_load<db_usage_strategy::create>(dbName);

    for (int i =from_dirs.size()  ; i < from_dirs.size(); ++i) {
        clk.tik();//test for simialr cases
        fs.process_directory(from_dirs[i]);
        clk.tak();
        clk.tik();
        fs.load_directory<directory_handling_strategy::create_main>(from_dirs[i], to_dirs[i]);
        clk.tak();
        //fs.delete_directory(from_dirs[0]);
        //fs.delete_file(from_dirs[0]);
    }
    clk.tak();
    std::cout<<"\n\n\n"<<clk;
    //fs.delete_directory<2>(from_dirs[0]);//kinda slow find culprit

    fs.process_file("../build.ninja");
    fs.load_file<directory_handling_strategy::create_main>("../build.ninja", "ninja");
    fs.db_drop(dbName);

}