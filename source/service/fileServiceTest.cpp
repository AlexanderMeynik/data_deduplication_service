#include <vector>
#include <iostream>
#include "FileUtils/ServiceFileInterface.h"


std::vector<std::string> from_dirs = {"../../../documentation", "../../../test1", "../../../res"};
std::vector<std::string> to_dirs = {from_dirs[0] + "_", from_dirs[1] + "_", from_dirs[2] + "_"};

int main(int argc, char *argv[]) {

    google::InitGoogleLogging(argv[0]);
    google::SetVLOGLevel("*",3);//todo proper vlog handling from terminal

    FileParsingService<64> fs;
    std::string dbName = "deduplication5";
    MEASURE_TIME(
    fs.db_load<db_usage_strategy::create>(dbName);

    for (int i = 0  ; i < from_dirs.size(); ++i) {
        fs.process_directory<data_insetion_strategy::replace_with_new>(from_dirs[i]);
        fs.load_directory<directory_handling_strategy::create_main COMMA data_retrieval_strategy::remove_>(from_dirs[i], to_dirs[i]);
        //fs.delete_directory(from_dirs[0]);
        //fs.delete_file(from_dirs[0]);
    }
    )
    std::cout<<"\n\n\n"<<clk;
    //fs.delete_directory<2>(from_dirs[0]);//kinda slow find culprit

    fs.process_file("cmake_install.cmake");
    fs.load_file<directory_handling_strategy::create_main,remove_>("cmake_install.cmake", "../../clone/cmake_install.cmake");


}