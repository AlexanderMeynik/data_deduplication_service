#ifndef DATA_DEDUPLICATION_SERVICE_DBCOMMON_H
#define DATA_DEDUPLICATION_SERVICE_DBCOMMON_H
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


    std::string inline to_spaced_path(std::string_view path)
    {
        std::string res(path.size(),'\0');
        std::replace_copy_if(path.begin(), path.end(),res.begin(),
                             [](auto n){ return n=='/'; }, ' ');
        return res.substr(1);
    }

    std::string inline from_spaced_path(std::string_view path)
    {
        std::string res(path.size()+1,'/');
        std::replace_copy_if(path.begin(), path.end(),res.begin()+1,
                             [](auto n){ return n==' '; }, '/');
        return res;
    }

    std::string inline to_tsquerable_path(std::string_view path)
    {
        std::string res(path.size(),'\0');
        std::transform(path.begin(), path.end(), res.begin(),
                       [](char c){
                           switch (c) {
                               case ' ':
                               case '/':
                                   return '&';
                               case '_':
                                   return '/';
                               default:
                                   return (char)std::tolower(c);
                           } });
        if(res[0]=='&')
            res= res.substr(1);

        //todo test to spaced path + this=this
        return res;
    }


    bool inline checkConnection(const conPtr &conn) {
        return conn && conn->is_open();
    }

    ResType terminate_all_db_connections(nonTransType &no_trans_exec, std::string_view db_name);

    ResType check_database_existence(nonTransType &no_trans_exec, std::string_view db_name);

    ResType check_schemas(trasnactionType &txn);

    ResType check_file_existence(trasnactionType &txn, std::string_view file_name);

    index_type get_file_id(trasnactionType &txn, std::string_view file_name);

    index_type does_file_exist(trasnactionType &txn, std::string_view file_name);

    ResType get_files_for_directory(trasnactionType &txn, std::string_view dir_path);//todo  delete

    std::vector<index_type> get_file_id_vector(trasnactionType &txn, std::string_view dir_path);


    template<typename T>
    concept print=requires(T&elem,std::ofstream &out){
        {out << elem}->std::same_as<std::ostream&>;
    };
    template<print T>
    std::string vec_to_string(std::vector<T>&vec)
    {
        std::stringstream ss;
        if(vec.empty())
        {
            return ss.str();
        }
        int i = 0;
        for (; i < vec.size()-1; ++i) {
            ss<<vec[i]<<',';
        }
        ss<<vec[i];
        return ss.str();
    }

    ResType check_files_existence(trasnactionType &txn, std::vector<std::filesystem::path> &files);


    static const char *const sample_temp_db = "template1";


    inline void printRows_affected(ResType &res) {
        VLOG(3) << vformat("Rows affected by latest request %d\n", res.affected_rows());
    }
    inline void printRows_affected(ResType &&res) {
        printRows_affected(res);
    }

    ResType inline get_hash_res(trasnactionType &txn, std::string_view file_name) {
        //todo maybe we can generate it with openssl
        std::string query = vformat("select LEFT(md5(\'%s\'),32)", file_name.data());
        return txn.exec(query);

    }

    std::string inline get_hash_str(trasnactionType &txn, std::string_view file_name) {
        return get_hash_res(txn, file_name).one_row()[0].as<std::string>();
    }


    ResType inline check_t_existence(db_services::trasnactionType &txn, std::string_view file_name) {
        auto hash_str = get_hash_str(txn, file_name);
        std::string table_name = vformat("temp_file_%s", hash_str.c_str());
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

        explicit operator std::string() {
            return formatted_string;
        }

        operator std::string_view() {
            return formatted_string;
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

    my_conn_string load_configuration(std::string_view filename);


    auto default_configuration = []() {
        //todo add verbose messages
        return load_configuration(cfile_name);
    };

    template<typename T, unsigned long size>
    std::array<T, size> from_string(std::basic_string<T> &string) {
        std::array<T, size> res;
        for (int i = 0; i < size; ++i) {
            res[i] = string[i];
        }
        return res;
    }


}


#endif //DATA_DEDUPLICATION_SERVICE_DBCOMMON_H