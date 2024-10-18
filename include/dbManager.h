#ifndef DATA_DEDUPLICATION_SERVICE_DBMANAGER_H
#define DATA_DEDUPLICATION_SERVICE_DBMANAGER_H

#include <iostream>
#include <vector>
#include <utility>
#include <string>

#include <pqxx/pqxx>

#include "myConcepts.h"
#include "dbCommon.h"
#include "HashUtils.h"

/// db_services namespace
using myConcepts::paramType,myConcepts::gClk;
using enum myConcepts::paramType;
using namespace hash_utils;

namespace db_services {

    /**
     * Closes and deletes connection
     * @param conn
     */
     void diconnect(conPtr &conn);

    /**
     * Database manager that handles database management
     * @tparam segment_size
     */
    template<unsigned long segment_size>
    class dbManager {
    public:
        dbManager() : cString_(db_services::defaultConfiguration()), conn_(nullptr) {}

        explicit dbManager(myConnString &ss) : cString_(ss), conn_(nullptr) {}

        void setCString(myConnString &ss) {
            cString_ = ss;
            disconnect();
        }

        [[nodiscard]] const myConnString &getCString() const {
            return cString_;
        }

        int connectToDb();

        void disconnect() {
            db_services::diconnect(conn_);
        }

        //inpired by https://stackoverflow.com/questions/49122358/libbqxx-c-api-to-connect-to-postgresql-without-db-name
        /**
         * Creates database entry and connect to it
         * @param dbName
         */
        int createDatabase(std::string_view dbName);

        /**
         * Deletes database entry
         * @param dbName
         */
        int dropDatabase(std::string_view dbName);

        /**
         * Create main database tables and indexes
         */
        int fillSchemas();

        /**
         * Removes segments that are not used in files
         */
        int clearSegments();

        /**
         * Get vector of pairs of file_names and file ids for directory
         * @param dirPath
         */
        std::vector<std::pair<indexType, std::string>> getAllFiles(std::string_view dirPath);

        /**
         * Creates entry for a given file
         * @param filePath
         * @param fileSize
         * @return
         */
        indexType createFile(std::string_view filePath, int fileSize = 0);


        /**
         * Recursevely deletes all file data
         * @param filePath
         * @param fileId
         */
        int deleteFile(std::string_view filePath, indexType fileId = paramType::EmptyParameterValue);

        /**
         * Recursevely deletes all file that are in directory
         * @param directoryPath
         */
        int deleteDirectory(std::string_view directoryPath);

        /**
         * Recreates file conents and bulk insert the in out
         * @param fileName
         * @param output
         * @param fileId
         */
        int getFileStreamed(std::string_view fileName, std::ostream &output,
                            indexType fileId = paramType::EmptyParameterValue);

        /**
         * Bulk insert file segments into temporary table
         * @tparam hash hash function that will be used for hashing
         * @param fileName
         * @param in
         * @param fileSize
         */
        template<hash_function hash = SHA_256>
        int insertFileFromStream(std::string_view fileName, std::istream &in, std::size_t fileSize);

        /**
         * Process bulk inserted file data
         * @param filePath
         * @param fileId
        */
        int finishFileProcessing(std::string_view filePath, indexType fileId);

        bool checkConnection() {
            return db_services::checkConnection(conn_);
        }

        ~dbManager() {
            disconnect();
        }

        /**
         * Wraps function in transaction block
         * @tparam ResultType
         * @tparam Args
         * @param call
         * @param args
         */
        template<typename ResultType, typename ... Args>
        tl::expected<ResultType, int>
        executeInTransaction(ResultType (*call)(trasnactionType &, Args ...), Args &&... args) {
            trasnactionType txn(*conn_);
            ResultType res = call(txn, std::forward<Args>(args)...);
            txn.commit();
            return res;
        }

        /**
         * Functional variant of previous call
         * @tparam ResultType
         * @tparam Args
         * @param call
         * @param args
         */
        template<typename ResultType, typename ... Args>
        tl::expected<ResultType, int>
        executeInTransaction(const std::function<ResultType(trasnactionType &, Args ...)> &call, Args &&... args) {
            trasnactionType txn(*conn_);
            ResultType res = call(txn, std::forward<Args>(args)...);
            txn.commit();
            return res;
        }

