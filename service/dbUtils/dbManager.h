#include <pqxx/pqxx>
#include <iostream>
#include <fstream>
#include "../common/myconcepts.h"
#include <functional>
#include <filesystem>

#ifndef SERVICE_DBMANAGER_H
#define SERVICE_DBMANAGER_H

namespace db_services {

    using index_type = long long;


    static const char *const sample_temp_db = "template1";
    using conPtr = std::shared_ptr<pqxx::connection>;

    struct MyCstring {

        operator std::string() {
            return formatted_string;
        }

        [[nodiscard]] const char *c_str() const {
            return formatted_string.c_str();
        }

        void update_format() {
            formatted_string = vformat("postgresql://%s:%s@%s:%d/%s",
                                       user.c_str(), password.c_str(), host.c_str(), port, dbname.c_str());
        }

        std::string user, password, host, dbname;
        unsigned port;
        std::string formatted_string;
    };


    template<unsigned short verbose = 0, typename str>
    requires is_twice_string_convertible<str>
    conPtr connect_if_possible(str &&cString);

    template<typename s1>
    requires std::is_convertible_v<s1, std::string>
    MyCstring getConConf(s1 &&filenam, unsigned port = 5501);

    template<typename T, unsigned long size>
    std::array<T, size> from_string(std::basic_string<T> &string) {
        std::array<T, size> res;
        for (int i = 0; i < size; ++i) {
            res[i] = string[i];
        }
        return res;
    }


    using namespace std::placeholders;

    std::string res_dir_path = "../res/";
    std::string filename = res_dir_path.append("config.txt");


    auto basic_configuration = [](unsigned int port = 5501) {
        return getConConf(filename, std::forward<decltype(port)>(port));//todo fina all shadow variables
    };


    template<unsigned long segment_size = 64> requires IsDiv<segment_size, SHA256size>
                                                       && IsDiv<blockXsegment, segment_size>
    class dbManager {
    public:
        static constexpr unsigned long long block_size = blockXsegment / segment_size;

        dbManager() {

        }

        template<typename ss>
        requires is_twice_string_convertible<ss>
        dbManager(ss &&cstring): cString_(std::forward<ss>(cstring)) {

        }

        template<unsigned short verbose = 0>
        conPtr connectToDb();


        //inpired by https://stackoverflow.com/questions/49122358/libbqxx-c-api-to-connect-to-postgresql-without-db-name
        template<unsigned short verbose = 0>
        void create();

        template<unsigned short verbose = 0>
        void fill_schemas();


        template<unsigned short verbose = 0>
        [[deprecated("function select process_file_data($1) to be deleted")]]
        void insert_bulk_segments(const segvec<segment_size> &segments, std::string &filename);

        template<unsigned short verbose = 0>
        [[deprecated("function select get_file_data($1) to be deleted")]] segvec<segment_size> get_file_segmented(std::string &filename);


        template<unsigned short verbose = 0, is_twice_string_convertible str>
        index_type create_directory(str &&dir_path) {
            pqxx::work txn(*conn_);
            std::string query = "insert into public.directories (dir_path) values ($1) returning dir_id;";
            pqxx::result res = txn.exec_params(query, dir_path);

            LOG_IF(INFO, verbose >= 2) << res.query() << '\n';

            txn.commit();
            return res[0][0].as<index_type>();
        }

        template<unsigned short verbose = 0, is_twice_string_convertible str>
        std::vector<std::string> get_all_files(str &&dirpath) {
            //todo как лучше по пути или по имени
            std::vector<std::string> result;
            pqxx::work txn(*conn_);

            std::string query = "select file_name from files\n"
                                "    inner join public.directories d\n"
                                "        on d.dir_id = files.dir_id\n"
                                "where dir_path=$1;";
            pqxx::result res = txn.exec_params(query, dirpath);
            LOG_IF(INFO, verbose >= 2) << res.query() << '\n';
            for (const auto &re: res) {
                auto tres = re[0].as<std::string>();
                result.push_back(tres.substr(1, tres.size() - 2));
            }

            txn.commit();
            return result;
        }

