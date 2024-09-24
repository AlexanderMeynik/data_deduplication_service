#include <pqxx/pqxx>
#include <iostream>
#include "lib.h"
#include "../common/myconcepts.h"

#ifndef SERVICE_DBMANAGER_H
#define SERVICE_DBMANAGER_H

namespace db_services {


    template<unsigned long segment_size = 64> requires is_divisible<total_block_size, segment_size>
    class dbManager {
    public:
        static constexpr unsigned long long block_size = total_block_size / segment_size;

        dbManager() = default;

        template<typename ss>
        requires to_str_to_c_str<ss>
        explicit dbManager(ss &&cstring): cString_(std::forward<ss>(cstring)) {

        }

        template<verbose_level verbose = 0>
        conPtr connectToDb();


        //inpired by https://stackoverflow.com/questions/49122358/libbqxx-c-api-to-connect-to-postgresql-without-db-name
        template<verbose_level verbose = 0>
        index_type create();

        template<verbose_level verbose = 0, hash_function hash = SHA_256>
        requires is_divisible<segment_size, hash_function_size[hash]>
        int fill_schemas();

        template<verbose_level verbose = 0>
        index_type create_directory(std::string_view dir_path);

        template<verbose_level verbose = 0>
        std::vector<std::pair<index_type, std::string>> get_all_files(std::string_view dir_path);

        template<verbose_level verbose = 0>
        index_type create_file(std::string_view file_path, index_type dir_id, int file_size = 0);

        template<verbose_level verbose = 0>
        index_type create_file_temp(std::string_view file_path);

        template<verbose_level verbose = 0>
        int finish_file_processing(std::string_view file_path, index_type file_id);

        template<verbose_level verbose = 0, delete_strategy del = cascade>
        int delete_file(std::string_view file_name, index_type file_id = return_codes::already_exists);

        template<verbose_level verbose = 0, delete_strategy del = cascade>
        int delete_directory(std::string_view directory_path, index_type dir_id = return_codes::already_exists);

        template<verbose_level verbose = 0>
        int get_file_streamed(std::string_view file_name, std::ostream &out);

        template<verbose_level verbose = 0>
        int insert_file_from_stream(std::string_view file_name, std::istream &in);


        bool checkConnection() {
            return conn_->is_open();
        }

    private:
        my_conn_string cString_;
        conPtr conn_;
    };