    private:
        myConnString cString_;
        conPtr conn_;
    };

    template<unsigned long segment_size>
    int dbManager<segment_size>::clearSegments() {
        try {
            trasnactionType txn(*conn_);

            deleteUnusedSegments(txn);
            txn.commit();
            VLOG(2) << vformat("Successfully deleted redundant "
                               "segments");
        } catch (const pqxx::sql_error &e) {
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
            return returnCodes::ErrorOccured;
        } catch (const std::exception &e) {
            VLOG(1) << "Error while delting segments :" << e.what() << '\n';
            return returnCodes::ErrorOccured;
        }
        return ReturnSucess;
    }


    template<unsigned long segment_size>
    int dbManager<segment_size>::deleteDirectory(std::string_view directoryPath) {
        try {
            trasnactionType txn(*conn_);

            auto fileIds = getFileIdVector(txn, directoryPath);
            auto idString = vecToString(fileIds);


            std::string query, qr;
            resType res;

            query = "update public.segments s "
                    "set segment_count=segment_count-ss.cnt "
                    "from (select segment_hash as hhhash, count(segment_hash) as cnt "
                    "      from public.data inner join public.files f on f.file_id = public.data.file_id "
                    "      where f.file_id in (%s) "
                    "      group by file_name, segment_hash "
                    ") as ss "
                    "where ss.hhhash=segment_hash;";
            qr = vformat(query.c_str(), idString.c_str());
            gClk.tik();
            printRowsAffected(txn.exec(qr));
            gClk.tak();
            VLOG(2) << vformat("Successfully reduced segment"
                               " counts for directory \"%s\"\n", directoryPath.data());


            query = "delete from public.data d where d.file_id in (%s);";

            qr = vformat(query.c_str(), idString.c_str());
            gClk.tik();
            printRowsAffected(txn.exec(qr));
            gClk.tak();

            VLOG(2) << vformat("Successfully deleted data "
                               "for directory \"%s\"\n", directoryPath.data());


            query = "delete from public.files where file_id in (%s);";
            qr = vformat(query.c_str(), idString.c_str());
            gClk.tik();
            printRowsAffected(txn.exec(qr));
            gClk.tak();

            VLOG(2) << vformat("Successfully deleted public.files "
                               " for directory \"%s\"\n", directoryPath.data());
            txn.commit();

        } catch (const pqxx::sql_error &e) {
            if (e.sqlstate() == sqlFqConstraight) {
                VLOG(1) << vformat("Unable to remove record for directory \"%s\" since files "
                                   "for it are stile present.", directoryPath.data());
                return returnCodes::WarningMessage;
            }
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
            return returnCodes::ErrorOccured;
        } catch (const std::exception &e) {
            VLOG(1) << "Error while delting directory:" << e.what() << '\n';
            return returnCodes::ErrorOccured;
        }
        return ReturnSucess;
    }


