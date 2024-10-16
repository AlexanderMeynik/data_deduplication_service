#ifndef DATA_DEDUPLICATION_SERVICE_DBCOMMON_H
#define DATA_DEDUPLICATION_SERVICE_DBCOMMON_H
#include <common.h>
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


    std::string to_spaced_path(std::string_view path);

    std::string from_spaced_path(std::string_view path);

    std::string to_tsquerable_path(std::string_view path);


    bool inline checkConnection(const conPtr &conn) {
        return conn && conn->is_open();
    }

    ResType terminate_all_db_connections(nonTransType &no_trans_exec, std::string_view db_name);

    ResType check_database_existence(nonTransType &no_trans_exec, std::string_view db_name);

    ResType check_schemas(trasnactionType &txn);

    index_type check_segment_count(trasnactionType &txn);

    ResType delete_unused_segments(trasnactionType &txn);

    ResType check_file_existence(trasnactionType &txn, std::string_view file_name);

    index_type get_file_id(trasnactionType &txn, std::string_view file_name);

    bool does_file_exist(trasnactionType &txn, std::string_view file_name);

    ResType get_files_for_directory(trasnactionType &txn, std::string_view dir_path);

    std::vector<index_type> get_file_id_vector(trasnactionType &txn, std::string_view dir_path);

    index_type inline get_total_file_size(trasnactionType &txn)
    {
        //todo check null
        return txn.query_value<index_type>("select sum(size_in_bytes) from files;");
    }
    ResType inline get_total_schema_sizes(trasnactionType &txn)
    {
        std::string qq="SELECT\n"
                         "    indexrelname, s.relname, "
                         "    pg_size_pretty(pg_relation_size(indexrelid)) AS index_size, "
                         "    pg_size_pretty(pg_table_size(s.relname::text)) as t_size, "
                         "    pg_size_pretty(pg_relation_size(s.relname::text)) as rel_size, "
                         "    pg_size_pretty(pg_total_relation_size(s.relname::text)) as t_rel_size "
                         "FROM "
                         "    pg_stat_user_indexes s/*,pg_stat_all_tables*/\n"
                         "WHERE "
                         "        schemaname = 'public' order by index_size desc;";

        return txn.exec(qq);
    }

    ResType inline getDedupCharacteristics(trasnactionType &txn,index_type  segment_size)
    {
        std::string query="with segments as( "
                          "select f.file_name,d.segment_hash ,count(d.segment_hash) from files f "
                          "        inner join public.data d on f.file_id = d.file_id "
                          "group by f.file_name,d.segment_hash "
                          "order by f.file_name) "
                          ", "
                          "unique_segments_count as( "
                          "select file_name,count(segment_hash) as unique_count  from segments "
                          "group by file_name) "
                          " "
                          " "
                          "select f.file_name,size_in_bytes,aa.unique_count, "
                          "       ceil(size_in_bytes::float8/$1 "
                          "       ) as segment_count,(aa.unique_count /ceil(size_in_bytes::float8/$1 "
                          "                          )::float8)*100 as unique_percenatage "
                          "from files f\n"
                          "inner join  unique_segments_count aa on f.file_name=aa.file_name;";
        return txn.exec(query,pqxx::params(segment_size));
    }

    void inline  print_table(ResType&rss,std::ostream &out)
    {
        int i = 0;
        for (; i < rss[0].size()-1; ++i) {
            out<<rss.column_name(i)<<'\t';
        }
        out<<rss.column_name(i)<<'\n';
        for (const auto & row:rss) {
            i=0;
            for ( ;i < row.size()-1; ++i) {
                out<<row[i].as<std::string>()<<'\t';
            }
            out<<row[i].as<std::string>()<<'\n';
        }
    }
    template<typename T>
    concept printable=requires(T&elem,std::ofstream &out){
        {out << elem}->std::same_as<std::ostream&>;
    };
    template<printable T>
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
        std::string query = vformat("select LEFT(md5(\'%s\'),32)", file_name.data());
        return txn.exec(query);

    }
    template<typename T>
    std::string inline convertToHex(const T& binaryResult)
    {
        std::ostringstream ss;//todo better way
        ss << std::hex << std::setfill('0');
        for (unsigned int i = 0; i < binaryResult.size(); ++i) {
            ss << std::setw(2) << static_cast<unsigned>(binaryResult.at(i));
        }

        return ss.str();
    }

    std::string inline get_hash_str(trasnactionType &txn, std::string_view file_name) {
        return get_hash_res(txn, file_name).one_row()[0].as<std::string>();
    }

    std::string inline get_hash_md5( std::string_view file_name) {
        std::basic_string<unsigned char>md(hash_function_size[MD_5],' ');
        funcs[MD_5](reinterpret_cast<const unsigned char *>(file_name.data()), file_name.size(),
                 md.data());

        return convertToHex(md);
        // return get_hash_res(txn, file_name).one_row()[0].as<std::string>();
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