        template<unsigned short verbose = 0, is_twice_string_convertible str>
        index_type create_file(str &&path, int dir_id, int file_size = 0) {
            pqxx::work txn(*conn_);

            std::string query = "insert into public.files (file_name,dir_id,size_in_bytes)"
                                " values ($1,$2,$3) returning file_id;";


            pqxx::result res = txn.exec_params(query, path, dir_id, file_size);

            LOG_IF(INFO, verbose >= 2) << res.query() << '\n';


            std::string table_name = "temp_file_" + path;
            std::string q1 = vformat("CREATE TABLE \"%s\" (pos bigint, data char(%d));", txn.esc(table_name).c_str(),
                                     segment_size);
            pqxx::result r2 = txn.exec(q1);

            LOG_IF(INFO, verbose >= 2) << r2.query() << '\n';


            txn.commit();
            return res[0][0].as<index_type>();
        }


        template<unsigned short verbose = 0, is_twice_string_convertible str>
        void finish_file_processing(str &&name, index_type file_id) {
            try {
                pqxx::work txn(*conn_);
                std::string table_name = "\"temp_file_" + name + "\"";
                std::string table_name_ = "\'temp_file_" + name + "\'";


                std::string aggregation_table_name = "\"new_segments_" + name + "\"";

                std::string q = vformat("SELECT * FROM pg_tables WHERE tablename = %s", table_name_.c_str());
                auto r = txn.exec(q);
                if (r.empty()) {
                    LOG_IF(WARNING, verbose >= 1) << vformat("No saved data found for file %s \n", name.c_str());
                    return;
                }

                q = vformat("CREATE TABLE  %s AS SELECT DISTINCT t.data, COUNT(t.data) AS count"
                            " FROM %s t "
                            "GROUP BY t.data;", aggregation_table_name.c_str(), table_name.c_str());
                r = txn.exec(q);
                LOG_IF(INFO, verbose >= 2) << r.query() << '\n';


                q = vformat("INSERT INTO public.segments (segment_data, segment_count) "
                            "SELECT ns.data, ns.count "
                            "FROM %s ns "
                            "ON CONFLICT ON CONSTRAINT unique_data_constr "
                            "DO UPDATE"
                            " SET segment_count = public.segments.segment_count +  excluded.segment_count;",
                            aggregation_table_name.c_str());
                r = txn.exec(q);
                LOG_IF(INFO, verbose >= 2) << r.query() << '\n';

                q = vformat("drop table %s;", aggregation_table_name.c_str());
                r = txn.exec(q);
                LOG_IF(INFO, verbose >= 2) << r.query() << '\n';

                q = vformat("INSERT INTO public.data (segment_num, segment_hash, file_id) "
                            "SELECT pos, se.segment_hash,  %d "
                            "FROM  %s tt "
                            "INNER JOIN public.segments se "
                            "ON tt.data = se.segment_data;",
                            file_id,
                            table_name.c_str()
                );
                r = txn.exec(q);
                LOG_IF(INFO, verbose >= 2) << r.query() << '\n';


                q = vformat("DROP TABLE IF EXISTS  %s;", table_name.c_str());
                r = txn.exec(q);
                LOG_IF(INFO, verbose >= 2) << r.query() << '\n';

                txn.commit();
            } catch (const pqxx::sql_error &e) {
                LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what() << "\n"
                                            << "Query: " << e.query() << "\n"
                                            << "SQL State: " << e.sqlstate() << '\n';
            } catch (const std::exception &e) {
                LOG_IF(ERROR, verbose >= 1) << "Error processing file data:" << e.what() << '\n';
            }
        }


