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
using namespace file_services;

int main(int argc, char *argv[]) {

    std::ios::sync_with_stdio(false);
    for (int i = 1; i < from_dirs.size(); i++) {
        to_dirs[i] = getNormalAbs((new_dir_prefix / from_dirs[i]));
        from_dirs[i] = getNormalAbs(parent_path / from_dirs[i]);
        auto res = db_services::toSpacedPath(to_dirs[i].string());
        auto res2 = db_services::fromSpacedPath(res);
        auto res3 = db_services::toTsquerablePath(res);
        auto res4 = db_services::toTsquerablePath(to_dirs[i].string());
        std::cout << to_dirs[i].string() << '\t' << res << '\t' << res2 << '\t';
        std::cout << res3 << '\t' << res4 << '\n';
    }
    /*return 0;*/





    google::InitGoogleLogging(argv[0]);
    google::SetVLOGLevel("*", 3);
    FileParsingService fs;
    std::string dbName = "deduplication12";
    gClk.tik();

    fs.dbDrop(dbName);

    fs.dbLoad<dbUsageStrategy::create>(dbName);

    for (int i = 1; i < 2; ++i) {
        gClk.tik();//test for simialr cases
        fs.processDirectory(from_dirs[i].string(),64);
        gClk.tak();
        gClk.tik();
        fs.loadDirectory<rootDirectoryHandlingStrategy::CreateMain>(from_dirs[i].string(), to_dirs[i].string());
        gClk.tak();
        gClk.tik();
        //fs.delete_directory(from_dirs[i].string());
        gClk.tak();
    }
    gClk.tak();
    gClk.tik();
    //todo add file check data
    //fs.delete_directory(from_dirs[1].string());//works

    /*  fs.process_file("../../conf/config.txt");//fails due to path containing chracters
      fs.load_file<directory_handling_strategy::create_main>("../../conf/config.txt", "../../conf/c2.txt");*/
    //fs.delete_file("../../conf/config.txt");
    fs.clearSegments();
    /*fs.db_drop(dbName);*/
    gClk.tak();
    std::cout << gClk;

}