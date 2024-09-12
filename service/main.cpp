#include <iostream>
#include <pqxx/pqxx>
#include <ostream>
#include <fstream>

std::string res_dir_path = "../res/";
std::string filename = res_dir_path.append("config.txt");

int main() {
    std::ifstream conf(filename);
    std::string dbname, user, password;
    conf >> dbname >> user >> password;
    std::string cstring = "dbname=" + dbname + " user=" + user + " password=" + password + " host=localhost port=5501";
    try {
        pqxx::connection C(cstring);
        if (C.is_open()) {
            std::cout << "Opened database successfully: " << C.dbname() << std::endl;
        } else {
            std::cerr << "Connection failed" << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}