    template<unsigned long segment_size>
    int dbManager<segment_size>::deleteFile(std::string_view filePath, indexType fileId) {
        try {

            trasnactionType txn(*conn_);
            if (fileId == paramType::EmptyParameterValue) {
                fileId = getFileId(txn, filePath);
            }
            std::string query, qr;
            resType res;

            //todo it's not required here
            auto hashStr = getHashStr(filePath);
            std::string tableName = vformat("temp_file_%s", hashStr.c_str());
            qr = vformat("DROP TABLE IF EXISTS  \"%s\";", tableName.c_str());
            res = txn.exec(qr);
            VLOG(3) << res.query();


            query = "update public.segments s "
                    "set segment_count=segment_count-ss.cnt "
                    "from (select segment_hash as hhhash, count(segment_hash) as cnt "
                    "      from public.data "
                    "      where file_id = %d "
                    "      group by file_id, segment_hash "
                    ") as ss "
                    "where ss.hhhash=segment_hash;";

            qr = vformat(query.c_str(), fileId);

            printRowsAffected(txn.exec(qr));

            VLOG(2) << vformat("Successfully reduced segments"
                               " counts for file \"%s\"\n", filePath.data());


            query = "delete from public.data where file_id=%d;";

            qr = vformat(query.c_str(), fileId);

            printRowsAffected(txn.exec(qr));

            VLOG(2) << vformat("Successfully deleted data "
                               "for file \"%s\"\n", filePath.data());

            query = "delete from public.files where file_id=%d;";
            qr = vformat(query.c_str(), fileId);
            printRowsAffected(txn.exec(qr));


            VLOG(2) << vformat("Successfully deleted file "
                               "entry for \"%s\"\n", filePath.data());


            txn.commit();
        } catch (const pqxx::sql_error &e) {
            if (e.sqlstate() == sqlFqConstraight) {
                VLOG(1) << vformat("Unable to remove record for file \"%s\" since segment data "
                                   "for it are stile present.", filePath.data());
                return returnCodes::WarningMessage;
            }
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
            return returnCodes::ErrorOccured;
        } catch (const std::exception &e) {
            VLOG(1) << "Error while deleting file:" << e.what() << '\n';
            return returnCodes::ErrorOccured;
        }
        return ReturnSucess;
    }


    template<unsigned long segment_size>
    int dbManager<segment_size>::getFileStreamed(std::string_view fileName, std::ostream &output, indexType fileId) {
        try {

            trasnactionType txn(*conn_);
            if (fileId == paramType::EmptyParameterValue) {
                fileId = db_services::getFileId(txn, fileName);
            }
            std::string query = vformat("select s.segment_data "
                                        "from public.data"
                                        "         inner join public.segments s on s.segment_hash "
                                        "                                         = public.data.segment_hash "
                                        "        inner join public.files f on f.file_id = public.data.file_id "
                                        "where f.file_id=%d "
                                        "order by segment_num", fileId);
            for (auto [name]: txn.stream<pqxx::binarystring>(query)) {
                output << name.str();
            }
            txn.commit();


        } catch (const pqxx::sql_error &e) {
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
            return returnCodes::ErrorOccured;
        } catch (const std::exception &e) {
            VLOG(1) << "Error inserting block data:" << e.what() << '\n';
            return returnCodes::ErrorOccured;
        }
        return returnCodes::ReturnSucess;
    }


    template<unsigned long segment_size>
    template<hash_function hash>
    int dbManager<segment_size>::insertFileFromStream(std::string_view fileName, std::istream &in,
                                                      std::size_t fileSize) {
        try {
            trasnactionType txn(*conn_);

            auto hashStr = getHashStr(fileName);
            std::string tableName = vformat("\"temp_file_%s\"", hashStr.c_str());
            pqxx::stream_to copyStream = pqxx::stream_to::raw_table(txn, tableName);
            int blockIndex = 1;

            //std::string buffer(segment_size, '\0');
            unsigned char buffer[segment_size];

            size_t blockCount = fileSize / segment_size;
            size_t lastBlockSize = fileSize - blockCount * segment_size;
            std::istream::sync_with_stdio(false);

            unsigned char mmd[hash_function_size[hash]];

            for (int i = 0; i < blockCount; ++i) {
                in.read(reinterpret_cast<char *>(buffer), segment_size);
                funcs[hash](buffer, segment_size, mmd);
                copyStream
                        << std::make_tuple(
                                blockIndex++,
                                pqxx::binarystring(buffer, segment_size),
                                pqxx::binarystring(mmd, hash_function_size[hash]));

            }
            if (lastBlockSize != 0) {
                std::string bff(lastBlockSize, '\0');
                in.read(bff.data(), segment_size);
                funcs[hash](reinterpret_cast<const unsigned char *>(bff.data()), bff.size(),
                           mmd);
                copyStream
                        << std::make_tuple(
                                blockIndex,
                                pqxx::binary_cast(bff), pqxx::binarystring(mmd, hash_function_size[hash]));
            }
            copyStream.complete();
            txn.commit();
        } catch (const pqxx::sql_error &e) {
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
            return returnCodes::ErrorOccured;
        } catch (const std::exception &e) {
            VLOG(1) << "Error inserting block data:" << e.what() << '\n';
            return returnCodes::ErrorOccured;
        }
        return ReturnSucess;
    }


