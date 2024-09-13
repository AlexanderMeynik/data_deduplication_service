#include <iostream>
#include <pqxx/pqxx>
#include <ostream>
#include <fstream>
#include "dbUtils/lib.h"
std::string res_dir_path = "../res/";
std::string filename = res_dir_path.append("config.txt");

int main() {
    std::ifstream conf(filename);
    std::string dbname, user, password;
    conf >> dbname >> user >> password;
    MyCstring ccstring{user,password,"localhost",dbname,5501};
    std::string name;
    std::cin>>name;
    createDB<64>(ccstring,name);

    return 0;
}