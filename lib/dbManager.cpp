#include "dbManager.h"

namespace db_services {
    void diconnect(db_services::conPtr &conn) {
        if (conn) {
            conn->close();
            conn = nullptr;
        }
    }

    int dbManager::clearSegments() {
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


    int dbManager::deleteDirectory(std::string_view directoryPath) {
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
            printRowsAffected(txn.exec(qr));
            VLOG(2) << vformat("Successfully reduced segment"
                               " counts for directory \"%s\"\n", directoryPath.data());


            query = "delete from public.data d where d.file_id in (%s);";

            qr = vformat(query.c_str(), idString.c_str());
            printRowsAffected(txn.exec(qr));

            VLOG(2) << vformat("Successfully deleted data "
                               "for directory \"%s\"\n", directoryPath.data());


            query = "delete from public.files where file_id in (%s);";
            qr = vformat(query.c_str(), idString.c_str());
            printRowsAffected(txn.exec(qr));

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


    int dbManager::deleteFile(std::string_view filePath, indexType fileId) {
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


    int dbManager::getFileStreamed(std::string_view fileName, std::ostream &output, indexType fileId) {
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

    int dbManager::insertFileFromStream(std::string_view fileName, std::istream &in, size_t segmentSize,
                                        std::size_t fileSize,const hash_function& hash) {
        try {
            trasnactionType txn(*conn_);

            auto hashStr = getHashStr(fileName);
            std::string tableName = vformat("\"temp_file_%s\"", hashStr.c_str());
            pqxx::stream_to copyStream = pqxx::stream_to::raw_table(txn, tableName);
            int blockIndex = 1;

            unsigned char buffer[segmentSize];

            size_t blockCount = fileSize / segmentSize;
            size_t lastBlockSize = fileSize - blockCount * segmentSize;
            std::istream::sync_with_stdio(false);

            unsigned char mmd[hash_function_size[hash]];

            for (int i = 0; i < blockCount; ++i) {
                in.read(reinterpret_cast<char *>(buffer), segmentSize);
                funcs[hash](buffer, segmentSize, mmd);
                copyStream
                        << std::make_tuple(
                                blockIndex++,
                                pqxx::binarystring(buffer, segmentSize),
                                pqxx::binarystring(mmd, hash_function_size[hash]));

            }
            if (lastBlockSize != 0) {
                std::string bff(lastBlockSize, '\0');
                in.read(bff.data(), segmentSize);
                funcs[hash](reinterpret_cast<const unsigned char *>(bff.data()), bff.size(),
                            mmd);
                copyStream
                        << std::make_tuple(
                                blockIndex,
                                pqxx::binary_cast(bff),
                                pqxx::binarystring(mmd, hash_function_size[hash]));
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


    int dbManager::finishFileProcessing(std::string_view filePath, indexType fileId) {
        try {
            trasnactionType txn(*conn_);
            auto hashStr = getHashStr(filePath);
            std::string tableName = vformat("temp_file_%s", hashStr.c_str());


            std::string aggregationTableName = vformat("new_segments_%s", hashStr.c_str());
            resType r;
            std::string q = vformat("SELECT * FROM pg_tables WHERE tablename = \'%s\'", tableName.c_str());
            txn.exec(q).one_row();


            q = vformat("CREATE TABLE  \"%s\" AS SELECT t.data, COUNT(t.data) AS count, t.hash "
                        "FROM \"%s\" t "
                        "GROUP BY t.data,t.hash;", aggregationTableName.c_str(), tableName.c_str());
            txn.exec(q);


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
            txn.exec(q);
            VLOG(2) << vformat("New segments were inserted for file \"%s\".", filePath.data());


            q = vformat("drop table \"%s\";", aggregationTableName.c_str());

            txn.exec(q);
            VLOG(2)
                            << vformat("Temporary aggregation table %s was deleted.", aggregationTableName.c_str());


            q = vformat("INSERT INTO public.data (segment_num, segment_hash, file_id) "
                        "SELECT tt.pos, tt.hash,  %d "
                        "FROM  \"%s\" tt ",
                        fileId,
                        tableName.c_str());
            txn.exec(q);
            VLOG(2) << vformat("Segment data of %s was inserted.", tableName.c_str());


            q = vformat("DROP TABLE IF EXISTS  \"%s\";", tableName.c_str());
            txn.exec(q);
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
        updateFileTime(fileId);
        return returnCodes::ReturnSucess;
    }


    indexType
    dbManager::createFile(std::string_view filePath, uintmax_t fileSize, size_t segmentSize, hash_function hash) {
        trasnactionType txn(*conn_);
        indexType futureFileId = returnCodes::ErrorOccured;

        std::string query;
        resType res;
        try {
            query = "insert into public.files (file_name,size_in_bytes,segment_size,hash_id)"
                    " values ($1,$2,$3,$4) returning file_id;";
            res = txn.exec(query, {toSpacedPath(filePath), fileSize, segmentSize, static_cast<unsigned>(hash)});

            futureFileId = res.one_row()[0].as<indexType>();
            VLOG(2) << vformat("File record (%d,\"%s\",%d) was successfully created.",
                               futureFileId,
                               filePath.data(),
                               fileSize);
            //todo refactor message
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
            VLOG(1) << "Error while creating file:" << e.what() << '\n';
        }
        return futureFileId;
    }


    std::vector<std::pair<indexType, std::string>> dbManager::getAllFiles(std::string_view dirPath) {
        std::vector<std::pair<indexType, std::string>> result;
        try {
            trasnactionType txn(*conn_);
            auto res = getEntriesForDirectory(txn, dirPath);

            VLOG(2)
                            << vformat("Filenames were successfully retrieved for directory \"%s\".", dirPath.data());
            for (const auto &re: res) {
                auto tres = re[1].as<std::string>();
                if(!re[6].is_null()) {
                    result.emplace_back(re[0].as<indexType>(), fromSpacedPath(tres));
                }
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


    int dbManager::connectToDb() {
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


    int dbManager::dropDatabase(std::string_view dbName) {
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


    int dbManager::createDatabase(std::string_view dbName) {
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


    int dbManager::fillSchemas() {
        try {

            trasnactionType txn(*conn_);
            checkSchemas(txn).no_rows();

            std::string query = vformat("create schema if not exists public;"
                                        "create table public.hash_functions"
                                        "("
                                        "    hash_id SMALLINT NOT NULL primary key,"
                                        "    hash_name text NOT NULL,"
                                        "    digest_size SMALLINT NOT NULL"
                                        ");"
                                        "create table public.segments"
                                        "("
                                        "    segment_hash bytea NOT NULL primary key,"
                                        "    segment_data bytea NOT NULL,"
                                        "    segment_count bigint NOT NULL"
                                        ");");

            txn.exec(query);
            VLOG(2) << vformat("Create segments table\n");

            pqxx::stream_to copyStream = pqxx::stream_to::raw_table(txn, "hash_functions");

            for (size_t i = 0; i < hash_function_name.size(); i++) {
                copyStream
                        << std::make_tuple(
                                i,
                                hash_function_name[i],
                                hash_function_size[i]);
            }
            copyStream.complete();

            VLOG(2) << vformat("Hashes table populated\n");

            query = "create table public.files "
                    "("
                    "    file_id serial primary key NOT NULL,"
                    "    file_name text NOT NULL,"
                    "    size_in_bytes bigint NULL,"
                    "    segment_size bigint NOT NULL,"
                    "    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
                    "    processed_at TIMESTAMP NULL,"
                    "    hash_id SMALLINT"
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
                    /*"create index if not exists hash_index on public.segments(segment_hash);"*/
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

    indexType dbManager::createDirectory(std::string_view dirPath) {
        trasnactionType txn(*conn_);
        indexType futureFileId = returnCodes::ErrorOccured;

        std::string query;
        resType res;
        try {
            query = "insert into public.files (file_name,size_in_bytes,segment_size)"
                    " values ($1,0,0) returning file_id;";
            res = txn.exec(query, pqxx::params{toSpacedPath(dirPath)});

            futureFileId = res.one_row()[0].as<indexType>();
            VLOG(2) << vformat("Directory record (%d,\"%s\",0) was successfully created.",
                               futureFileId,
                               dirPath.data());

            txn.commit();
        }
        catch (const pqxx::sql_error &e) {
            if (e.sqlstate() == sqlLimitBreachedState) {
                VLOG(1) << vformat("Directory %s already exists\n", dirPath.data());
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


}
