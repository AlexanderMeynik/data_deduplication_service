#ifndef DATA_DEDUPLICATION_SERVICE_DBCOMMON_H
#define DATA_DEDUPLICATION_SERVICE_DBCOMMON_H

#include <fstream>
#include <functional>
#include <memory>

#include <pqxx/pqxx>

#include "expected.hpp"
#include "myConnString.h"
#include "myConcepts.h"
#include "HashUtils.h"


/// db_services namespace
namespace db_services {

    using myConcepts::printable, myConcepts::returnCodes, myConcepts::vformat, hash_utils::getHashStr;
    using
    enum myConcepts::returnCodes;
#ifdef IT_test
    static inline std::string resDirPath = "../../conf/";
#else
    static inline std::string resDirPath = "../../conf/";
#endif

    ///  default configuration file path
    static inline std::string cfileName = resDirPath.append("config.txt");
    static constexpr const char *const sqlLimitBreachedState = "23505";
    static constexpr const char *const sqlFqConstraight = "23503";
    static const char *const sampleTempDb = "template1";

    using indexType = int64_t;
    using trasnactionType = pqxx::transaction<pqxx::isolation_level::read_committed>;
    using connectionType = pqxx::connection;
    using conPtr = std::shared_ptr<connectionType>;
    using resType = pqxx::result;
    using nonTransType = pqxx::nontransaction;


    /**
     * Swaps / symbols for spaces
     * @param path
     */
    std::string toSpacedPath(std::string_view path);

    /**
     * Swaps space syblos for /
     * @param path
     */
    std::string fromSpacedPath(std::string_view path);

    /**
     * Replaces symbols in path to cast it to tsquery
     * @param path
     */
    std::string toTsquerablePath(std::string_view path);

    /**
     * Checks that conn is not null and opened
     * @param conn
     */
    bool checkConnection(const conPtr &conn);

    /**
     * Checks connection  for connString
     * @param connString
     */
    bool checkConnString(const myConnString &connString);

    /**
     * Connects to database with cString
     * @param cString
     */
    tl::expected <conPtr, returnCodes> connectIfPossible(std::string_view cString);

    /**
     * Terminates all connections to dbName via sql query
     * @param noTransExec
     * @param dbName
     */
    resType terminateAllDbConnections(nonTransType &noTransExec, std::string_view dbName);

    /**
     * Checks database existence
     * @param noTransExec
     * @param dbName
     */
    resType checkDatabaseExistence(nonTransType &noTransExec, std::string_view dbName);

    /**
     * Returns all schemas for database
     * @param txn
     */
    resType checkSchemas(trasnactionType &txn);

    /**
     * Compares segments counts from segments table to total counst from data table
     * @param txn
     */
    indexType checkSegmentCount(trasnactionType &txn);

    /**
     * Deletes segments with count=0
     * @param txn
     * @return
     */
    resType deleteUnusedSegments(trasnactionType &txn);

    /**
     * Checks file existence
     * @param txn
     * @param fileName
     */
    resType checkFileExistence(trasnactionType &txn, std::string_view fileName);

    /**
    * Multifile variant for @ref db_services::checkFileExistence() "checkFileExistence"
    * @param txn
    * @param files
    */
    [[deprecated]]resType checkFilesExistence(trasnactionType &txn, const std::vector<std::filesystem::path> &files);

    /**
     * Parses one file id from request
     * @param txn
     * @param fileName
     * @see @ref db_services::checkFileExistence() "checkFileExistence"
     */
    indexType getFileId(trasnactionType &txn, std::string_view fileName);

    /**
     * Bool variant for existence checks
     * @param txn
     * @param fileName
     * @see @ref db_services::getFileId() "getFileId"
     */
    bool doesFileExist(trasnactionType &txn, std::string_view fileName);

    /**
     * Returns resType that contains all file/subdirectory entries for the selected directory
     * @param txn
     * @param dirPath
     */
    resType getEntriesForDirectory(trasnactionType &txn, std::string_view dirPath);

    /**
     * Vector returning wrapper for @ref db_services::getEntriesForDirectory() "getEntriesForDirectory
     * @param txn
     * @param dirPath
     */
    std::vector<indexType> getFileIdVector(trasnactionType &txn, std::string_view dirPath);

    /**
     * Calculates sum of files sizes
     * @param txn
     */
    tl::expected<indexType, int> getTotalFileSize(trasnactionType &txn);

    /**
     * Gets table thta contains schemas sizes for database
     * @param txn
     */
    resType getTotalSchemaSizes(trasnactionType &txn);

    /**
     * Calculates main deduplication characteristics for files
     * @param txn
     */
    resType getDedupCharacteristics(trasnactionType &txn);

    [[deprecated]]resType getFileSizes(trasnactionType &txn);

    /**
     * Peinrst resType to ostream
     * @param rss
     * @param out
     */
    void printRes(resType &rss, std::ostream &out);

    /**
     * @tparam T
     * @param vec
     */
    template<printable T>
    std::string vecToString(std::vector<T> &vec);

    /**
     * Gets affected rows for delete/update query result
     * @param res
     */
    void printRowsAffected(const resType &res);

    /**
     * Checks whether temporary table was created for file
     * @param txn
     * @param fileName
     */
    resType checkTExistence(db_services::trasnactionType &txn, std::string_view fileName);

    /**
     * Loads configuration from file
     * @param filename
     */
    myConnString loadConfiguration(std::string_view filename);

    auto static defaultConfiguration = []() {
        return loadConfiguration(cfileName);
    };

    template<typename T, unsigned long size>
    std::array<T, size> fromString(std::basic_string<T> &string) {
        std::array<T, size> res;
        for (int i = 0; i < size; ++i) {
            res[i] = string[i];
        }
        return res;
    }

    template<printable T>
    std::string vecToString(std::vector<T> &vec) {
        std::stringstream ss;
        if (vec.empty()) {
            return ss.str();
        }
        int i = 0;
        for (; i < vec.size() - 1; ++i) {
            ss << vec[i] << ',';
        }
        ss << vec[i];
        return ss.str();
    }


    /**
     * Wraps function in transaction block
     * @tparam ResultType
     * @tparam Args
     * @param conn not null connection
     * @param call
     * @param args
     */
    template<typename ResultType, typename ... Args>
    tl::expected<ResultType, int>
    executeInTransaction(conPtr &conn, ResultType (*call)(trasnactionType &, Args ...), Args &&... args) {
        trasnactionType txn(*conn);//todo if conn is null this one will segfault
        ResultType res = call(txn, std::forward<Args>(args)...);
        txn.commit();
        return res;
    }

    /**
     * Functional variant of previous call
     * @tparam ResultType
     * @tparam Args
     * @param conn  not null connection
     * @param call
     * @param args
     */
    template<typename ResultType, typename ... Args>
    tl::expected<ResultType, int>
    executeInTransaction(conPtr &conn, const std::function<ResultType(trasnactionType &, Args ...)> &call,
                         Args &&... args) {
        trasnactionType txn(*conn);
        ResultType res = call(txn, std::forward<Args>(args)...);
        txn.commit();
        return res;
    }

}

#endif //DATA_DEDUPLICATION_SERVICE_DBCOMMON_H
