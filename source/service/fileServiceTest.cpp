#include <vector>
#include <iostream>
#include "FileUtils/ServiceFileInterface.h"


std::vector<std::string> from_dirs = {"../../../documentation", "../../../test1", "../../../res"};
std::vector<std::string> to_dirs = {from_dirs[0] + "_", from_dirs[1] + "_", from_dirs[2] + "_"};

int main() {
    FileParsingService<64,hash_function::MD_5> fs;
    std::string dbName = "deduplication3";
    fs.db_load<2, db_usage_strategy::create>(dbName);

    for (int i = 0; i < from_dirs.size(); ++i) {

        fs.process_directory<2, data_insetion_strategy::replace_with_new>(from_dirs[i]);
        fs.load_directory<2, directory_handling_strategy::create_main>(from_dirs[i], to_dirs[i]);
    }

    //fs.load_directory<2,directory_handling_strategy::create_main>(from_dirs[i], to_dirs[i]);






    /* fs.load_directory<2,directory_handling_strategy::create_main,data_retrieval_strategy::remove_>(res_dir_path, res_dir_path2);*/


}