    template<unsigned long segment_size>
    int dbManager<segment_size>::finishFileProcessing(std::string_view filePath, indexType fileId) {
        try {
            trasnactionType txn(*conn_);
            auto hashStr = getHashStr(filePath);
            std::string tableName = vformat("temp_file_%s", hashStr.c_str());


            std::string aggregationTableName = vformat("new_segments_%s", hashStr.c_str());
            resType r;
            std::string q = vformat("SELECT * FROM pg_tables WHERE tablename = \'%s\'", tableName.c_str());
            gClk.tik();
            txn.exec(q).one_row();
            gClk.tak();
            q = vformat("CREATE TABLE  \"%s\" AS SELECT t.data, COUNT(t.data) AS count, t.hash "
                        "FROM \"%s\" t "
                        "GROUP BY t.data,t.hash;", aggregationTableName.c_str(), tableName.c_str());
            gClk.tik();
            txn.exec(q);
            gClk.tak();
            VLOG(2) << vformat(
                    "Segment data was aggregated into %s for file %s.",
                    aggregationTableName.c_str(),
                    filePath.data());


            q = vformat("INSERT INTO public.segments (segment_data, segment_count, segment_hash) "
                        "SELECT ns.data, ns.count,ns.hash "
                        "FROM \"%s\" ns "
                        "ON CONFLICT (segment_hash) "
                        "DO UPDATE "
                        "SET segment_count = public.segments.segment_count +  excluded.segment_count;",
                        aggregationTableName.c_str());
            gClk.tik();
            txn.exec(q);
            gClk.tak();
            VLOG(2) << vformat("New segments were inserted for file \"%s\".", filePath.data());
            q = vformat("drop table \"%s\";", aggregationTableName.c_str());
            gClk.tik();
            txn.exec(q);
            gClk.tak();
            VLOG(2)
                            << vformat("Temporary aggregation table %s was deleted.", aggregationTableName.c_str());


            q = vformat("INSERT INTO public.data (segment_num, segment_hash, file_id) "
                        "SELECT tt.pos, tt.hash,  %d "
                        "FROM  \"%s\" tt ",
                        fileId,
                        tableName.c_str());
            gClk.tik();
            txn.exec(q);
            gClk.tak();
            VLOG(2) << vformat("Segment data of %s was inserted.", tableName.c_str());


            q = vformat("DROP TABLE IF EXISTS  \"%s\";", tableName.c_str());
            gClk.tik();
            txn.exec(q);
            gClk.tak();
            VLOG(2) << vformat("Temp data table %s was deleted.", tableName.c_str());

            txn.commit();
        } catch (const pqxx::sql_error &e) {
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
            return returnCodes::ErrorOccured;
        }
        catch (const pqxx::unexpected_rows &r) {
            VLOG(1) << vformat("No data found for file \"%s\"", filePath.data());
            VLOG(2) << vformat("Exception description: %s", r.what()) << '\n';
            return returnCodes::ErrorOccured;
        }
        catch (const std::exception &e) {
            VLOG(1) << "Error processing file data:" << e.what() << '\n';
            return returnCodes::ErrorOccured;
        }
        return returnCodes::ReturnSucess;
    }