        template<unsigned short verbose = 0, index_type block_sz = block_size, Index_size T, is_twice_string_convertible str>
        void stream_segment_array(T &segmentBlock, str &&fileName, size_t block_index) {
            try {
                pqxx::work txn(*conn_);
                std::string table_name = "\"temp_file_" + fileName + "\"";


                pqxx::stream_to copy_stream(txn, table_name);
                size_t index_start = block_index * block_sz;
                for (size_t i = 0; i < segmentBlock.size(); ++i) {
                    copy_stream
                            << std::make_tuple(static_cast<int64_t>(index_start + i + 1),
                                               std::string(segmentBlock[i].data(), segment_size));
                }

                copy_stream.complete();
                txn.commit();
            } catch (const pqxx::sql_error &e) {
                LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what() << "\n"
                                            << "Query: " << e.query() << "\n"
                                            << "SQL State: " << e.sqlstate() << '\n';
            } catch (const std::exception &e) {
                LOG_IF(ERROR, verbose >= 1) << "Error inserting block data:" << e.what() << '\n';
            }
        }

        template<unsigned short verbose = 0, index_type block_sz = block_size, is_twice_string_convertible str, char fill = 23>
        int get_file_streamed(str &&fileName, std::ostream &out) {
            try {
                pqxx::work txn(*conn_);
                std::string query = vformat("select s.segment_data\n"
                                            "from data\n"
                                            "         inner join public.segments s on s.segment_hash = data.segment_hash\n"
                                            "        inner join public.files f on f.file_id = data.file_id\n"
                                            "where file_name=\'%s\'::tsvector\n"
                                            "order by segment_num", fileName.c_str());

                for (auto [name]: txn.stream<std::string>(query)) {
                    for (char i: name) {
                        if (i == fill) {
                            goto end;
                        }
                        out.put(i);

                    }
                }

                /*auto stream= pqxx::stream_from::query(txn,query);
                std::tuple<std::string> row;
                while (stream>>row) {
                    std::string name=get<0>(row);
                    for (char i : name) {
                        if(i==fill)
                        {
                            goto end;
                        }
                        out.put(i);

                    }
                }
                end:stream.complete();*/

                end:
                txn.commit();


            } catch (const pqxx::sql_error &e) {
                LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what() << "\n"
                                            << "Query: " << e.query() << "\n"
                                            << "SQL State: " << e.sqlstate() << '\n';
                return -1;
            } catch (const std::exception &e) {
                LOG_IF(ERROR, verbose >= 1) << "Error inserting block data:" << e.what() << '\n';
                return -1;
            }
            return 0;
        };


        template<unsigned short verbose = 0>
        std::string get_file_contents(std::string &filename);

        bool checkConnection() {
            return conn_->is_open();
        }


    private:
        MyCstring cString_;
        conPtr conn_;

    };


    template<unsigned long segment_size>
    requires IsDiv<segment_size, SHA256size>
             && IsDiv<blockXsegment, segment_size>
    template<unsigned short verbose>
    conPtr dbManager<segment_size>::connectToDb() {
        if (conn_ && conn_->is_open()) {
            return conn_;
        }
        LOG_IF(WARNING, verbose >= 1) << "Failed to listen with current connection, attempt to reconnect via cString\n";
        conn_ = connect_if_possible<verbose>(cString_);
        return conn_;
    }


