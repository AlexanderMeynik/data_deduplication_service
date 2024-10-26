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


using myConcepts::paramType/*, myConcepts::gClk*/;
using enum myConcepts::paramType;
using namespace hash_utils;


/// db_services namespace
namespace db_services {
    using myConcepts::gClk;
    /**
     * Closes and deletes connection
     * @param conn
     */
     void diconnect(conPtr &conn);

    /**
     * Database manager that handles database management
     */
    class dbManager {
    public:
        dbManager(): cString_(db_services::defaultConfiguration()), conn_(nullptr) {}

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
         * Recursively deletes all file that are in directory
         * @param directoryPath
         */
        int deleteDirectory(std::string_view directoryPath);

        /**
         * Recreates file contents and bulk insert the in out
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
        int insertFileFromStream(size_t segmentSize, std::string_view fileName, std::istream &in, std::size_t fileSize);

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
            return db_services::executeInTransaction(conn_,call,std::forward<Args>(args)...);
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
            return db_services::executeInTransaction(conn_,call,std::forward<Args>(args)...);
        }

    private:
        myConnString cString_;
        conPtr conn_;
    };


    template<hash_function hash>
    int dbManager::insertFileFromStream(size_t segmentSize, std::string_view fileName, std::istream &in,
                                        std::size_t fileSize) {
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



}


#endif //DATA_DEDUPLICATION_SERVICE_DBMANAGER_H
