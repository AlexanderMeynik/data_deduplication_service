#ifndef SERVICE_LIB_H
#define SERVICE_LIB_H

#include "../common/myconcepts.h"
#include <fstream>
#include <pqxx/pqxx>
#include <functional>
namespace db_services {


    using index_type = long long;
    using trasnactionType = pqxx::work;

    static const char *const sample_temp_db = "template1";
    using conPtr = std::shared_ptr<pqxx::connection>;


    struct my_conn_string {
        my_conn_string():port(5432) {}

        template<to_str_to_c_str s1,
                to_str_to_c_str s2,
                to_str_to_c_str s3,
                to_str_to_c_str s4>
        my_conn_string(s1 &&user_, s2 &&password_, s3 &&host_, s4 &&dbname_, unsigned port_)
                : user(std::forward<s1>(user_)), password(std::forward<s2>(password_)),
                  host(std::forward<s3>(host_)),
                  dbname(std::forward<s4>(dbname_)),
                  port(port_) {
            update_format();
        }

        operator std::string() {
            return formatted_string;
        }

        [[nodiscard]] const char *c_str() const {
            return formatted_string.c_str();
        }

        template<to_str_to_c_str str>
        void set_user(str &&new_user) {
            user = std::forward<str>(new_user);
            update_format();
        }

        template<to_str_to_c_str str>
        void set_password(str &&new_password) {
            password = std::forward<str>(new_password);
            update_format();
        }

        template<to_str_to_c_str str>
        void set_host(str &&new_host) {
            host = std::forward<str>(new_host);
            update_format();
        }

        void set_port(unsigned new_port) {
            port = new_port;
            update_format();
        }

        template<to_str_to_c_str str>
        void set_dbname(str &&new_dbname) {
            dbname = std::forward<str>(new_dbname);
            update_format();
        }

        const std::string &getUser() const {
            return user;
        }

        const std::string &getPassword() const {
            return password;
        }

        const std::string &getHost() const {
            return host;
        }

        const std::string &getDbname() const {
            return dbname;
        }

        unsigned int getPort() const {
            return port;
        }


    private:
        void update_format() {
            formatted_string = vformat("postgresql://%s:%s@%s:%d/%s",
                                       user.c_str(), password.c_str(), host.c_str(), port, dbname.c_str());
        }

        std::string user, password, host, dbname;
        unsigned port;
        std::string formatted_string;
    };


    template<unsigned short verbose = 0, typename str>
    requires to_str_to_c_str<str>
    conPtr connect_if_possible(str &&cString);

    template<typename s1>
    requires std::is_convertible_v<s1, std::string>
    my_conn_string load_configuration(s1 &&filenam, unsigned port = 5501);

    template<typename T, unsigned long size>
    std::array <T, size> from_string(std::basic_string <T> &string) {
        std::array <T, size> res;
        for (int i = 0; i < size; ++i) {
            res[i] = string[i];
        }
        return res;
    }


    using namespace std::placeholders;

    std::string res_dir_path = "../res/";
    std::string cfile_name = res_dir_path.append("config.txt");


    auto default_configuration = [](unsigned int port = 5501) {
        return load_configuration(cfile_name, std::forward<decltype(port)>(port));
    };


    template<typename s1>
    requires std::is_convertible_v<s1, std::string>
    my_conn_string load_configuration(s1 &&filenam, unsigned port) {
        std::ifstream conf(filenam);
        std::string dbname1, user, password;
        conf >> dbname1 >> user >> password;
        auto res = my_conn_string(user, password, "localhost", dbname1, port);
        //res.update_format();
        return res;
    }
}



#endif //SERVICE_LIB_H