    template<unsigned long segment_size>
    requires is_divisible<total_block_size, segment_size>
    template<verbose_level verbose, delete_strategy del>
    int dbManager<segment_size>::delete_directory(std::string_view directory_path, index_type dir_id) {
        try {
            trasnactionType txn(*conn_);

            if (dir_id == return_codes::already_exists) {//todo not very good name for optional vlaue
                std::string query = "select dir_id from public.directories where dir_path = \'%s\';";
                std::string qx1 = vformat(query.c_str(), directory_path.data());
                dir_id = txn.query_value<index_type>(qx1);
            }

            std::string query, qr;
            ResType res;
            if constexpr (del == cascade) {

                query = "update public.segments s "
                        "set segment_count=segment_count-ss.cnt "
                        "from (select segment_hash as hhhash, count(segment_hash) as cnt "
                        "      from public.data inner join public.files f on f.file_id = public.data.file_id "
                        "      where dir_id = %d "
                        "      group by dir_id, segment_hash "
                        ") as ss "
                        "where ss.hhhash=segment_hash;";
                qr = vformat(query.c_str(), dir_id);

                txn.exec(qr);

                LOG_IF(INFO, verbose >= 2) << vformat("Successfully reduced segments"
                                                      " counts for directory \"%s\"\n", directory_path.data());


                query = "delete from public.data d "
                        "       where exists "
                        "           ("
                        "           select f.file_id "
                        "           from public.files f "
                        "           where dir_id=%d and d.file_id=f.file_id "
                        "        );  ";

                qr = vformat(query.c_str(), dir_id);
                txn.exec(qr);

                LOG_IF(INFO, verbose >= 2) << vformat("Successfully deleted data "
                                                      "for directory \"%s\"\n", directory_path.data());


                query = "delete from public.files where dir_id=%d;";
                qr = vformat(query.c_str(), dir_id);
                txn.exec(qr);

                LOG_IF(INFO, verbose >= 2) << vformat("Successfully deleted public.files "
                                                      " for directory \"%s\"\n", directory_path.data());

                query = "delete from public.segments where segment_count=0";
                txn.exec(query);

                LOG_IF(INFO, verbose >= 2) << vformat("Successfully deleted redundant "
                                                      "segments for \"%s\"\n", directory_path.data());
            }

            query = "delete from public.directories where dir_id=%d;";
            qr = vformat(query.c_str(), dir_id);
            txn.exec(qr);

            LOG_IF(INFO, verbose >= 2) << vformat("Successfully entry "
                                                  " for directory \"%s\"\n", directory_path.data());
            txn.commit();

        } catch (const pqxx::sql_error &e) {
            if(e.sqlstate()==sqlFqConstraight)
            {
                LOG_IF(ERROR,verbose>=1)<<vformat("Unable to remove record for directory \"%s\" since files "
                                                  "for it are stile present.",directory_path.data());
                return return_codes::error_occured;//todo is thin an error
            }
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what()
                                        << "Query: " << e.query()
                                        << "SQL State: " << e.sqlstate() << '\n';
            return return_codes::error_occured;
        } catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error while delting directory:" << e.what() << '\n';
            return return_codes::error_occured;
        }
        return return_sucess;
    }


    template<unsigned long segment_size>
    requires is_divisible<total_block_size, segment_size>
    template<verbose_level verbose, delete_strategy del>
    int dbManager<segment_size>::delete_file(std::string_view file_name, index_type file_id) {
        try {
            trasnactionType txn(*conn_);
            if (file_id == return_codes::already_exists) {
                std::string query = "select file_id from public.files where file_name = \'%s\';";
                auto qx1 = vformat(query.c_str(), file_name.data());
                file_id = txn.query_value<index_type>(qx1);
            }
            std::string query, qr;
            ResType res;
            if constexpr (del == cascade) {

                query = "update public.segments s "
                        "set segment_count=segment_count-ss.cnt "
                        "from (select segment_hash as hhhash, count(segment_hash) as cnt "
                        "      from public.data "
                        "      where file_id = %d "
                        "      group by file_id, segment_hash "
                        ") as ss "
                        "where ss.hhhash=segment_hash;";

                qr = vformat(query.c_str(), file_id);
                res = txn.exec(qr);

                LOG_IF(INFO, verbose >= 2) << vformat("Successfully reduced segments"
                                                      " counts for file \"%s\"\n", file_name.data());


                query = "delete from public.data where file_id=%d;";

                qr = vformat(query.c_str(), file_id);
                res = txn.exec(qr);

                LOG_IF(INFO, verbose >= 2) << vformat("Successfully deleted data "
                                                      "for file \"%s\"\n", file_name.data());


                query = "delete from public.segments where segment_count=0";
                res = txn.exec(query);

                LOG_IF(INFO, verbose >= 2) << vformat("Successfully deleted redundant "
                                                      "segments for \"%s\"\n", file_name.data());
            }


            query = "delete from public.files where file_id=%d;";
            qr = vformat(query.c_str(), file_id);
            res = txn.exec(qr);


            LOG_IF(INFO, verbose >= 2) << vformat("Successfully deleted file "
                                                  "entry for \"%s\"\n", file_name.data());


            txn.commit();
        } catch (const pqxx::sql_error &e) {
            if(e.sqlstate()==sqlFqConstraight)
            {
                LOG_IF(ERROR,verbose>=1)<<vformat("Unable to remove record for file \"%s\" since segment data "
                                                  "for it are stile present.",file_name.data());
                return return_codes::error_occured;//todo is this an error
            }
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what()
                                        << "Query: " << e.query()
                                        << "SQL State: " << e.sqlstate() << '\n';
            return return_codes::error_occured;
        } catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error while deleting file:" << e.what() << '\n';
            return return_codes::error_occured;
        }
        return return_sucess;
    }


    template<unsigned long segment_size>
    requires is_divisible<total_block_size, segment_size>
    template<verbose_level verbose>
    int dbManager<segment_size>::get_file_streamed(std::string_view file_name, std::ostream &out) {
        try {
            trasnactionType txn(*conn_);
            std::string query = vformat("select s.segment_data "
                                        "from public.data"
                                        "         inner join public.segments s on s.segment_hash = public.data.segment_hash "
                                        "        inner join public.files f on f.file_id = public.data.file_id "
                                        "where file_name=\'%s\'::tsvector "
                                        "order by segment_num", file_name.data());
            for (auto [name]: txn.stream<pqxx::bytes>(query)) {
                out <<  hex_to_string(pqxx::to_string(name).substr(2));
            }
            txn.commit();


        } catch (const pqxx::sql_error &e) {
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what()
                                        << "Query: " << e.query()
                                        << "SQL State: " << e.sqlstate() << '\n';
            return return_codes::error_occured;
        } catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error inserting block data:" << e.what() << '\n';
            return return_codes::error_occured;
        }
        return return_codes::return_sucess;
    }


    template<unsigned long segment_size>
    requires is_divisible<total_block_size, segment_size>
    template<verbose_level verbose>
    int dbManager<segment_size>::insert_file_from_stream(std::string_view file_name, std::istream &in) {
        try {
            trasnactionType txn(*conn_);
            std::string table_name = vformat("\"temp_file_%s\"", file_name.data());
            pqxx::stream_to copy_stream = pqxx::stream_to::raw_table(txn, table_name);
            char cc = 0;
            int count = 0;
            int block_index = 1;
            std::string buffer(segment_size, '\0');
            while (!in.eof() && in.peek() != -1)//todo get file size
            {
                in.get(cc);

                buffer[count++] = cc;
                if (count == segment_size) {
                    auto str = string_to_hex(buffer);
                    copy_stream
                            << std::make_tuple(
                                    block_index,
                                    pqxx::binary_cast(buffer));
                    count = 0;

                    block_index++;
                }
            }
            if (count != 0) {
                copy_stream
                        << std::make_tuple(
                                block_index,
                                pqxx::binary_cast(buffer.substr(0, count)));
            }
            copy_stream.complete();
            txn.commit();
        } catch (const pqxx::sql_error &e) {
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what()
                                        << "Query: " << e.query()
                                        << "SQL State: " << e.sqlstate() << '\n';
            return return_codes::error_occured;
        } catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error inserting block data:" << e.what() << '\n';
            return return_codes::error_occured;
        }
        return return_sucess;
    }


    template<unsigned long segment_size>
    requires is_divisible<total_block_size, segment_size>
    template<verbose_level verbose>
    int dbManager<segment_size>::finish_file_processing(std::string_view file_path, index_type file_id) {
        try {
            trasnactionType txn(*conn_);

            std::string table_name = vformat("\"temp_file_%s\"", file_path.data());
            std::string table_name_ = vformat("\'temp_file_%s\'", file_path.data());

            std::string aggregation_table_name = vformat("\"new_segments_%s\"", file_path.data());
            ResType r;
            std::string q = vformat("SELECT * FROM pg_tables WHERE tablename = %s", table_name_.c_str());
            txn.exec(q).one_row();

            q = vformat("CREATE TABLE  %s AS SELECT DISTINCT t.data, COUNT(t.data) AS count "
                        "FROM %s t "
                        "GROUP BY t.data;", aggregation_table_name.c_str(), table_name.c_str());
            txn.exec(q);
            LOG_IF(INFO, verbose >= 2) << vformat(
                    "Segment data was aggregated into %s for file %s.",//todo some strange things happen there \"%s\"."
                    aggregation_table_name.c_str(),
                    file_path.data());


            q = vformat("INSERT INTO public.segments (segment_data, segment_count) "
                        "SELECT ns.data, ns.count "
                        "FROM %s ns "
                        "ON CONFLICT ON CONSTRAINT unique_data_constr "
                        "DO UPDATE "
                        "SET segment_count = public.segments.segment_count +  excluded.segment_count;",
                        aggregation_table_name.c_str());
            txn.exec(q);
            LOG_IF(INFO, verbose >= 2) << vformat("New segments were inserted for file \"%s\".", file_path.data());

            q = vformat("drop table %s;", aggregation_table_name.c_str());
            txn.exec(q);
            LOG_IF(INFO, verbose >= 2)
                            << vformat("Temporary aggregation table %s was deleted.", aggregation_table_name.c_str());


            q = vformat("INSERT INTO public.data (segment_num, segment_hash, file_id) "
                        "SELECT pos, se.segment_hash,  %d "
                        "FROM  %s tt "
                        "INNER JOIN public.segments se "
                        "ON tt.data = se.segment_data;",
                        file_id,
                        table_name.c_str()
            );
            txn.exec(q);
            LOG_IF(INFO, verbose >= 2) << vformat("Segment data of %s was inserted.", table_name.c_str());


            q = vformat("DROP TABLE IF EXISTS  %s;", table_name.c_str());
            txn.exec(q);
            LOG_IF(INFO, verbose >= 2) << vformat("Temp data table %s was deleted.", table_name.c_str());

            txn.commit();
        } catch (const pqxx::sql_error &e) {
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what()
                                        << "Query: " << e.query()
                                        << "SQL State: " << e.sqlstate() << '\n';
            return return_codes::error_occured;
        }
        catch (const pqxx::unexpected_rows &r) {
            LOG_IF(ERROR, verbose >= 1) << vformat("No data found for file \"%s\"", file_path.data());
            LOG_IF(ERROR, verbose >= 2) << vformat("Exception description: %s", r.what()) << '\n';
            return return_codes::error_occured;
        }
        catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error processing file data:" << e.what() << '\n';
            return return_codes::error_occured;
        }
        return 0;
    }


    template<unsigned long segment_size>
    requires is_divisible<total_block_size, segment_size>
    template<verbose_level verbose>
    index_type dbManager<segment_size>::create_file_temp(std::string_view file_path) {
        trasnactionType txn(*conn_);
        index_type rrr = return_codes::error_occured;

        std::string query;

        try {
            std::string table_name = vformat("temp_file_%s", file_path.data());
            std::string q1 = vformat(
                    "CREATE TABLE \"%s\" (pos bigint, data bytea);",
                    txn.esc(table_name).c_str()
            );
            ResType r2 = txn.exec(q1);
            txn.commit();

            LOG_IF(INFO, verbose >= 2) << vformat("Temp data table %s was created.", table_name.c_str());
        }
        catch (const pqxx::sql_error &e) {
            if (e.sqlstate() == sqlLimitBreached_state) {
                LOG_IF(ERROR, verbose >= 1) << vformat("File %s already exists\n", file_path.data());
                return return_codes::already_exists;
            }
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what()
                                        << "Query: " << e.query()
                                        << "SQL State: " << e.sqlstate() << '\n';
        }
        catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "File already exists";
        }
        return rrr;
    }

    template<unsigned long segment_size>
    requires is_divisible<total_block_size, segment_size>
    template<verbose_level verbose>
    index_type dbManager<segment_size>::create_file(std::string_view file_path, index_type dir_id, int file_size) {
        trasnactionType txn(*conn_);
        index_type future_file_id = return_codes::error_occured;

        std::string query;
        ResType res;
        try {
            if (dir_id == index_vals::empty_parameter_value) {
                query = "insert into public.files (file_name,size_in_bytes)"
                        " values ($1,$2) returning file_id;";
                res = txn.exec(query, {file_path, file_size});
            } else {
                query = "insert into public.files (file_name,dir_id,size_in_bytes)"
                        " values ($1,$2,$3) returning file_id;";
                res = txn.exec(query, {file_path, dir_id, file_size});
            }

            future_file_id = res.one_row()[0].as<index_type>();
            LOG_IF(INFO, verbose >= 2) << vformat("File record (%d,\"%s\",%d,%d) was successfully created.",
                                                  future_file_id,
                                                  file_path.data(),
                                                  dir_id,
                                                  file_size);

            std::string table_name = vformat("temp_file_%s", file_path.data());
            std::string q1 = vformat(
                    "CREATE TABLE \"%s\" (pos bigint, data bytea);",
                    txn.esc(table_name).c_str()
            );
            txn.exec(q1);

            txn.commit();

            LOG_IF(INFO, verbose >= 2) << vformat("Temp data table \"%s\" was created.", table_name.c_str());
        }
        catch (const pqxx::sql_error &e) {
            if (e.sqlstate() == sqlLimitBreached_state) {
                LOG_IF(ERROR, verbose >= 1) << vformat("File %s already exists\n", file_path.data());
                return return_codes::already_exists;
            }
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what()
                                        << "Query: " << e.query()
                                        << "SQL State: " << e.sqlstate() << '\n';
        }
        catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error while creting file:" << e.what() << '\n';
        }
        return future_file_id;
    }

    template<unsigned long segment_size>
    requires is_divisible<total_block_size, segment_size>
    template<verbose_level verbose>
    std::vector<std::pair<index_type, std::string>> dbManager<segment_size>::get_all_files(std::string_view dir_path) {
        //todo как лучше по пути или по имени
        std::vector<std::pair<index_type, std::string>> result;
        try {
            trasnactionType txn(*conn_);

            std::string query = "select file_id,file_name from public.files "
                                "    inner join public.directories d "
                                "        on d.dir_id = public.files.dir_id "
                                "where dir_path=$1;";
            ResType res = txn.exec(query, pqxx::params(dir_path));
            LOG_IF(INFO, verbose >= 2)
                            << vformat("Filenames were successfully retrieved for directory \"%s\".", dir_path.data());
            for (const auto &re: res) {
                auto tres = re[1].as<std::string>();
                result.emplace_back(re[0].as<index_type>(), tres.substr(1, tres.size() - 2));
            }
            txn.commit();
        } catch (const pqxx::sql_error &e) {
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what()
                                        << "Query: " << e.query()
                                        << "SQL State: " << e.sqlstate() << '\n';
        } catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error while getting all files:" << e.what() << '\n';
        }
        return result;
    }

    template<unsigned long segment_size>
    requires is_divisible<total_block_size, segment_size>
    template<verbose_level verbose>
    index_type dbManager<segment_size>::create_directory(std::string_view dir_path) {
        index_type result = return_codes::error_occured;
        try {
            trasnactionType txn(*conn_);
            std::string query = "insert into public.directories (dir_path) values ($1) returning dir_id;";
            result = txn.exec(query, pqxx::params(dir_path)).one_row()[0].as<index_type>();

            txn.commit();
            LOG_IF(INFO, verbose >= 2)
                            << vformat("New directory %s with id %d was created", dir_path.data(), result) << '\n';
        } catch (const pqxx::sql_error &e) {
            if (e.sqlstate() == sqlLimitBreached_state) {
                LOG_IF(ERROR, verbose >= 1) << vformat("Directory \"%s\" already exists\n", dir_path.data());
                return return_codes::already_exists;
            }
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what()
                                        << "Query: " << e.query()
                                        << "SQL State: " << e.sqlstate() << '\n';
        } catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error creating directory:" << e.what() << '\n';
        }
        return result;
    }


    template<unsigned long segment_size>
    requires is_divisible<total_block_size, segment_size>
    template<verbose_level verbose>
    conPtr dbManager<segment_size>::connectToDb() {
        if (conn_ && conn_->is_open()) {
            return conn_;
        }
        LOG_IF(WARNING, verbose >= 1) << "Failed to listen with current connection, attempt to reconnect via cString\n";
        conn_ = connect_if_possible<verbose>(cString_);//todo error return via expected
        return conn_;
    }


    template<unsigned long segment_size>
    requires is_divisible<total_block_size, segment_size>
    template<verbose_level verbose>
    index_type dbManager<segment_size>::create() {
        conn_ = nullptr;
        auto tString = cString_;
        tString.set_dbname(sample_temp_db);
        auto temp_connection = connect_if_possible<verbose>(tString);

        if (!temp_connection || !temp_connection->is_open()) {
            LOG_IF(WARNING, verbose >= 1)
                            << vformat("Unable to connect by url \"%s\"\n", (tString).operator std::string().c_str());
            return return_codes::error_occured;
        }
        LOG_IF(INFO, verbose >= 1) << vformat("Connected to database %s\n", tString.getDbname().c_str());


        pqxx::nontransaction no_trans_exec(*temp_connection);
        //todo use type template

        try {
            ResType r = no_trans_exec.exec("SELECT 1 FROM pg_database WHERE datname = $1;",
                                                pqxx::params(no_trans_exec.esc(cString_.getDbname())));
            r.no_rows();


            no_trans_exec.exec(vformat("CREATE DATABASE %s;", no_trans_exec.esc(cString_.getDbname()).c_str()));//todo use "" to save db
            //не менять, паарметры здесь не подпадают под query
            no_trans_exec.exec(vformat("GRANT ALL ON DATABASE %s TO %s;",
                                       no_trans_exec.esc(cString_.getDbname()).c_str(),
                                       no_trans_exec.esc(cString_.getUser()).c_str()));
            no_trans_exec.commit();

            LOG_IF(WARNING, verbose >= 2) << "Database created successfully: " << cString_.getDbname() << '\n';


        } catch (const pqxx::sql_error &e) {
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what()
                                        << "Query: " << e.query()
                                        << "SQL State: " << e.sqlstate() << '\n';
            return return_codes::error_occured;
        }
        catch (const pqxx::unexpected_rows &r) {
            LOG_IF(WARNING, verbose >= 1) << "Database already exists, aborting creation";
            LOG_IF(WARNING, verbose >= 2) << "    exception message: " << r.what();
            no_trans_exec.abort();
            temp_connection->close();
            conn_ = connect_if_possible<verbose>(cString_);
            return return_codes::already_exists;
        }
        catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error: " << e.what() << '\n';
            return return_codes::error_occured;
        }


        temp_connection->close();
        conn_ = connect_if_possible<verbose>(cString_);
        return return_codes::return_sucess;
    }


    template<unsigned long segment_size>
    requires is_divisible<total_block_size, segment_size>
    template<verbose_level verbose, hash_function hash>
    requires is_divisible<segment_size, hash_function_size[hash]>
    int dbManager<segment_size>::fill_schemas() {
        try {

            trasnactionType txn(*conn_);
            txn.exec("select tablename "
                     "from pg_tables "
                     "where schemaname = 'public';").no_rows();

            std::string query = vformat("create schema if not exists public;"
                                        "create table public.segments"
                                        "("
                                        "    segment_hash bytea NOT NULL GENERATED ALWAYS AS ((%s(segment_data::bytea))::bytea) STORED primary key, "
                                        "    segment_data bytea NOT NULL,"
                                        "    segment_count bigint NOT NULL"
                                        ");", hash_function_name[hash]);
            txn.exec(query);
            LOG_IF(INFO, verbose >= 2) << vformat("Create segments table with preferred hashing function %s\n",
                                                  hash_function_name[hash]);


            query = "create table public.directories "
                    "("
                    "    dir_id serial primary key NOT NULL,"
                    "    dir_path tsvector NOT NULL"
                    ");"
                    ""
                    "create table public.files "
                    "("
                    "    file_id serial primary key NOT NULL,"
                    "    file_name tsvector NOT NULL,"
                    "    dir_id int REFERENCES public.directories(dir_id) NULL,"
                    "    size_in_bytes bigint NULL"
                    ");"
                    ""
                    "create table public.data "
                    "("
                    "    file_id int REFERENCES public.files(file_id) NOT NULL,"
                    "    segment_num bigint NOT NULL,"
                    "    segment_hash bytea REFERENCES public.segments(segment_hash) NOT NULL"
                    ");";
            txn.exec(query);
            LOG_IF(INFO, verbose >= 2) << "Create directories, files and data tables\n";


            query = "CREATE INDEX if not exists hash_segment_hash on public.segments(segment_hash); "
                    "CREATE INDEX if not exists hash_segment_data on public.segments(segment_data); "
                    "CREATE INDEX if not exists segment_count on public.segments(segment_count); "
                    "CREATE INDEX  if not exists bin_file_id on public.data(file_id); "
                    "CREATE INDEX if not exists dir_gin_index on public.directories using gin(dir_path); "
                    "CREATE INDEX if not exists dir_bin_index on public.directories(dir_path); "
                    "CREATE INDEX if not exists dir_id_bin on public.files(dir_id); "
                    "CREATE INDEX if not exists files_bin_index on public.files(file_name); "
                    "CREATE INDEX if not exists files_gin_index on public.files using gin(file_name); "
                    "CREATE INDEX if not exists bin_file_id_ on public.files(file_id); ";
            txn.exec(query);
            LOG_IF(INFO, verbose >= 2) << "Create indexes for main tables\n";


            query = "ALTER TABLE public.segments ADD CONSTRAINT unique_data_constr UNIQUE(segment_data); "
                    "ALTER TABLE public.files ADD CONSTRAINT unique_file_constr UNIQUE(file_name); "
                    "ALTER TABLE public.directories ADD CONSTRAINT unique_dir_constr UNIQUE(dir_path);";
            txn.exec(query);
            LOG_IF(INFO, verbose >= 2) << "Create unique constraints for tables\n";
            txn.commit();
        } catch (const pqxx::sql_error &e) {
            LOG_IF(ERROR, verbose >= 1) << "SQL Error: " << e.what()
                                        << "Query: " << e.query()
                                        << "SQL State: " << e.sqlstate() << '\n';
            return return_codes::error_occured;
        }
        catch (const pqxx::unexpected_rows &r) {
            LOG_IF(WARNING, verbose >= 1) << "Database schemas have been already created, aborting creation";
            LOG_IF(WARNING, verbose >= 2) << "    exception message: " << r.what();
            return return_codes::already_exists;
        }
        catch (const std::exception &e) {
            LOG_IF(ERROR, verbose >= 1) << "Error setting up database schemas: " << e.what() << '\n';
            return return_codes::error_occured;
        }
        return return_sucess;
    }
}


#endif