    template<unsigned long segment_size>
    requires IsDiv<segment_size, SHA256size>
             && IsDiv<blockXsegment, segment_size>
    template<unsigned short verbose>
    void dbManager<segment_size>::create() {
        conn_ = nullptr;
        auto tString = cString_;
        tString.dbname = sample_temp_db;
        tString.update_format();
        auto temp_connection = connect_if_possible<verbose>(tString);

        if (!temp_connection || !temp_connection->is_open()) {
            LOG_IF(WARNING, verbose >= 1)
                            << vformat("Unable to connect by url \"%s\"\n", (tString).operator std::string().c_str());
            return;
        }
        LOG_IF(INFO, verbose >= 1) << vformat("Connected to database %s\n", tString.dbname.c_str());


        pqxx::nontransaction no_trans_exec(*temp_connection);

        try {
            pqxx::result r = no_trans_exec.exec(vformat("SELECT 1 FROM pg_database WHERE datname = '%s';",
                                                        no_trans_exec.esc(cString_.dbname).c_str()));

            if (!r.empty()) {
                LOG_IF(WARNING, verbose >= 2)
                                << "Database already exists, skipping creation: " << cString_.dbname << '\n';
                no_trans_exec.abort();
            } else {
                no_trans_exec.exec(vformat("CREATE DATABASE %s;", no_trans_exec.esc(cString_.dbname).c_str()));
                no_trans_exec.exec(
                        vformat("GRANT ALL ON DATABASE %s TO %s;", no_trans_exec.esc(cString_.dbname).c_str(),
                                no_trans_exec.esc(cString_.user).c_str()));
                no_trans_exec.commit();

                LOG_IF(WARNING, verbose >= 2) << "Database created successfully: " << cString_.dbname << '\n';

            }


        } catch (const pqxx::sql_error &e) {
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what() << "\n"
                                        << "Query: " << e.query() << "\n"
                                        << "SQL State: " << e.sqlstate() << '\n';
            return;
        } catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error: " << e.what() << '\n';

            return;
        }


