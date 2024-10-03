#ifndef SERVICE_LIB_H
#define SERVICE_LIB_H

#include "myconcepts.h"
#include "expected.hpp"
#include <fstream>
#include <pqxx/pqxx>
#include <functional>

namespace db_services {
    using namespace std::placeholders;
    #ifdef IT_test

    static std::string res_dir_path = "../../conf/";
    #else
    static std::string res_dir_path = "../../conf/";
    #endif

    static std::string cfile_name = res_dir_path.append("config.txt");
    static constexpr const char *const sqlLimitBreached_state = "23505";
    static constexpr const char *const sqlFqConstraight = "23503";


    using index_type = long long;
    using trasnactionType = pqxx::work;
    using connection_type = pqxx::connection;
    using conPtr = std::shared_ptr<connection_type>;
    using ResType = pqxx::result;
    using nonTransType = pqxx::nontransaction;

    enum delete_strategy {
        cascade,
        only_record
    };

    bool inline checkConnection(const conPtr &conn) {
        return conn && conn->is_open();
    }

    ResType terminate_all_db_connections(nonTransType &no_trans_exec, std::string_view db_name);

    ResType check_database_existence(nonTransType &no_trans_exec, std::string_view db_name);

    ResType check_schemas(trasnactionType &txn);

    ResType check_file_existence(trasnactionType &txn, std::string_view file_name);

    ResType check_directory_existence(trasnactionType &txn, std::string_view dir_path);

    static const char *const sample_temp_db = "template1";


    inline void printRows_affected(ResType &res) {
        VLOG(3) << vformat("Rows affected by latest request %d\n", res.affected_rows());
    }

    ResType inline get_table(trasnactionType& txn,std::string_view file_name)
    {
        std::string query=vformat("select LEFT(md5(\'%s\'),32)",file_name.data());
        return txn.exec(query);

    }
    std::string inline get_table_name(trasnactionType& txn,std::string_view file_name)
    {
        return get_table(txn,file_name).one_row()[0].as<std::string>();
    }


    ResType inline check_t_existence(db_services::trasnactionType &txn, std::string_view file_name) {
            auto hash_str=get_table_name(txn,file_name);
            std::string table_name=vformat("temp_file_%s",hash_str.c_str());
            //std::string t_name= get_table_name(txn,file_name);
            std::string query = "select 1 from pg_tables "
                                "where tablename=\'%s\' and schemaname='public';";
            auto r_q = vformat(query.c_str(), table_name.data(), table_name.c_str());
            return txn.exec(r_q);
    }



    struct my_conn_string {
        my_conn_string() : port(5432) {}

        my_conn_string(std::string_view user_,
                       std::string_view password_,
                       std::string_view host_,
                       std::string_view dbname_, unsigned port_)
                : user(user_), password(password_),
                  host(host_),
                  dbname(dbname_),
                  port(port_) {
            update_format();
        }

        operator std::string() {
            return formatted_string;
        }

        operator std::string_view() {
            return formatted_string.c_str();
        }

        [[nodiscard]] const char *c_str() const {
            return formatted_string.c_str();
        }

        void set_user(std::string_view new_user) {
            user = std::forward<std::string_view>(new_user);
            update_format();
        }

        void set_password(std::string_view new_password) {
            password = new_password;
            update_format();
        }

        void set_host(std::string_view new_host) {
            host = new_host;
            update_format();
        }

        void set_port(unsigned new_port) {
            port = new_port;
            update_format();
        }

        void set_dbname(std::string_view new_dbname) {
            dbname = new_dbname;
            update_format();
        }

        [[nodiscard]] const std::string &getUser() const {
            return user;
        }

        [[nodiscard]] const std::string &getPassword() const {
            return password;
        }

        [[nodiscard]] const std::string &getHost() const {
            return host;
        }

        [[nodiscard]] const std::string &getDbname() const {
            return dbname;
        }

        [[nodiscard]] unsigned int getPort() const {
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


    tl::expected<conPtr, return_codes> connect_if_possible(std::string_view cString);

    template<typename s1>
    requires std::is_convertible_v<s1, std::string>
    my_conn_string load_configuration(s1 &&filenam, unsigned port = 5501);




    auto default_configuration = [](unsigned int port = 5501) {
        //todo add verbose messages
        return load_configuration(cfile_name, std::forward<decltype(port)>(port));
    };

    template<typename T, unsigned long size>
    std::array<T, size> from_string(std::basic_string<T> &string) {
        std::array<T, size> res;
        for (int i = 0; i < size; ++i) {
            res[i] = string[i];
        }
        return res;
    }

#include <filesystem>



    template<typename s1>
    requires std::is_convertible_v<s1, std::string>
    my_conn_string load_configuration(s1 &&filenam, unsigned port) {
        std::ifstream conf(filenam);
        if (!conf.is_open()) {
            int count = 0;
            for (auto &entry: std::filesystem::recursive_directory_iterator(
                    std::filesystem::current_path().parent_path())) {
                count++;
                VLOG(1) << entry.path();
                if (count > 20) {
                    break;
                }
            }
            //return  {};
        }
        std::string dbname1, user, password;
        conf >> dbname1 >> user >> password;
        auto res = my_conn_string(user, password, "localhost", dbname1, port);
        //res.update_format();
        return res;
    }


}


#endif //SERVICE_LIB_H
