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


using myConcepts::paramType;
using
enum myConcepts::paramType;
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
     * @brief Database manager that handles database management
     * @details This class is designed to handle main interactions for database.
     * @details You can connect/create/deelte databases.
     * @details You can send requests for file load/delete/export.
     * @details Some external helper functions can be found in dbCommon.h
     */
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
        indexType createFile(std::string_view filePath, uintmax_t fileSize = 0, size_t segmentSize = 0,
                             hash_function hash = SHA_256);

        /**
         * Creates entry for directory
         * @param dirPath
         * @return
         */
        indexType createDirectory(std::string_view dirPath);

        /**
         * Recursively deletes all file data
         * @param filePath
         * @param fileId
         */
        int deleteFile(std::string_view filePath, indexType fileId = paramType::EmptyParameterValue);

        /**
         * Recursively deletes all file data for selected directory
         * @param directoryPath
         */
        int deleteDirectory(std::string_view directoryPath);

        /**
         * Recreates file contents and bulk insert them in out stream
         * @param fileName
         * @param output
         * @param fileId
         */
        int getFileStreamed(std::string_view fileName, std::ostream &output,
                            indexType fileId = paramType::EmptyParameterValue);

        /**
         * Bulk insert file segments into temporary table
         * @param fileName
         * @param in
         * @param fileSize
         * @param hash hash function that will be used for hashing
         */
        int insertFileFromStream(std::string_view fileName, std::istream &in, size_t segmentSize, std::size_t fileSize,
                                 const hash_function& hash=SHA_256);

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
            return db_services::executeInTransaction(conn_, call, std::forward<Args>(args)...);
        }

        /**
         * Functional variant of previous function
         * @tparam ResultType
         * @tparam Args
         * @param call
         * @param args
         */
        template<typename ResultType, typename ... Args>
        tl::expected<ResultType, int>
        executeInTransaction(const std::function<ResultType(trasnactionType &, Args ...)> &call, Args &&... args) {
            return db_services::executeInTransaction(conn_, call, std::forward<Args>(args)...);
        }

    private:
        /**
         * Updates processed at time for file
         * @param fileId
         */
        int updateFileTime(indexType fileId);

        myConnString cString_;
        conPtr conn_;

    };




}


#endif //DATA_DEDUPLICATION_SERVICE_DBMANAGER_H
