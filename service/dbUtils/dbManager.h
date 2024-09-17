#include <pqxx/pqxx>
#include <iostream>
#include "lib.h"
#include "../common/myconcepts.h"

#ifndef SERVICE_DBMANAGER_H
#define SERVICE_DBMANAGER_H

namespace db_services {
    //todo exec1 для существоания exec1
    //for_query() https://libpqxx.readthedocs.io/7.8.0/a01222.html#adae522da46299d4cd7c48128403e7c93

    template<unsigned long segment_size = 64> requires is_divisible<segment_size, SHA256size>
                                                       && is_divisible<total_block_size, segment_size>
    class dbManager {
    public:
        static constexpr unsigned long long block_size = total_block_size / segment_size;

        dbManager() {

        }

        template<typename ss>
        requires to_str_to_c_str<ss>
        explicit dbManager(ss &&cstring): cString_(std::forward<ss>(cstring)) {

        }

        template<verbose_level verbose = 0>
        conPtr connectToDb();


        //inpired by https://stackoverflow.com/questions/49122358/libbqxx-c-api-to-connect-to-postgresql-without-db-name
        template<verbose_level verbose = 0>
        void create();

        template<verbose_level verbose = 0>
        void fill_schemas();


        template<verbose_level verbose = 0>
        [[deprecated("function select process_file_data($1) to be deleted")]]
        void insert_bulk_segments(const segvec<segment_size> &segments, std::string &filename);

        template<verbose_level verbose = 0>
        [[deprecated("function select get_file_data($1) to be deleted")]] segvec<segment_size> get_file_segmented(std::string &filename);


        template<verbose_level verbose = 0, to_str_to_c_str str>
        index_type create_directory(str &&dir_path);

        template<verbose_level verbose = 0, to_str_to_c_str str>
        std::vector<std::string> get_all_files(str &&dir_path);

        template<verbose_level verbose = 0, to_str_to_c_str str>
        index_type create_file(str &&file_path, int dir_id, int file_size = 0);


        template<verbose_level verbose = 0, to_str_to_c_str str>
        void finish_file_processing(str &&file_path, index_type file_id);


        template<verbose_level verbose = 0, index_type block_sz = block_size, Index_size T, to_str_to_c_str str>
        void stream_segment_array(T &segment_block, str &&file_name, size_t block_index);

        template<verbose_level verbose = 0, index_type block_sz = block_size, to_str_to_c_str str, char fill = 23>
        int get_file_streamed(str &&file_name, std::ostream &out);


        template<verbose_level verbose = 0>
        std::string get_file_contents(std::string &file_name);

        bool checkConnection() {
            return conn_->is_open();
        }


    private:
        my_conn_string cString_;
        conPtr conn_;

    };

