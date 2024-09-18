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
        index_type create_directory(std::string_view dir_path);

        template<verbose_level verbose = 0>
        std::vector<std::string> get_all_files(std::string_view dir_path);

        template<verbose_level verbose = 0>
        index_type create_file(std::string_view file_path, int dir_id, int file_size = 0);


        template<verbose_level verbose = 0>
        void finish_file_processing(std::string_view file_path, index_type file_id);


        template<verbose_level verbose = 0, index_type block_sz = block_size, Index_size T>
        void stream_segment_array(T &segment_block, std::string_view file_name, size_t block_index);

        template<verbose_level verbose = 0, index_type block_sz = block_size, char fill = 23>
        int get_file_streamed(std::string_view file_name, std::ostream &out);


        template<verbose_level verbose = 0>
        [[deprecated]]std::string get_file_contents(std::string &file_name);

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
    template<verbose_level verbose, index_type block_sz, char fill>
    int dbManager<segment_size>::get_file_streamed(std::string_view file_name, std::ostream &out) {
        try {
            trasnactionType txn(*conn_);
            std::string query = vformat("select s.segment_data\n"
                                        "from data\n"
                                        "         inner join public.segments s on s.segment_hash = data.segment_hash\n"
                                        "        inner join public.files f on f.file_id = data.file_id\n"
                                        "where file_name=\'%s\'::tsvector\n"
                                        "order by segment_num", file_name.data());

            for (auto [name]: txn.stream<pqxx::binarystring>(query)) {
                auto str=name.str();
                auto decoded=/*pg_from_hex*/(str);
                for (char i: decoded) {
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
    template<verbose_level verbose, index_type block_sz, Index_size T>
    void
    dbManager<segment_size>::stream_segment_array(T &segment_block, std::string_view file_name, size_t block_index) {
        try {
            trasnactionType txn(*conn_);
            std::string table_name = vformat("\"temp_file_%s\"", file_name.data());

            /*std::string insert_query= vformat("insert into %s ( pos , data ) values \n",table_name.c_str());
            std::stringstream strem_to;
            strem_to<<insert_query;
            size_t i = 0;
            for (; i < segment_block.size()-1; ++i) {
                auto sssss=pg_from_hex(segment_block[i].data());
                strem_to<<vformat("( %d , encode(E\'%s\',\'escape\')),\n",i+1,sssss.c_str());
            }
            auto sssss=(pg_from_hex(segment_block[i].data()));
            strem_to<<vformat("( %d , encode(E\'%s\',\'escape\'));\n",i+1,sssss.c_str());
            auto res_query=strem_to.str();
            auto res=txn.exec(res_query);*/

            pqxx::stream_to copy_stream = pqxx::stream_to::raw_table(txn, table_name);
            size_t index_start = block_index * block_sz;
            for (size_t i = 0; i < segment_block.size(); ++i) {
                //auto rres=/*pg_to_hex*/(segment_block[i].data());
                std::string temp(segment_block[i].begin(),segment_block[i].end());
                pqxx::binarystring bn(temp);
                copy_stream
                        << std::make_tuple<int64_t,pqxx::binarystring>(static_cast<int64_t>(index_start + i + 1),
                                                                       std::move(bn));
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
    template<verbose_level verbose>
    void dbManager<segment_size>::finish_file_processing(std::string_view file_path, index_type file_id) {
        try {
            trasnactionType txn(*conn_);

            std::string table_name = vformat("\"temp_file_%s\"", file_path.data());
            std::string table_name_ = vformat("\'temp_file_%s\'", file_path.data());


            std::string aggregation_table_name = vformat("\"new_segments_%s\"", file_path.data());
            pqxx::result r;
            std::string q = vformat("SELECT * FROM pg_tables WHERE tablename = %s", table_name_.c_str());
            txn.exec(q).one_row();
            /*if (r.empty()) {
                LOG_IF(WARNING, verbose >= 1) << vformat("No saved data found for file %s \n", file_path.data());
                return;
            }*/

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

    static constexpr const char *const sqlLimitBreached_state = "23505";

    template<unsigned long segment_size>
    requires is_divisible<segment_size, SHA256size>
             && is_divisible<total_block_size, segment_size>
    template<verbose_level verbose>
    index_type dbManager<segment_size>::create_file(std::string_view file_path, int dir_id, int file_size) {
        trasnactionType txn(*conn_);
        index_type rrr = -1;

        std::string query = "insert into public.files (file_name,dir_id,size_in_bytes)"
                            " values ($1,$2,$3) returning file_id;";

        try {
            pqxx::result res = txn.exec(query, {file_path, dir_id, file_size});
            rrr = res.one_row()[0].as<index_type>();

            LOG_IF(INFO, verbose >= 2) << res.query() << '\n';

            std::string table_name = vformat("temp_file_%s", file_path.data());
            std::string q1 = vformat(
                    "CREATE TABLE \"%s\" (pos bigint, data bytea);",
                    txn.esc(table_name).c_str()
                    //,segment_size//todo check comma
            );
            pqxx::result r2 = txn.exec(q1);
            txn.commit();

            LOG_IF(INFO, verbose >= 2) << r2.query() << '\n';
        }
        catch (const pqxx::sql_error &e) {
            if(e.sqlstate() == sqlLimitBreached_state)
            {
                LOG_IF(ERROR, verbose >= 1)<<vformat("File %s already exists\n",file_path.data());
                return -1;
            }
            //sql state 23505= limitation violation
            //sql statte 42P01=: no such raktion
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what() << "\n"
                                        << "Query: " << e.query() << "\n"
                                        << "SQL State: " << e.sqlstate() << '\n';
        }
        catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "File already exists";
        }
        return rrr;
    }

    template<unsigned long segment_size>
    requires is_divisible<segment_size, SHA256size>
             && is_divisible<total_block_size, segment_size>
    template<verbose_level verbose>
    std::vector<std::string> dbManager<segment_size>::get_all_files(std::string_view dir_path) {
        //todo как лучше по пути или по имени
        std::vector<std::string> result;
        try {
            trasnactionType txn(*conn_);

            std::string query = "select file_name from files\n"
                                "    inner join public.directories d\n"
                                "        on d.dir_id = files.dir_id\n"
                                "where dir_path=$1;";
            pqxx::result res = txn.exec(query, pqxx::params(dir_path));
            LOG_IF(INFO, verbose >= 2) << res.query() << '\n';
            for (const auto &re: res) {
                auto tres = re[0].as<std::string>();
                result.push_back(tres.substr(1, tres.size() - 2));
            }

            txn.commit();
        } catch (const pqxx::sql_error &e) {
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what() << "\n"
                                        << "Query: " << e.query() << "\n"
                                        << "SQL State: " << e.sqlstate() << '\n';
        } catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error inserting block data:" << e.what() << '\n';
        }
        return result;
    }

    template<unsigned long segment_size>
    requires is_divisible<segment_size, SHA256size>
             && is_divisible<total_block_size, segment_size>
    template<verbose_level verbose>
    index_type dbManager<segment_size>::create_directory(std::string_view dir_path) {
        index_type result = -1;
        try {
            trasnactionType txn(*conn_);
            std::string query = "insert into public.directories (dir_path) values ($1) returning dir_id;";
            result = txn.exec(query, pqxx::params(dir_path)).one_row()[0].as<index_type>();


            txn.commit();
            LOG_IF(INFO, verbose >= 2)
                            << vformat("New directory %s with id %d was created", dir_path.data(), result) << '\n';
        } catch (const pqxx::sql_error &e) {
            if(e.sqlstate() == sqlLimitBreached_state)
            {
                LOG_IF(ERROR, verbose >= 1)<<vformat("Directory %s already exists\n",dir_path.data());
                return -1;
            }
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what() << "\n"
                                        << "Query: " << e.query() << "\n"
                                        << "SQL State: " << e.sqlstate() << '\n';
        } catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error inserting block data:" << e.what() << '\n';
        }
        return result;
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
            pqxx::result r = no_trans_exec.exec("SELECT 1 FROM pg_database WHERE datname = $1;",
                                                pqxx::params(no_trans_exec.esc(cString_.getDbname())));
            r.no_rows();

            /* if (!r.empty()) {
                 LOG_IF(WARNING, verbose >= 2)
                                 << "Database already exists, skipping creation: " << cString_.getDbname() << '\n';
                 no_trans_exec.abort();
             } else*/ {
                no_trans_exec.exec(vformat("CREATE DATABASE %s;", no_trans_exec.esc(
                        cString_.getDbname()).c_str()));//не менять, паарметры здесь не подпадают под query
                no_trans_exec.exec(
                        vformat("GRANT ALL ON DATABASE %s TO %s;", no_trans_exec.esc(cString_.getDbname()).c_str(),
                                no_trans_exec.esc(cString_.getUser()).c_str()));
                no_trans_exec.commit();

                LOG_IF(WARNING, verbose >= 2) << "Database created successfully: " << cString_.getDbname() << '\n';

            }


        } catch (const pqxx::sql_error &e) {
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what() << "\n"
                                        << "Query: " << e.query() << "\n"
                                        << "SQL State: " << e.sqlstate() << '\n';
            return;
        }
        catch (const pqxx::unexpected_rows &r) {
            LOG_IF(WARNING, verbose >= 1) << "Database already exists, aborting creation";
            LOG_IF(WARNING, verbose >= 2) << "    exception message: " << r.what();
            no_trans_exec.abort();
        }
        catch (const std::exception &e) {
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

            trasnactionType txn(*conn_);
            txn.exec("select tablename\n"
                     "from pg_tables\n"
                     "where schemaname = 'public';").no_rows();

            std::string query = "create table public.segments\n"
                                "(\n"
                                "    segment_hash bytea NOT NULL GENERATED ALWAYS AS (sha256(segment_data::bytea)) STORED primary key,\n"
                                "    segment_data bytea NOT NULL,\n"
                                "    segment_count bigint NOT NULL\n"
                                ");";

            std::string q1 = vformat(query.c_str());//, segment_size);
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
        }
        catch (const pqxx::unexpected_rows &r) {
            LOG_IF(WARNING, verbose >= 1) << "Database schemas have been already created, aborting creation";
            LOG_IF(WARNING, verbose >= 2) << "    exception message: " << r.what();
        }
        catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error setting up database schemas: " << e.what() << '\n';
        }
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
                                "    select get_file_data($1) as data \n"//todo replace
                                "    )as ss;";
            pqxx::result res = txn.exec(query, pqxx::params(file_name));

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