    template<unsigned long segment_size>
    indexType dbManager<segment_size>::createFile(std::string_view filePath, int fileSize) {
        trasnactionType txn(*conn_);
        indexType futureFileId = returnCodes::ErrorOccured;

        std::string query;
        resType res;
        try {
            query = "insert into public.files (file_name,size_in_bytes)"
                    " values ($1,$2) returning file_id;";
            res = txn.exec(query, {toSpacedPath(filePath), fileSize});

            futureFileId = res.one_row()[0].as<indexType>();
            VLOG(2) << vformat("File record (%d,\"%s\",%d) was successfully created.",
                               futureFileId,
                               filePath.data(),
                               fileSize);
            auto hashStr = getHashStr(filePath);
            std::string tableName = vformat("temp_file_%s", hashStr.c_str());
            std::string q1 = vformat(
                    "CREATE TABLE \"%s\" (pos bigint, data bytea,hash bytea);",
                    tableName.c_str()
            );
            txn.exec(q1);

            txn.commit();

            VLOG(2) << vformat("Temp data table %s was created.", tableName.c_str());
        }
        catch (const pqxx::sql_error &e) {
            if (e.sqlstate() == sqlLimitBreachedState) {
                VLOG(1) << vformat("File %s already exists\n", filePath.data());
                return returnCodes::AlreadyExists;
            }
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
        }
        catch (const std::exception &e) {
            VLOG(1) << "Error while creting file:" << e.what() << '\n';
        }
        return futureFileId;
    }

