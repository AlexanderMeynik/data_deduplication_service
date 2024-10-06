#include <vector>
#include <iostream>
#include "ServiceFileInterface.h"

namespace fs = std::filesystem;
std::string parent_path = "../../testDirectories/";
std::string new_dir_prefix = "../../testDirectoriesRes/";
std::vector<fs::path> from_dirs = {"documentation",
                                   "test1",
                                   "res"};
std::vector<fs::path> to_dirs(from_dirs.size(), "");

int main(int argc, char *argv[]) {

    for (int i = 0; i < from_dirs.size(); i++) {
        to_dirs[i] = get_normal_abs((new_dir_prefix / from_dirs[i]));
        from_dirs[i] = get_normal_abs(parent_path / from_dirs[i]);
    }
    /*return 0;*/





    google::InitGoogleLogging(argv[0]);
    google::SetVLOGLevel("*", 3);//todo proper vlog handling from terminal

    FileParsingService<64> fs;
    std::string dbName = "deduplication10";
    clk.tik();

    fs.db_drop(dbName);

    fs.db_load<db_usage_strategy::create>(dbName);

    for (int i = 3; i < from_dirs.size(); ++i) {
        clk.tik();//test for simialr cases
        fs.process_directory(from_dirs[i].string());
        clk.tak();
        clk.tik();
        fs.load_directory<directory_handling_strategy::create_main,remove_>(from_dirs[i].string(), to_dirs[i].string());
        clk.tak();
    }
    clk.tak();
    clk.tik();
    //todo segment counts are handled improperly
    //todo add file check data
    //todo file check segments(check taht segemnst sums add up)
    //fs.delete_directory(from_dirs[1].string());//works


    fs.process_file("../../conf/config.txt");//fails due to path containing chracters
    fs.load_file<directory_handling_strategy::create_main>("../../conf/config.txt", "../../conf/c2.txt");
    fs.delete_file("../../conf/config.txt");
    /*fs.db_drop(dbName);*/
    clk.tak();
    std::cout << clk;

}