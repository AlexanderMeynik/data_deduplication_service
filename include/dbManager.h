

#ifndef SERVICE_DBMANAGER_H
#define SERVICE_DBMANAGER_H

#include <pqxx/pqxx>
#include <iostream>
#include "lib.h"
#include "myconcepts.h"

namespace db_services {


    inline void diconnect(conPtr &conn_) {
        if (conn_) {
            conn_->close();
            conn_ = nullptr;
        }
    }

    template<unsigned long segment_size>
    class dbManager {
    public:

        static constexpr unsigned long long block_size = total_block_size / segment_size;

        dbManager() : cString_(db_services::default_configuration()), conn_(nullptr) {};

        explicit dbManager(my_conn_string &ss) : cString_(ss), conn_(nullptr) {};

        void setCString(my_conn_string &ss) {
            cString_ = ss;
            disconnect();
        }

        [[nodiscard]] const my_conn_string &getCString() const {
            return cString_;
        }


        int connectToDb();

        void disconnect() {
            db_services::diconnect(conn_);
        }

        //inpired by https://stackoverflow.com/questions/49122358/libbqxx-c-api-to-connect-to-postgresql-without-db-name
        index_type create_database(std::string_view dbName);

        index_type drop_database(std::string_view dbName);

        template<hash_function hash = SHA_256>
        requires is_divisible<segment_size, hash_function_size[hash]>
        int fill_schemas();


        index_type create_directory(std::string_view dir_path);


        std::vector<std::pair<index_type, std::string>> get_all_files(std::string_view dir_path);


        index_type create_file(std::string_view file_path, index_type dir_id, int file_size = 0);


        template<hash_function hash = SHA_256>
        int finish_file_processing(std::string_view file_path, index_type file_id);

        template<delete_strategy del = cascade>
        int delete_file(std::string_view file_name, index_type file_id = return_codes::already_exists);

        template<delete_strategy del = cascade>
        int delete_directory(std::string_view directory_path, index_type dir_id = index_vals::empty_parameter_value);


        int get_file_streamed(std::string_view file_name, std::ostream &out);


        int insert_file_from_stream(std::string_view file_name, std::istream &in, std::size_t file_size);


        bool check_connection() {
            return db_services::checkConnection(conn_);
        }

        ~dbManager() {
            disconnect();
        }


    private:
        my_conn_string cString_;
        conPtr conn_;


    };


    template<unsigned long segment_size>
    template<delete_strategy del>
    int dbManager<segment_size>::delete_directory(std::string_view directory_path, index_type dir_id) {
        try {
            trasnactionType txn(*conn_);

            if (dir_id == index_vals::empty_parameter_value) {
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

                auto res1 = txn.exec(qr);

                VLOG(2) << vformat("Successfully reduced segment"
                                   " counts for directory \"%s\"\n", directory_path.data());
                printRows_affected(res1);


                query = "delete from public.data d "
                        "       where exists "
                        "           ("
                        "           select f.file_id "
                        "           from public.files f "
                        "           where dir_id=%d and d.file_id=f.file_id "
                        "        );  ";

                qr = vformat(query.c_str(), dir_id);
                res1 = txn.exec(qr);

                VLOG(2) << vformat("Successfully deleted data "
                                   "for directory \"%s\"\n", directory_path.data());
                printRows_affected(res1);


                query = "delete from public.files where dir_id=%d;";
                qr = vformat(query.c_str(), dir_id);
                res1 = txn.exec(qr);

                VLOG(2) << vformat("Successfully deleted public.files "
                                   " for directory \"%s\"\n", directory_path.data());
                printRows_affected(res1);

                query = "delete from public.segments where segment_count=0";
                res1 = txn.exec(query);

                VLOG(2) << vformat("Successfully deleted redundant "
                                   "segments for \"%s\"\n", directory_path.data());
                printRows_affected(res1);
            }

            query = "delete from public.directories where dir_id=%d;";
            qr = vformat(query.c_str(), dir_id);
            txn.exec(qr);

            VLOG(2) << vformat("Successfully deleted entry "
                               " for directory \"%s\"\n", directory_path.data());
            txn.commit();

        } catch (const pqxx::sql_error &e) {
            if (e.sqlstate() == sqlFqConstraight) {
                VLOG(1) << vformat("Unable to remove record for directory \"%s\" since files "
                                   "for it are stile present.", directory_path.data());
                return return_codes::warning_message;
            }
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
            return return_codes::error_occured;
        } catch (const std::exception &e) {
            VLOG(1) << "Error while delting directory:" << e.what() << '\n';
            return return_codes::error_occured;
        }
        return return_sucess;
    }