    template<unsigned long segment_size>
    std::vector<std::pair<indexType, std::string>> dbManager<segment_size>::getAllFiles(std::string_view dirPath) {
        std::vector<std::pair<indexType, std::string>> result;
        try {
            trasnactionType txn(*conn_);
            auto res = getFilesForDirectory(txn, dirPath);

            VLOG(2)
                            << vformat("Filenames were successfully retrieved for directory \"%s\".", dirPath.data());
            for (const auto &re: res) {
                auto tres = re[1].as<std::string>();
                result.emplace_back(re[0].as<indexType>(), fromSpacedPath(tres));
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
    int dbManager<segment_size>::connectToDb() {
        if (checkConnection()) {
            return returnCodes::ReturnSucess;
        }
        VLOG(1) << "Failed to listen with current connection, attempt to reconnect via cString\n";
        auto result = connectIfPossible(cString_);

        conn_ = result.value_or(nullptr);
        if (!result.has_value()) {
            return result.error();
        }
        return returnCodes::ReturnSucess;
    }


    template<unsigned long segment_size>
    int dbManager<segment_size>::dropDatabase(std::string_view dbName) {
        if (checkConnection()) {
            conn_->close();
        }

        conn_ = nullptr;
        cString_.setDbname(dbName);

        auto tString = cString_;
        tString.setDbname(sampleTempDb);

        auto result = connectIfPossible(tString);

        auto tempConnection = result.value_or(nullptr);

        if (!result.has_value()) {
            VLOG(1)
                            << vformat("Unable to connect by url \"%s\"\n", (tString).operator std::string().c_str());
            return returnCodes::ErrorOccured;
        }
        VLOG(1) << vformat("Connected to database %s\n", tString.getDbname().c_str());


        nonTransType noTransExec(*tempConnection);
        std::string qq;
        try {
            checkDatabaseExistence(noTransExec, cString_.getDbname()).one_row();

            terminateAllDbConnections(noTransExec, cString_.getDbname());


            qq = vformat("DROP DATABASE \"%s\";",
                         cString_.getDbname().c_str());
            noTransExec.exec(qq);

            noTransExec.commit();

            VLOG(2) << "Database deleted successfully: " << cString_.getDbname();
        } catch (const pqxx::sql_error &e) {
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
            return returnCodes::ErrorOccured;
        }
        catch (const pqxx::unexpected_rows &r) {
            VLOG(1) << vformat("No database %s found! Abort drop! ", cString_.getDbname().c_str());
            VLOG(2) << "    exception message: " << r.what();
            noTransExec.abort();
            tempConnection->close();
            conn_ = connectIfPossible(cString_).value_or(nullptr);
            return returnCodes::WarningMessage;
        }
        catch (const std::exception &e) {
            VLOG(1) << "Error: " << e.what() << '\n';
            return returnCodes::ErrorOccured;
        }


        tempConnection->close();

        return ReturnSucess;
    }


    template<unsigned long segment_size>
    int dbManager<segment_size>::createDatabase(std::string_view dbName) {
        conn_ = nullptr;

        cString_.setDbname(dbName);

        auto tString = cString_;
        tString.setDbname(sampleTempDb);

        auto result = connectIfPossible(tString);

        auto tempConnection = result.value_or(nullptr);

        if (!result.has_value()) {
            VLOG(1)
                            << vformat("Unable to connect by url \"%s\"\n", (tString).operator std::string().c_str());
            return returnCodes::ErrorOccured;
        }
        VLOG(1) << vformat("Connected to database %s\n", tString.getDbname().c_str());


        nonTransType noTransExec(*tempConnection);

        try {
            checkDatabaseExistence(noTransExec, cString_.getDbname()).no_rows();

            noTransExec.exec(vformat("CREATE DATABASE \"%s\";",
                                     cString_.getDbname().c_str()));

            noTransExec.exec(vformat("GRANT ALL ON DATABASE \"%s\" TO %s;",
                                     cString_.getDbname().c_str(),
                                     cString_.getUser().c_str()));
            noTransExec.commit();

            VLOG(2) << "Database created successfully: " << cString_.getDbname() << '\n';
        } catch (const pqxx::sql_error &e) {
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
            return returnCodes::ErrorOccured;
        }
        catch (const pqxx::unexpected_rows &r) {
            VLOG(1) << "Database already exists, aborting creation";
            VLOG(2) << "    exception message: " << r.what();
            noTransExec.abort();
            tempConnection->close();
            conn_ = connectIfPossible(cString_).value_or(nullptr);
            return returnCodes::AlreadyExists;
        }
        catch (const std::exception &e) {
            VLOG(1) << "Error: " << e.what() << '\n';
            return returnCodes::ErrorOccured;
        }


        tempConnection->close();
        conn_ = connectIfPossible(cString_).value_or(nullptr);
        return returnCodes::ReturnSucess;
    }


    template<unsigned long segment_size>
    int dbManager<segment_size>::fillSchemas() {
        try {

            trasnactionType txn(*conn_);
            checkSchemas(txn).no_rows();

            std::string query = vformat("create schema if not exists public;"
                                        "create table public.segments"
                                        "("
                                        "    segment_hash bytea NOT NULL primary key,"
                                        "    segment_data bytea NOT NULL,"
                                        "    segment_count bigint NOT NULL"
                                        ");");
            txn.exec(query);
            VLOG(2) << vformat("Create segments table\n");

            query = "create table public.files "
                    "("
                    "    file_id serial primary key NOT NULL,"
                    "    file_name text NOT NULL,"
                    "    size_in_bytes bigint NULL"
                    ");"
                    ""
                    "create table public.data "
                    "("
                    "    file_id int NOT NULL,"
                    "    segment_num bigint NOT NULL,"
                    "    segment_hash bytea  NOT NULL"
                    ");";
            txn.exec(query);
            VLOG(2) << "Create directories, files and data tables\n";


            query = "CREATE INDEX if not exists segment_count on public.segments(segment_count); "
                    "CREATE INDEX  if not exists bin_file_id on public.data(file_id); "
                    ""
                    ""
                    "create index if not exists gin_f_name ON files USING GIN "
                    "(to_tsvector('simple',replace(file_name,'_', '/')));";
            txn.exec(query);
            VLOG(2) << "Create indexes for main tables\n";


            query =
                    "ALTER TABLE public.files ADD CONSTRAINT unique_file_constr UNIQUE(file_name); ";
            txn.exec(query);
            VLOG(2) << "Create unique constraints for tables\n";
            txn.commit();
        } catch (const pqxx::sql_error &e) {
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
            return returnCodes::ErrorOccured;
        }
        catch (const pqxx::unexpected_rows &r) {
            VLOG(1) << "Database schemas have been already created, aborting creation";
            VLOG(2) << "    exception message: " << r.what();
            return returnCodes::AlreadyExists;
        }
        catch (const std::exception &e) {
            VLOG(1) << "Error setting up database schemas: " << e.what() << '\n';
            return returnCodes::ErrorOccured;
        }
        return ReturnSucess;
    }


}


#endif //DATA_DEDUPLICATION_SERVICE_DBMANAGER_H