        temp_connection->close();
        conn_ = connect_if_possible<verbose>(cString_);
    }


    template<unsigned long segment_size>
    requires IsDiv<segment_size, SHA256size>
             && IsDiv<blockXsegment, segment_size>
    template<unsigned short verbose>
    void dbManager<segment_size>::fill_schemas() {
        try {
            //todo check init
            pqxx::work txn(*conn_);

            std::string query = "create table public.segments\n"
                                "(\n"
                                "    segment_hash bytea NOT NULL GENERATED ALWAYS AS (sha256(segment_data::bytea)) STORED primary key,\n"
                                "    segment_data char(%d) NOT NULL,\n"
                                "    segment_count bigint NOT NULL\n"
                                ");";

            std::string q1 = vformat(query.c_str(), segment_size);
            pqxx::result r1 = txn.exec(q1);

            LOG_IF(INFO, verbose >= 2) << "Create segments table " << r1.query() << '\n';


            query = "create table public.directories\n"
                    "(\n"
                    "    dir_id serial primary key NOT NULL,\n"
                    "    dir_path tsvector NOT NULL\n"
                    ");\n"
                    "\n"
                    "create table public.files\n"
                    "(\n"
                    "    file_id serial primary key NOT NULL,\n"
                    "    file_name tsvector NOT NULL,\n"
                    "    dir_id int REFERENCES public.directories(dir_id) NULL,\n"
                    "    size_in_bytes bigint NULL\n"
                    ");\n"
                    "\n"
                    "create table public.data\n"
                    "(\n"
                    "    file_id int REFERENCES public.files(file_id) NOT NULL,\n"
                    "    segment_num bigint NOT NULL,\n"
                    "    segment_hash bytea REFERENCES public.segments(segment_hash) NOT NULL\n"
                    ");"
                    ""
                    "CREATE INDEX hash_segment_hash on segments using Hash(segment_hash);\n"
                    "CREATE INDEX hash_segment_data on segments using Hash(segment_data);\n"
                    "\n"
                    "CREATE INDEX dir_id_hash on files using Hash(dir_id);\n"
                    "\n"
                    "CREATE INDEX dir_gin_index on directories using gin(dir_path);\n"
                    "CREATE INDEX files_gin_index on files using gin(file_name);\n"
                    "\n"
                    "\n"
                    "ALTER TABLE segments ADD CONSTRAINT unique_data_constr UNIQUE(segment_data);\n"
                    "ALTER TABLE files ADD CONSTRAINT unique_file_constr UNIQUE(file_name);";

            r1 = txn.exec(query);
            LOG_IF(INFO, verbose >= 2) << "Create other tables " << r1.query() << '\n';

            /*query = "CREATE OR REPLACE FUNCTION process_file_data(file_name text)\n"
                    "    RETURNS int\n"
                    "    LANGUAGE plpgsql\n"
                    "AS $$\n"
                    "DECLARE\n"
                    "    file_id_ bigint;\n"
                    "    table_name text := 'temp_file_' || file_name;\n"
                    "    aggregation_table_name text:='new_segments'|| file_name;\n"
                    "    query text;\n"
                    "BEGIN\n"
                    "    IF NOT EXISTS (SELECT * FROM pg_tables WHERE tablename = table_name)\n"
                    "    THEN\n"
                    "        return -1;\n"
                    "    end if;\n"
                    "\n"
                    "    insert into public.files (file_name) values (file_name::tsvector) returning file_id into file_id_;\n"
                    "    query := 'CREATE TABLE ' || quote_ident(aggregation_table_name) ||\n"
                    "             ' AS SELECT DISTINCT t.data, COUNT(t.data) AS count\n"
                    "             FROM '||quote_ident(table_name) ||' t ' ||\n"
                    "             'GROUP BY t.data;';\n"
                    "\n"
                    "    EXECUTE query;\n"
                    "    query := 'INSERT INTO public.segments (segment_data, segment_count)\n"
                    "              SELECT ns.data, ns.count\n"
                    "              FROM ' || quote_ident(aggregation_table_name) || ' ns\n"
                    "              ON CONFLICT ON CONSTRAINT unique_data_constr\n"
                    "              DO UPDATE\n"
                    "                SET segment_count = public.segments.segment_count +  excluded.segment_count;';\n"
                    "\n"
                    "    EXECUTE query;\n"
                    "\n"
                    "    EXECUTE 'drop table ' || quote_ident(aggregation_table_name) || ';';\n"
                    "\n"
                    "    query := 'INSERT INTO public.data (segment_num, segment_hash, file_id)\n"
                    "              SELECT pos, se.segment_hash, ' || file_id_ || '\n"
                    "              FROM ' || quote_ident(table_name) || '\n"
                    "              INNER JOIN public.segments se\n"
                    "              ON ' || quote_ident(table_name) || '.data = se.segment_data';\n"
                    "\n"
                    "    EXECUTE query;\n"
                    "\n"
                    "    EXECUTE 'DROP TABLE IF EXISTS ' || quote_ident(table_name);\n"
                    "    return 0;\n"
                    "END $$;";

            r1 = txn.exec(query);

            LOG_IF(INFO, verbose >= 2) << "Create  functions and procedures " << r1.query() << '\n';


            query = "CREATE OR REPLACE FUNCTION get_file_data(fileName text)\n"
                    "    returns table\n"
                    "            (\n"
                    "                data char(%d)\n"
                    "            )\n"
                    "    LANGUAGE plpgsql\n"
                    "AS $$\n"
                    "BEGIN\n"
                    "    return query\n"
                    "        select s.segment_data\n"
                    "        from data\n"
                    "                 inner join public.segments s on s.segment_hash = data.segment_hash\n"
                    "        where data.file_id=(select files.file_id from files where files.file_name=fileName::tsvector)\n"
                    "        order by segment_num;\n"
                    "\n"
                    "END $$;";
            q1 = vformat(txn.esc(query).c_str(), segment_size);
            r1 = txn.exec(q1);


            LOG_IF(INFO, verbose >= 2) << "Create function get_file_contents " << r1.query() << '\n';*/


            txn.commit();

        } catch (const pqxx::sql_error &e) {
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what() << "\n"
                                        << "Query: " << e.query() << "\n"
                                        << "SQL State: " << e.sqlstate() << '\n';
        } catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error setting up database schemas: " << e.what() << '\n';
        }
    }


    template<unsigned long segment_size>
    requires IsDiv<segment_size, SHA256size>
             && IsDiv<blockXsegment, segment_size>
    template<unsigned short verbose>
    void dbManager<segment_size>::insert_bulk_segments(const segvec<segment_size> &segments, std::string &filename) {
        try {
            pqxx::work txn(*conn_);
            std::string table_name = "temp_file_" + filename;
            std::string q1 = vformat("CREATE TABLE %s (pos bigint, data char(%d));", txn.esc(table_name).c_str(),
                                     segment_size);
            pqxx::result r2 = txn.exec(q1);

            LOG_IF(INFO, verbose >= 2) << r2.query() << '\n';


            pqxx::stream_to copy_stream(txn, table_name);

            for (size_t i = 0; i < segments.size(); ++i) {
                copy_stream
                        << std::make_tuple(static_cast<int64_t>(i + 1), std::string(segments[i].data(), segment_size));
            }

            copy_stream.complete();
            std::string query = "select process_file_data($1)";
            pqxx::result res = txn.exec_params(query, filename);

            LOG_IF(INFO, verbose >= 2) << res.query() << '\n';

            txn.commit();
        } catch (const pqxx::sql_error &e) {
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what() << "\n"
                                        << "Query: " << e.query() << "\n"
                                        << "SQL State: " << e.sqlstate() << '\n';
        } catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error inserting bulk data:" << e.what() << '\n';
        }
    }


    template<unsigned long segment_size>
    requires IsDiv<segment_size, SHA256size>
             && IsDiv<blockXsegment, segment_size>
    template<unsigned short verbose>
    segvec<segment_size> dbManager<segment_size>::get_file_segmented(std::string &filename) {
        segvec<segment_size> vector;
        try {
            pqxx::work txn(*conn_);
            std::string query = "select get_file_data($1)";
            pqxx::result res = txn.exec_params(query, filename);

            LOG_IF(INFO, verbose >= 2) << res.query() << '\n';

            txn.commit();

            for (const auto &re: res) {
                auto string = re[0].as<std::string>();
                vector.push_back(from_string<char, 64>(string));
            }

        } catch (const pqxx::sql_error &e) {
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what() << "\n"
                                        << "Query: " << e.query() << "\n"
                                        << "SQL State: " << e.sqlstate() << '\n';
        } catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error get_file_segments: " << e.what() << '\n';
        }
        return vector;
    }

    template<unsigned long segment_size>
    requires IsDiv<segment_size, SHA256size>
             && IsDiv<blockXsegment, segment_size>
    template<unsigned short verbose>
    std::string
    dbManager<segment_size>::get_file_contents(std::string &filename) {
        try {
            pqxx::work txn(*conn_);
            std::string query = "select string_agg(ss.data,'')\n"
                                "from \n"
                                "    (\n"
                                "    select get_file_data($1) as data \n"
                                "    )as ss;";
            pqxx::result res = txn.exec_params(query, filename);

            LOG_IF(INFO, verbose >= 2) << res.query() << '\n';

            txn.commit();
            return res[0][0].as<std::string>();

        } catch (const pqxx::sql_error &e) {
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what() << "\n"
                                        << "Query: " << e.query() << "\n"
                                        << "SQL State: " << e.sqlstate() << '\n';
        } catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error gett_file_contents: " << e.what() << '\n';
        }
        return "";
    }


    template<typename s1>
    requires std::is_convertible_v<s1, std::string>
    MyCstring getConConf(s1 &&filenam, unsigned port) {
        std::ifstream conf(filenam);
        std::string dbname1, user, password;
        conf >> dbname1 >> user >> password;
        auto res = MyCstring{user, password, "localhost", dbname1, port};
        res.update_format();
        return res;
    }


    template<unsigned short verbose, typename str>
    requires is_twice_string_convertible<str>
    conPtr connect_if_possible(str &&cString) {
        conPtr c;
        std::string css = cString;
        try {
            c = std::make_shared<pqxx::connection>(css);
            if (!c->is_open()) {

                LOG_IF(ERROR, verbose >= 1) << vformat("Unable to connect by url \"%s\"\n", cString.c_str());
            } else {
                LOG_IF(INFO, verbose >= 2) << "Opened database successfully: " << c->dbname() << '\n';
            }
        } catch (const pqxx::sql_error &e) {
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what() << "\n"
                                        << "Query: " << e.query() << "\n"
                                        << "SQL State: " << e.sqlstate() << '\n';
        } catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error: " << e.what() << '\n';
        }
        return c;
    }

}


#endif
