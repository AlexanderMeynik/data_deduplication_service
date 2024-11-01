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


    std::string toSpacedPath(std::string_view path);

    std::string fromSpacedPath(std::string_view path);

    std::string toTsquerablePath(std::string_view path);

    bool checkConnection(const conPtr &conn);

    resType terminateAllDbConnections(nonTransType &noTransExec, std::string_view dbName);

    resType checkDatabaseExistence(nonTransType &noTransExec, std::string_view dbName);

    resType checkSchemas(trasnactionType &txn);

    indexType checkSegmentCount(trasnactionType &txn);

    resType deleteUnusedSegments(trasnactionType &txn);

    resType checkFileExistence(trasnactionType &txn, std::string_view fileName);

    indexType getFileId(trasnactionType &txn, std::string_view fileName);

    bool doesFileExist(trasnactionType &txn, std::string_view fileName);

    resType getEntriesForDirectory(trasnactionType &txn, std::string_view dirPath);

    std::vector<indexType> getFileIdVector(trasnactionType &txn, std::string_view dirPath);

    tl::expected<indexType, int> getTotalFileSize(trasnactionType &txn);

    resType getTotalSchemaSizes(trasnactionType &txn);

    resType getDedupCharacteristics(trasnactionType &txn);

    resType getFileSizes(trasnactionType &txn);

    void printRes(resType &rss, std::ostream &out);

    template<printable T>
    std::string vecToString(std::vector<T> &vec);

    resType checkFilesExistence(trasnactionType &txn, const std::vector<std::filesystem::path> &files);

    void printRowsAffected(resType &res);

    void printRowsAffected(resType &&res);

    resType checkTExistence(db_services::trasnactionType &txn, std::string_view fileName);

    tl::expected <conPtr, returnCodes> connectIfPossible(std::string_view cString);

    myConnString loadConfiguration(std::string_view filename);

    auto defaultConfiguration = []() {
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

    bool checkConnString(const myConnString &connString);


    /**
     * Wraps function in transaction block
     * @tparam ResultType
     * @tparam Args
     * @param conn_ not null connection
     * @param call
     * @param args
     */
    template<typename ResultType, typename ... Args>
    tl::expected<ResultType, int>
    executeInTransaction(conPtr &conn_, ResultType (*call)(trasnactionType &, Args ...), Args &&... args) {
        trasnactionType txn(*conn_);//todo if conn is null this one will segfault
        ResultType res = call(txn, std::forward<Args>(args)...);
        txn.commit();
        return res;
    }

    /**
     * Functional variant of previous call
     * @tparam ResultType
     * @tparam Args
     * @param conn_  not null connection
     * @param call
     * @param args
     */
    template<typename ResultType, typename ... Args>
    tl::expected<ResultType, int>
    executeInTransaction(conPtr &conn_, const std::function<ResultType(trasnactionType &, Args ...)> &call,
                         Args &&... args) {
        trasnactionType txn(*conn_);
        ResultType res = call(txn, std::forward<Args>(args)...);
        txn.commit();
        return res;
    }

}

#endif //DATA_DEDUPLICATION_SERVICE_DBCOMMON_H
