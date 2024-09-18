#include <vector>
#include <iostream>
#include "FileUtils/ServiceFileInterface.h"
std::string res_dir_path = "../../res";
std::string res_dir_path2 = "../../res1";
int main() {
    std::string dbName="deduplication";
    FileParsingService<64> fs;

    fs.db_load<2,db_usage_strategy::create>(dbName);
    fs.process_directory<2>(res_dir_path);

    fs.load_directory<2>(res_dir_path,res_dir_path2);



}