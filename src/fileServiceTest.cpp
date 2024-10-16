#include <vector>
#include <iostream>
#include "FileService.h"

namespace fs = std::filesystem;
std::string parent_path = "../../testDirectories/";
std::string new_dir_prefix = "../../testDirectoriesRes/";
std::vector<fs::path> from_dirs = {"shakespear",
                                   "res",
                                   "res"};
std::vector<fs::path> to_dirs(from_dirs.size(), "");

int main(int argc, char *argv[]) {

    std::ios::sync_with_stdio(false);
    for (int i = 0; i < from_dirs.size(); i++) {
        to_dirs[i] = get_normal_abs((new_dir_prefix / from_dirs[i]));
        from_dirs[i] = get_normal_abs(parent_path / from_dirs[i]);
        auto res=db_services::to_spaced_path(to_dirs[i].string());
        auto res2=db_services::from_spaced_path(res);
        auto res3=db_services::to_tsquerable_path(res);
        auto res4=db_services::to_tsquerable_path(to_dirs[i].string());
        std::cout<<to_dirs[i].string()<<'\t'<<res<<'\t'<<res2<<'\t';
        std::cout<<res3<<'\t'<<res4<<'\n';
    }
    /*return 0;*/





    google::InitGoogleLogging(argv[0]);
    google::SetVLOGLevel("*", 3);
    FileParsingService<64> fs;
    std::string dbName = "deduplication12";
    clk.tik();

    fs.db_drop(dbName);

    fs.db_load<db_usage_strategy::create>(dbName);

    for (int i = 0; i < 2; ++i) {
        clk.tik();//test for simialr cases
        fs.process_directory(from_dirs[i].string());
        clk.tak();
        clk.tik();
        fs.load_directory<directory_handling_strategy::create_main>(from_dirs[i].string(), to_dirs[i].string());
        clk.tak();
        clk.tik();
        //fs.delete_directory(from_dirs[i].string());
        clk.tak();
    }
    clk.tak();
    clk.tik();
    //todo add file check data
    //fs.delete_directory(from_dirs[1].string());//works

  /*  fs.process_file("../../conf/config.txt");//fails due to path containing chracters
    fs.load_file<directory_handling_strategy::create_main>("../../conf/config.txt", "../../conf/c2.txt");*/
    //fs.delete_file("../../conf/config.txt");
    fs.clear_segments();
    /*fs.db_drop(dbName);*/
    clk.tak();
    std::cout << clk;

}