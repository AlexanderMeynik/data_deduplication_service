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
        auto res=db_services::to_dotted_path(to_dirs[i].string());
        auto res2=db_services::from_dotted_path(res);
        std::cout<<to_dirs[i].string()<<'\t'<<res<<'\t'<<res2<<'\n';
    }
    /*return 0;*/





    google::InitGoogleLogging(argv[0]);
    google::SetVLOGLevel("*", 3);//todo proper vlog handling from terminal

    FileParsingService<64> fs;
    std::string dbName = "deduplication10";
    clk.tik();

    fs.db_drop(dbName);

    fs.db_load<db_usage_strategy::create>(dbName);

    for (int i = 1; i < from_dirs.size(); ++i) {
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
    fs.delete_directory(from_dirs[1].string());//works

    //fs.delete_directory<2>(from_dirs[0]);//kinda slow find culprit

    fs.process_file("../../conf/config.txt");//fails due to path containing chracters
    fs.load_file<directory_handling_strategy::create_main>("../../conf/config.txt", "../../conf/c2.txt");
    /*fs.db_drop(dbName);*/
    clk.tak();
    std::cout << clk;

}