    template<unsigned long segment_size>
    requires is_divisible<segment_size, SHA256size>
             && is_divisible<total_block_size, segment_size>
    template<verbose_level verbose, index_type block_sz, to_str_to_c_str str, char fill>
    int dbManager<segment_size>::get_file_streamed(str &&file_name, std::ostream &out) {
        try {
            trasnactionType txn(*conn_);
            std::string query = vformat("select s.segment_data\n"
                                        "from data\n"
                                        "         inner join public.segments s on s.segment_hash = data.segment_hash\n"
                                        "        inner join public.files f on f.file_id = data.file_id\n"
                                        "where file_name=\'%s\'::tsvector\n"
                                        "order by segment_num", file_name.c_str());

            for (auto [name]: txn.stream<std::string>(query)) {
                for (char i: name) {
                    if (i == fill) {
                        goto end;
                    }
                    out.put(i);

                }
            }
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
    }
    
    
    template<unsigned long segment_size>
    requires is_divisible<segment_size, SHA256size>
             && is_divisible<total_block_size, segment_size>
    template<verbose_level verbose, index_type block_sz, Index_size T, to_str_to_c_str str>
    void dbManager<segment_size>::stream_segment_array(T &segment_block, str &&file_name, size_t block_index){
        try {
            trasnactionType txn(*conn_);
            std::string table_name = "\"temp_file_" + file_name + "\"";


            pqxx::stream_to copy_stream(txn, table_name);
            size_t index_start = block_index * block_sz;
            for (size_t i = 0; i < segment_block.size(); ++i) {
                copy_stream
                        << std::make_tuple(static_cast<int64_t>(index_start + i + 1),
                                           std::string(segment_block[i].data(), segment_size));
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

    template<unsigned long segment_size>
    requires is_divisible<segment_size, SHA256size>
             && is_divisible<total_block_size, segment_size>
    template<verbose_level verbose, to_str_to_c_str str>
    void dbManager<segment_size>::finish_file_processing(str &&file_path, index_type file_id) {
        try {
            trasnactionType txn(*conn_);
            std::string table_name = "\"temp_file_" + file_path + "\"";
            std::string table_name_ = "\'temp_file_" + file_path + "\'";


            std::string aggregation_table_name = "\"new_segments_" + file_path + "\"";

            std::string q = vformat("SELECT * FROM pg_tables WHERE tablename = %s", table_name_.c_str());
            auto r = txn.exec(q);
            if (r.empty()) {
                LOG_IF(WARNING, verbose >= 1) << vformat("No saved data found for file %s \n", file_path.c_str());
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

    template<unsigned long segment_size>
    requires is_divisible<segment_size, SHA256size>
             && is_divisible<total_block_size, segment_size>
    template<verbose_level verbose, to_str_to_c_str str>
    index_type dbManager<segment_size>::create_file(str &&file_path, int dir_id, int file_size)  {
        trasnactionType txn(*conn_);

        std::string query = "insert into public.files (file_name,dir_id,size_in_bytes)"
                            " values ($1,$2,$3) returning file_id;";


        pqxx::result res = txn.exec_params(query, file_path, dir_id, file_size);

        LOG_IF(INFO, verbose >= 2) << res.query() << '\n';


        std::string table_name = "temp_file_" + file_path;
        std::string q1 = vformat("CREATE TABLE \"%s\" (pos bigint, data char(%d));", txn.esc(table_name).c_str(),
                                 segment_size);
        pqxx::result r2 = txn.exec(q1);

        LOG_IF(INFO, verbose >= 2) << r2.query() << '\n';


        txn.commit();
        return res[0][0].as<index_type>();
    }

    template<unsigned long segment_size>
    requires is_divisible<segment_size, SHA256size>
             && is_divisible<total_block_size, segment_size>
    template<verbose_level verbose, to_str_to_c_str str>
    std::vector<std::string> dbManager<segment_size>::get_all_files(str &&dir_path) {
        //todo как лучше по пути или по имени
        std::vector<std::string> result;
        trasnactionType txn(*conn_);

        std::string query = "select file_name from files\n"
                            "    inner join public.directories d\n"
                            "        on d.dir_id = files.dir_id\n"
                            "where dir_path=$1;";
        pqxx::result res = txn.exec_params(query, dir_path);
        LOG_IF(INFO, verbose >= 2) << res.query() << '\n';
        for (const auto &re: res) {
            auto tres = re[0].as<std::string>();
            result.push_back(tres.substr(1, tres.size() - 2));
        }

        txn.commit();
        return result;
    }

    template<unsigned long segment_size>
    requires is_divisible<segment_size, SHA256size>
             && is_divisible<total_block_size, segment_size>
    template<verbose_level verbose, to_str_to_c_str str>
    index_type dbManager<segment_size>::create_directory(str &&dir_path) {
        trasnactionType txn(*conn_);
        std::string query = "insert into public.directories (dir_path) values ($1) returning dir_id;";
        pqxx::result res = txn.exec_params(query, dir_path);

        LOG_IF(INFO, verbose >= 2) << res.query() << '\n';

        txn.commit();
        return res[0][0].as<index_type>();
    }


    template<unsigned long segment_size>
    requires is_divisible<segment_size, SHA256size>
             && is_divisible<total_block_size, segment_size>
    template<verbose_level verbose>
    conPtr dbManager<segment_size>::connectToDb() {
        if (conn_ && conn_->is_open()) {
            return conn_;
        }
        LOG_IF(WARNING, verbose >= 1) << "Failed to listen with current connection, attempt to reconnect via cString\n";
        conn_ = connect_if_possible<verbose>(cString_);
        return conn_;
    }


    template<unsigned long segment_size>
    requires is_divisible<segment_size, SHA256size>
             && is_divisible<total_block_size, segment_size>
    template<verbose_level verbose>
    void dbManager<segment_size>::create() {
        conn_ = nullptr;
        auto tString = cString_;
        tString.set_dbname(sample_temp_db);
        auto temp_connection = connect_if_possible<verbose>(tString);

        if (!temp_connection || !temp_connection->is_open()) {
            LOG_IF(WARNING, verbose >= 1)
                            << vformat("Unable to connect by url \"%s\"\n", (tString).operator std::string().c_str());
            return;
        }
        LOG_IF(INFO, verbose >= 1) << vformat("Connected to database %s\n", tString.getDbname().c_str());


        pqxx::nontransaction no_trans_exec(*temp_connection);

        try {
            pqxx::result r = no_trans_exec.exec(vformat("SELECT 1 FROM pg_database WHERE datname = '%s';",
                                                        no_trans_exec.esc(cString_.getDbname()).c_str()));

            if (!r.empty()) {
                LOG_IF(WARNING, verbose >= 2)
                                << "Database already exists, skipping creation: " << cString_.getDbname() << '\n';
                no_trans_exec.abort();
            } else {
                no_trans_exec.exec(vformat("CREATE DATABASE %s;", no_trans_exec.esc(cString_.getDbname()).c_str()));
                no_trans_exec.exec(
                        vformat("GRANT ALL ON DATABASE %s TO %s;", no_trans_exec.esc(cString_.getDbname()).c_str(),
                                no_trans_exec.esc(cString_.getDbname()).c_str()));
                no_trans_exec.commit();

                LOG_IF(WARNING, verbose >= 2) << "Database created successfully: " << cString_.getDbname() << '\n';

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
    requires is_divisible<segment_size, SHA256size>
             && is_divisible<total_block_size, segment_size>
    template<verbose_level verbose>
    void dbManager<segment_size>::fill_schemas() {
        try {
            //todo check init
            trasnactionType txn(*conn_);

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
    requires is_divisible<segment_size, SHA256size>
             && is_divisible<total_block_size, segment_size>
    template<verbose_level verbose>
    void dbManager<segment_size>::insert_bulk_segments(const segvec<segment_size> &segments, std::string &filename) {
        try {
            trasnactionType txn(*conn_);
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
    requires is_divisible<segment_size, SHA256size>
             && is_divisible<total_block_size, segment_size>
    template<verbose_level verbose>
    segvec<segment_size> dbManager<segment_size>::get_file_segmented(std::string &filename) {
        segvec<segment_size> vector;
        try {
            trasnactionType txn(*conn_);
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
    requires is_divisible<segment_size, SHA256size>
             && is_divisible<total_block_size, segment_size>
    template<verbose_level verbose>
    std::string
    dbManager<segment_size>::get_file_contents(std::string &file_name) {
        try {
            trasnactionType txn(*conn_);
            std::string query = "select string_agg(ss.data,'')\n"
                                "from \n"
                                "    (\n"
                                "    select get_file_data($1) as data \n"
                                "    )as ss;";
            pqxx::result res = txn.exec_params(query, file_name);

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




    template<verbose_level verbose, typename str>
    requires to_str_to_c_str<str>
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