    template<unsigned long segment_size>
    template<delete_strategy del>
    int dbManager<segment_size>::delete_file(std::string_view file_name, index_type file_id) {
        try {

            trasnactionType txn(*conn_);
            if (file_id == return_codes::already_exists) {//todo optinal value
                std::string query = "select file_id from public.files where file_name = \'%s\';";
                auto qx1 = vformat(query.c_str(), file_name.data());
                file_id = txn.query_value<index_type>(qx1);
            }
            std::string query, qr;
            ResType res;


            auto hash_str=get_table_name(txn,file_name);
            std::string table_name=vformat("temp_file_%s",hash_str.c_str());
            //std::string table_name = vformat("\"temp_file_%s\"", file_name.data());//todo check
            qr = vformat("DROP TABLE IF EXISTS  \"%s\";", table_name.c_str());
            res = txn.exec(qr);
            VLOG(3) << res.query();
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

                VLOG(2) << vformat("Successfully reduced segments"
                                   " counts for file \"%s\"\n", file_name.data());


                query = "delete from public.data where file_id=%d;";

                qr = vformat(query.c_str(), file_id);
                res = txn.exec(qr);

                VLOG(2) << vformat("Successfully deleted data "
                                   "for file \"%s\"\n", file_name.data());


                query = "delete from public.segments where segment_count=0";
                res = txn.exec(query);

                VLOG(2) << vformat("Successfully deleted redundant "
                                   "segments for \"%s\"\n", file_name.data());
            }


            query = "delete from public.files where file_id=%d;";
            qr = vformat(query.c_str(), file_id);
            res = txn.exec(qr);


            VLOG(2) << vformat("Successfully deleted file "
                               "entry for \"%s\"\n", file_name.data());


            txn.commit();
        } catch (const pqxx::sql_error &e) {
            if (e.sqlstate() == sqlFqConstraight) {
                VLOG(1) << vformat("Unable to remove record for file \"%s\" since segment data "
                                   "for it are stile present.", file_name.data());
                return return_codes::warning_message;
            }
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
            return return_codes::error_occured;
        } catch (const std::exception &e) {
            VLOG(1) << "Error while deleting file:" << e.what() << '\n';
            return return_codes::error_occured;
        }
        return return_sucess;
    }


    template<unsigned long segment_size>
    int dbManager<segment_size>::get_file_streamed(std::string_view file_name, std::ostream &out) {
        try {
            trasnactionType txn(*conn_);
            std::string query = vformat("select s.segment_data "
                                        "from public.data"
                                        "         inner join public.segments s on s.segment_hash = public.data.segment_hash "
                                        "        inner join public.files f on f.file_id = public.data.file_id "
                                        "where file_name=\'%s\'::tsvector "
                                        "order by segment_num", file_name.data());
            for (auto [name]: txn.stream<pqxx::binarystring>(query)) {
                //out<<name;
                out << name.str();
                //out << hex_to_string(pqxx::to_string(name).substr(2));//this get string bytes without conversion
            }
            txn.commit();


        } catch (const pqxx::sql_error &e) {
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
            return return_codes::error_occured;
        } catch (const std::exception &e) {
            VLOG(1) << "Error inserting block data:" << e.what() << '\n';
            return return_codes::error_occured;
        }
        return return_codes::return_sucess;
    }


    template<unsigned long segment_size>
    int dbManager<segment_size>::insert_file_from_stream(std::string_view file_name, std::istream &in,
                                                         std::size_t file_size) {
        try {
            trasnactionType txn(*conn_);

            auto hash_str=get_table_name(txn,file_name);
            std::string table_name=vformat("\"temp_file_%s\"",hash_str.c_str());
            /*std::string table_name = vformat("\"temp_file_%s\"", file_name.data());*///todo check2
            pqxx::stream_to copy_stream = pqxx::stream_to::raw_table(txn, table_name);
            int block_index = 1;

            std::string buffer(segment_size, '\0');

            size_t block_count = file_size / segment_size;
            size_t l_block_size = file_size - block_count * segment_size;
            std::istream::sync_with_stdio(false);
            for (int i = 0; i < block_count; ++i) {
                in.read(buffer.data(), segment_size);
                //std::from_b
                copy_stream
                        << std::make_tuple(
                                block_index++,
                                pqxx::binary_cast(buffer));

            }
            if (l_block_size != 0) {
                std::string bff(l_block_size,'\0');
                in.read(bff.data(),segment_size);
                copy_stream
                        << std::make_tuple(
                                block_index,
                                pqxx::binary_cast(bff));
            }
            copy_stream.complete();
            txn.commit();
        } catch (const pqxx::sql_error &e) {
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
            return return_codes::error_occured;
        } catch (const std::exception &e) {
            VLOG(1) << "Error inserting block data:" << e.what() << '\n';
            return return_codes::error_occured;
        }
        return return_sucess;
    }


    template<unsigned long segment_size>
    template<hash_function hash>
    int dbManager<segment_size>::finish_file_processing(std::string_view file_path, index_type file_id) {
        try {
            trasnactionType txn(*conn_);
            auto hash_str=get_table_name(txn,file_path);
            std::string table_name=vformat("temp_file_%s",hash_str.c_str()); //todo check
            /*std::string table_name = vformat("temp_file_%s", file_path.data());
            std::string table_name_ = vformat("temp_file_%s", file_path.data());*/


            std::string aggregation_table_name = vformat("new_segments_%s", hash_str.c_str());//todo check 2
            ResType r;
            std::string q = vformat("SELECT * FROM pg_tables WHERE tablename = \'%s\'", table_name.c_str());
            clk.tik();
            txn.exec(q).one_row();
            clk.tak();
            q = vformat("CREATE TABLE  \"%s\" AS SELECT t.data, COUNT(t.data) AS count "
                        "FROM \"%s\" t "
                        "GROUP BY t.data;", aggregation_table_name.c_str(), table_name.c_str());
            clk.tik();
            txn.exec(q);
            clk.tak();
            VLOG(2) << vformat(
                    "Segment data was aggregated into %s for file %s.",
                    aggregation_table_name.c_str(),
                    file_path.data());


            q = vformat("INSERT INTO public.segments (segment_data, segment_count) "
                        "SELECT ns.data, ns.count "
                        "FROM \"%s\" ns "
                        "ON CONFLICT (segment_hash) "
                        "DO UPDATE "
                        "SET segment_count = public.segments.segment_count +  excluded.segment_count;",
                        aggregation_table_name.c_str());
            clk.tik();
            txn.exec(q);
            clk.tak();
            VLOG(2) << vformat("New segments were inserted for file \"%s\".", file_path.data());

            q = vformat("drop table \"%s\";", aggregation_table_name.c_str());
            clk.tik();
            txn.exec(q);
            clk.tak();
            VLOG(2)
                            << vformat("Temporary aggregation table %s was deleted.", aggregation_table_name.c_str());


            q = vformat("INSERT INTO public.data (segment_num, segment_hash, file_id) "
                        "SELECT tt.pos, %s(tt.data),  %d "
                        "FROM  \"%s\" tt ", hash_function_name[hash],
                        file_id,
                        table_name.c_str()
            );
            clk.tik();
            txn.exec(q);
            clk.tak();
            VLOG(2) << vformat("Segment data of %s was inserted.", table_name.c_str());


            q = vformat("DROP TABLE IF EXISTS  \"%s\";", table_name.c_str());
            //todo if exists is redundant
            clk.tik();
            txn.exec(q);
            clk.tak();
            VLOG(2) << vformat("Temp data table %s was deleted.", table_name.c_str());

            txn.commit();
        } catch (const pqxx::sql_error &e) {
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
            return return_codes::error_occured;
        }
        catch (const pqxx::unexpected_rows &r) {
            VLOG(1) << vformat("No data found for file \"%s\"", file_path.data());
            VLOG(2) << vformat("Exception description: %s", r.what()) << '\n';
            return return_codes::error_occured;
        }
        catch (const std::exception &e) {
            VLOG(1) << "Error processing file data:" << e.what() << '\n';
            return return_codes::error_occured;
        }
        return return_codes::return_sucess;
    }


    template<unsigned long segment_size>
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
            VLOG(2) << vformat("File record (%d,\"%s\",%d,%d) was successfully created.",
                               future_file_id,
                               file_path.data(),
                               dir_id,
                               file_size);
            auto hash_str=get_table_name(txn,file_path);
            std::string table_name=vformat("temp_file_%s",hash_str.c_str());//todo check 4
            /*std::string table_name = vformat("temp_file_%s",
                                             file_path.data());
            */
            std::string q1 = vformat(
                    "CREATE TABLE \"%s\" (pos bigint, data bytea);",
                    table_name.c_str()//todo removed esc
            );
            txn.exec(q1);

            txn.commit();

            VLOG(2) << vformat("Temp data table %s was created.", table_name.c_str());
        }
        catch (const pqxx::sql_error &e) {
            if (e.sqlstate() == sqlLimitBreached_state) {
                VLOG(1) << vformat("File %s already exists\n", file_path.data());
                return return_codes::already_exists;
            }
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
        }
        catch (const std::exception &e) {
            VLOG(1) << "Error while creting file:" << e.what() << '\n';
        }
        return future_file_id;
    }

    template<unsigned long segment_size>
    std::vector<std::pair<index_type, std::string>> dbManager<segment_size>::get_all_files(std::string_view dir_path) {
        std::vector<std::pair<index_type, std::string>> result;
        try {
            trasnactionType txn(*conn_);

            std::string query = "select file_id,file_name from public.files "
                                "    inner join public.directories d "
                                "        on d.dir_id = public.files.dir_id "
                                "where dir_path=$1;";
            ResType res = txn.exec(query, pqxx::params(dir_path));
            VLOG(2)
                            << vformat("Filenames were successfully retrieved for directory \"%s\".", dir_path.data());
            for (const auto &re: res) {
                auto tres = re[1].as<std::string>();
                result.emplace_back(re[0].as<index_type>(), tres.substr(1, tres.size() - 2));
            }
            txn.commit();
        } catch (const pqxx::sql_error &e) {
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
        } catch (const std::exception &e) {
            VLOG(1) << "Error while getting all files:" << e.what() << '\n';
        }
        return result;
    }

    template<unsigned long segment_size>
    index_type dbManager<segment_size>::create_directory(std::string_view dir_path) {
        index_type result = return_codes::error_occured;
        try {
            trasnactionType txn(*conn_);
            std::string query = "insert into public.directories (dir_path) values ($1) returning dir_id;";
            result = txn.exec(query, pqxx::params(dir_path)).one_row()[0].as<index_type>();

            txn.commit();
            VLOG(2)
                            << vformat("New directory %s with id %d was created", dir_path.data(), result) << '\n';
        } catch (const pqxx::sql_error &e) {
            if (e.sqlstate() == sqlLimitBreached_state) {
                VLOG(1) << vformat("Directory \"%s\" already exists\n", dir_path.data());
                return return_codes::already_exists;
            }
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
        } catch (const std::exception &e) {
            VLOG(1) << "Error creating directory:" << e.what() << '\n';
        }
        return result;
    }


    template<unsigned long segment_size>
    int dbManager<segment_size>::connectToDb() {
        if (check_connection()) {
            return return_codes::return_sucess;
        }
        VLOG(1) << "Failed to listen with current connection, attempt to reconnect via cString\n";
        auto result = connect_if_possible(cString_);

        conn_ = result.value_or(nullptr);
        if (!result.has_value()) {
            return result.error();
        }
        //return result.error();
        return return_codes::return_sucess;
    }


    template<unsigned long segment_size>
    index_type dbManager<segment_size>::drop_database(std::string_view dbName) {
        if (check_connection()) {
            conn_->close();
        }
        //todo variant with no dbName
        conn_ = nullptr;
        cString_.set_dbname(dbName);

        auto tString = cString_;
        tString.set_dbname(sample_temp_db);

        auto result = connect_if_possible(tString);

        auto temp_connection = result.value_or(nullptr);

        if (!result.has_value()) {
            VLOG(1)
                            << vformat("Unable to connect by url \"%s\"\n", (tString).operator std::string().c_str());
            return return_codes::error_occured;
        }
        VLOG(1) << vformat("Connected to database %s\n", tString.getDbname().c_str());


        nonTransType no_trans_exec(*temp_connection);
        std::string qq;
        try {
            check_database_existence(no_trans_exec, cString_.getDbname()).one_row();

            terminate_all_db_connections(no_trans_exec, cString_.getDbname());


            qq = vformat("DROP DATABASE \"%s\";",
                         cString_.getDbname().c_str());
            no_trans_exec.exec(qq);

            no_trans_exec.commit();

            VLOG(2) << "Database deleted successfully: " << cString_.getDbname();


        } catch (const pqxx::sql_error &e) {
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
            return return_codes::error_occured;
        }
        catch (const pqxx::unexpected_rows &r) {
            VLOG(1) << vformat("No database %s found! Abort drop! ", cString_.getDbname().c_str());
            VLOG(2) << "    exception message: " << r.what();
            no_trans_exec.abort();
            temp_connection->close();
            conn_ = connect_if_possible(cString_).value_or(nullptr);
            return return_codes::warning_message;
        }
        catch (const std::exception &e) {
            VLOG(1) << "Error: " << e.what() << '\n';
            return return_codes::error_occured;
        }


        temp_connection->close();

        return return_sucess;
    }


    template<unsigned long segment_size>
    index_type dbManager<segment_size>::create_database(std::string_view dbName) {
        conn_ = nullptr;

        cString_.set_dbname(dbName);

        auto tString = cString_;
        tString.set_dbname(sample_temp_db);

        auto result = connect_if_possible(
                tString);//todo since we change only database name do we need to pass whole string

        auto temp_connection = result.value_or(nullptr);

        if (!result.has_value()) {
            VLOG(1)
                            << vformat("Unable to connect by url \"%s\"\n", (tString).operator std::string().c_str());
            return return_codes::error_occured;
        }
        VLOG(1) << vformat("Connected to database %s\n", tString.getDbname().c_str());


        nonTransType no_trans_exec(*temp_connection);

        try {
            check_database_existence(no_trans_exec, cString_.getDbname()).no_rows();

            no_trans_exec.exec(vformat("CREATE DATABASE \"%s\";",
                                       cString_.getDbname().c_str()));
            //не менять, паарметры здесь не подпадают под query
            no_trans_exec.exec(vformat("GRANT ALL ON DATABASE %s TO %s;",
                                       no_trans_exec.esc(cString_.getDbname()).c_str(),
                                       no_trans_exec.esc(cString_.getUser()).c_str()));
            no_trans_exec.commit();

            VLOG(2) << "Database created successfully: " << cString_.getDbname() << '\n';


        } catch (const pqxx::sql_error &e) {
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
            return return_codes::error_occured;
        }
        catch (const pqxx::unexpected_rows &r) {
            VLOG(1) << "Database already exists, aborting creation";
            VLOG(2) << "    exception message: " << r.what();
            no_trans_exec.abort();
            temp_connection->close();
            conn_ = connect_if_possible(cString_).value_or(nullptr);
            return return_codes::already_exists;
        }
        catch (const std::exception &e) {
            VLOG(1) << "Error: " << e.what() << '\n';
            return return_codes::error_occured;
        }


        temp_connection->close();
        conn_ = connect_if_possible(cString_).value_or(nullptr);
        return return_codes::return_sucess;
    }


    template<unsigned long segment_size>
    template<hash_function hash>
    requires is_divisible<segment_size, hash_function_size[hash]>
    int dbManager<segment_size>::fill_schemas() {
        //todo check schemas
        try {

            trasnactionType txn(*conn_);
            check_schemas(txn).no_rows();

            std::string query = vformat("create schema if not exists public;"
                                        "create table public.segments"
                                        "("
                                        "    segment_hash bytea NOT NULL GENERATED ALWAYS AS ((%s(segment_data::bytea))::bytea) STORED primary key, "
                                        "    segment_data bytea NOT NULL,"
                                        "    segment_count bigint NOT NULL"
                                        ");", hash_function_name[hash]);
            txn.exec(query);
            VLOG(2) << vformat("Create segments table with preferred hashing function %s\n",
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
            VLOG(2) << "Create directories, files and data tables\n";


            query = "CREATE INDEX if not exists segment_count on public.segments(segment_count); "
                    "CREATE INDEX  if not exists bin_file_id on public.data(file_id); "
                    "CREATE INDEX if not exists dir_gin_index on public.directories using gin(dir_path); "
                    "CREATE INDEX if not exists dir_id_bin on public.files(dir_id); "
                    "CREATE INDEX if not exists files_gin_index on public.files using gin(file_name); ";
            txn.exec(query);
            VLOG(2) << "Create indexes for main tables\n";


            query = "ALTER TABLE public.files ADD CONSTRAINT unique_file_constr UNIQUE(file_name); "
                    "ALTER TABLE public.directories ADD CONSTRAINT unique_dir_constr UNIQUE(dir_path);";
            txn.exec(query);
            VLOG(2) << "Create unique constraints for tables\n";
            txn.commit();
        } catch (const pqxx::sql_error &e) {
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
            return return_codes::error_occured;
        }
        catch (const pqxx::unexpected_rows &r) {
            VLOG(1) << "Database schemas have been already created, aborting creation";
            VLOG(2) << "    exception message: " << r.what();
            return return_codes::already_exists;
        }
        catch (const std::exception &e) {
            VLOG(1) << "Error setting up database schemas: " << e.what() << '\n';
            return return_codes::error_occured;
        }
        return return_sucess;
    }


}


#endif
