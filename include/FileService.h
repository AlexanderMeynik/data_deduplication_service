#ifndef DATA_DEDUPLICATION_SERVICE_FILESERVICE_H
#define DATA_DEDUPLICATION_SERVICE_FILESERVICE_H

#include <vector>
#include <array>
#include <string>
#include <iostream>


#include "dbManager.h"
#include "fileUtils.h"

/// file services namespace
namespace file_services {

    using namespace db_services;

    /**
     * @brief this class handles file/directory management and uses
     * @ref db_services::dbManager "dbManager" to perform calls
     * @details This class have comfortable wrappers for most db_services::dbManager functions
     * @details Some external helepr functions can be found @ref fileUtils.h "here".
     */
    class FileService {
    public:
        using indexType = db_services::indexType;

        FileService()
        = default;

        /**
         * This function handles database open/create action
         * If db_usage_str==create this function will create new database
         * othervice it'll try to open existing database.
         * @tparam dbUsageStrategy
         * @tparam hash
         * @param dbName
         * @param configurationFile
         */
        template<dbUsageStrategy dbUsageStrategy = use>
        int dbLoad(std::string_view dbName, std::string_view configurationFile = db_services::cfileName);

        /**
         *
         * @tparam dbUsageStrategy
         * @param cStr
         * @ref "dbLoad(std::string_view dbName, std::string_view configurationFile = db_services::cfileName)
         * "dbLoad()"
         */
        template<dbUsageStrategy dbUsageStrategy = use>
        int dbLoad(db_services::myConnString &cStr);

        /**
         *
         * @param dbName
         */
        int dbDrop(std::string_view dbName) {
            auto res = manager_.dropDatabase(dbName);
            return res;
        };

        /**
         * Get vector of pairs of file_names and file ids for directory
         * @param dirPath
         */
        std::vector<std::pair<indexType, std::string>> getAllFiles(std::string_view dirPath)
        {
            return manager_.getAllFiles(dirPath);
        }

        /**
         * Processes all files in the given directory runs @ref process_file() "processFile()" for each file
         * @tparam data_insertion_str
         * @param dirPath
         * @param segmentSize
         * @param hash
         */
        template<dataInsetionStrategy data_insertion_str = PreserveOld>
        int processDirectory(std::string_view dirPath, size_t segmentSize, const hash_function& hash= SHA_256);

        /** @details Creates entry for file in database.
         * @details Load file segments into temp table.
         * @details Upsets segment entiries
         * @details Add file data.
         * @tparam data_insertion_str
         * @tparam existence_checks
         * @param filePath
         * @param segmentSize
         * @param hash
         */
        template<dataInsetionStrategy data_insertion_str = PreserveOld, bool existence_checks = true>
        int processFile(std::string_view filePath, size_t segmentSize,const hash_function& hash= SHA_256);

        /**
         * Insert entry for directory
         * @param dirPath
         */
        int insertDirEntry(std::string_view dirPath);

        /**
         * Gets unique segments percentage
         */
        tl::expected<double,int> getCoefficient();
        /**
         * Gets database sizes statistics
         */
        tl::expected<std::array<int, 5>, int> getDataD();

        /**
         * Retrieves directory from database to to_dir
         * @tparam root_directory_str
         * @tparam retrievalStrategy
         * @param fromDir
         * @param toDir
         */
        template<rootDirectoryHandlingStrategy root_directory_str = NoCreateMain,
                dataRetrievalStrategy retrievalStrategy = Persist>
        int loadDirectory(std::string_view fromDir, std::string_view toDir);

        /**
         * Retrieves file from database to to_file
         * @tparam dir_s
         * @tparam retrievalStrategy
         * @param fromFile
         * @param toFile
         */
        template<rootDirectoryHandlingStrategy dir_s = NoCreateMain,
                dataRetrievalStrategy retrievalStrategy = Persist, bool from_load_dir = false>
        int loadFile(std::string_view fromFile, std::string_view toFile,
                     indexType fileId = paramType::EmptyParameterValue);

        /**
         * Deletes file entry and data from database
         * @param filePath
         */
        int deleteFile(std::string_view filePath);

        /**
         * Deletes database entry and data for directory
         * @param dirPath
         */
        int deleteDirectory(std::string_view dirPath);

        bool checkConnection() {
            return manager_.checkConnection();
        }

        int clearSegments();

        /**
         * Wrapper for associated dbManger member function
         * @see @ref  dbManager::executeInTransaction() "executeInTransaction()"
        */
        template<typename ResType1, typename ... Args>
        tl::expected<ResType1, int> executeInTransaction(ResType1
                                                         (*call)(db_services::trasnactionType &, Args ...),
                                                         Args &&... args) {
            return manager_.executeInTransaction(call, std::forward<Args>(args)...);
        }

        /**
         * Wrapper for assoctiated dbManger member function
         * @see @ref  dbManager::executeInTransaction(const std::function< ResultType
         * (trasnactionType &, Args ...)> &  call,Args &&... 	args ) "executeInTransaction()"
        */
        template<typename ResType1, typename ... Args>
        tl::expected<ResType1, int>
        executeInTransaction(const std::function<ResType1(db_services::trasnactionType &, Args ...)> &call,
                             Args &&... args) {
            return manager_.executeInTransaction(call, std::forward<Args>(args)...);
        }

        void inline disconnect() {
            manager_.disconnect();
        }

    private:
        dbManager manager_;
    };


    template<rootDirectoryHandlingStrategy dir_s, dataRetrievalStrategy rr, bool from_load_dir>
    int FileService::loadFile(std::string_view fromFile, std::string_view toFile,
                              indexType fileId) {
        fs::path toFilePath;
        fs::path fromFilePath;
        fs::path parentDirPath;
        try {
            toFilePath = getNormalAbs(toFile);
            parentDirPath = toFilePath.parent_path();
            fromFilePath = getNormalAbs(fromFile);

            if (!fs::exists(parentDirPath)) {
                if constexpr (dir_s == CreateMain) {
                    fs::create_directories(parentDirPath);
                    VLOG(2)
                                    << vformat("Root directory \"%s\" was created successfully",
                                               parentDirPath.c_str());
                } else {
                    VLOG(1)
                                    << vformat("\"%s\" no such file or directory\n", parentDirPath.c_str());
                    return returnCodes::ErrorOccured;
                }
            }
            if constexpr (dir_s == NoCreateMain) {

                if (!fs::is_directory(parentDirPath)) {
                    VLOG(1) << vformat("\"%s\" is not a directory use procesFile for files\n",
                                       parentDirPath.c_str());
                    return returnCodes::ErrorOccured;
                }
            }

        }
        catch (const fs::filesystem_error &e) {
            VLOG(1) << vformat("Filesystem error : %s , error code %d\n", e.what(), e.code());
            return returnCodes::ErrorOccured;
        }
        if (fs::is_directory(toFilePath)) {
            VLOG(1) << vformat("Entry  %s is not a file use processDirectory for directories\n", toFilePath.c_str());
            return returnCodes::ErrorOccured;
        }

        std::basic_ofstream<symbolType> out(toFilePath.c_str());

        auto streamRes = manager_.getFileStreamed(fromFilePath.string(), out, fileId);

        out.close();

        if (streamRes == returnCodes::ErrorOccured) {
            VLOG(1) << vformat("Error occurred during "
                               "file \"%s\" streaming",
                               fromFilePath.c_str());
            return streamRes;
        }

        if constexpr (!from_load_dir && rr == dataRetrievalStrategy::Remove) {

            auto delRes = manager_.deleteFile(fromFilePath.string());

            if (delRes == returnCodes::ErrorOccured) {
                VLOG(1) << vformat("Error occurred during "
                                   "file \"%s\" deletion",
                                   fromFilePath.c_str());
                return delRes;
            }
        }
        return returnCodes::ReturnSucess;
    }


    template<rootDirectoryHandlingStrategy dir_s, dataRetrievalStrategy rr>
    int FileService::loadDirectory(std::string_view fromDir, std::string_view toDir) {
        fs::path newDirPath;
        fs::path fromDirPath = getNormalAbs(fromDir);


        try {
            if (!fs::exists(toDir)) {
                if constexpr (dir_s == CreateMain) {
                    fs::create_directories(toDir);
                    VLOG(2)
                                    << vformat("Root directory \"%s\" was created successfully", toDir.data());
                } else {
                    VLOG(1) << vformat("\"%s\" no such file or directory\n", toDir.data());
                    return returnCodes::ErrorOccured;
                }
            }
            if constexpr (dir_s == NoCreateMain) {

                if (!fs::is_directory(toDir)) {
                    VLOG(1) << vformat("\"%s\" is not a directory change to_dir path\n",
                                       toDir.data());
                    return returnCodes::ErrorOccured;
                }
            }
            newDirPath = getNormalAbs(toDir);
        } catch (const fs::filesystem_error &e) {
            VLOG(1) << vformat("Filesystem error : %s , error code %d\n", e.what(), e.code());
            return returnCodes::ErrorOccured;
        }

        if (!fs::is_directory(newDirPath)) {
            VLOG(1) << vformat("Entry  %s is not a directory use processFile for files\n", newDirPath.c_str());
            return returnCodes::ErrorOccured;
        }
        auto files = manager_.getAllFiles(fromDirPath.string());
        if (files.empty()) {
            VLOG(1) << vformat("No files found for directory %s", fromDir.data());
        }


        for (const std::pair<db_services::indexType, std::string> &pair: files) {
            std::string filePath = pair.second;

            auto newDirRealPath = newDirPath / fs::path(pair.second).lexically_relative(fromDirPath);

            auto result = this->template loadFile<rootDirectoryHandlingStrategy::CreateMain, rr, true>
                    (pair.second, newDirRealPath.string(), pair.first);
            if (result == returnCodes::ErrorOccured) {
                VLOG(1) << vformat("Error occurred during "
                                   "file \"%s\" retrieval",
                                   fromDirPath.c_str());
                continue;
            }
        }

        if constexpr (rr == dataRetrievalStrategy::Remove) {
            manager_.deleteDirectory(fromDirPath.string());
        }
        return returnCodes::ReturnSucess;
    }


    template<dbUsageStrategy str>
    int FileService::dbLoad(std::string_view dbName, std::string_view configurationFile) {
        auto cString = db_services::loadConfiguration(configurationFile);
        cString.setDbname(dbName);
        return dbLoad<str>(cString);
    }


    template<dbUsageStrategy str>
    int FileService::dbLoad(db_services::myConnString &cStr) {
        manager_ = dbManager(cStr);

        if constexpr (str == create) {

            auto reusult = manager_.createDatabase(cStr.getDbname());

            if (reusult == returnCodes::ErrorOccured) {
                VLOG(1) << vformat("Error occurred during database \"%s\" creation\n", cStr.getDbname().data());
                return returnCodes::ErrorOccured;
            }

            reusult = manager_.fillSchemas();

            if (reusult == returnCodes::ErrorOccured) {
                VLOG(1)
                                << vformat("Error occurred during database's \"%s\" schema's creation\n",
                                           cStr.getDbname().data());
                return returnCodes::ErrorOccured;
            }
        } else {

            auto res = manager_.connectToDb();
            if (res == returnCodes::ErrorOccured) {
                VLOG(1)
                                << vformat("Error occurred during database's \"%s\" schema's creation\n",
                                           cStr.getDbname().data());
                return returnCodes::ErrorOccured;
            }
        }
        VLOG(2) << ((manager_.checkConnection()) ? "connection established\n" : "cannot connect\n");
        return ReturnSucess;
    }


    template<dataInsetionStrategy strategy, bool existence_checks>
    int FileService::processFile(std::string_view filePath, size_t segmentSize,const hash_function& hash) {
        std::string file;
        if constexpr (existence_checks) {
            auto result = checkFileExistence(filePath);
            if (!result.has_value()) {
                return returnCodes::ErrorOccured;
            }
            file = result.value();
        } else {
            file = filePath;
        }

        auto size = fs::file_size(file);
        if(!size)
        {
            VLOG(1)<<"Application doesnt support empty file insertion!";
            return AlreadyExists;
        }

        gClk.tik();
        auto fileId = manager_.createFile(file, size, segmentSize, hash);
        gClk.tak();

        if (fileId == returnCodes::AlreadyExists) {
            if (strategy == PreserveOld) {
                return returnCodes::AlreadyExists;
            }

            auto res = manager_.deleteFile(file, fileId);

            if (res == returnCodes::ErrorOccured) {
                VLOG(1)
                                << vformat("Error occurred during insert/replace.\n File path \"%s\"!",
                                           file.c_str());
                return returnCodes::ErrorOccured;
            }
            fileId = manager_.createFile(file, size, segmentSize, hash);
        }

        if (fileId == returnCodes::ErrorOccured) {
            VLOG(1)
                            << vformat("Error occurred during file creation.\n File path \"%s\"!", file.c_str());
            return returnCodes::ErrorOccured;
        }
        std::basic_ifstream<symbolType> in(file);

        gClk.tik();
        auto res1 = manager_.insertFileFromStream(file, in, segmentSize, size,hash);
        gClk.tak();

        if (res1 == returnCodes::ErrorOccured) {
            VLOG(1)
                            << vformat("Error occurred during file contents streaming.\n File path \"%s\"!",
                                       file.c_str());
            return res1;
        }
        gClk.tik();
        res1 = manager_.finishFileProcessing(file, fileId);
        gClk.tak();

        if (res1 == returnCodes::ErrorOccured) {
            VLOG(1)
                            << vformat("Error occurred during file contents processing.\n File path \"%s\"!",
                                       file.c_str());
            return res1;
        }
        return returnCodes::ReturnSucess;
    }


    template<dataInsetionStrategy strategy>
    int FileService::processDirectory(std::string_view dirPath, size_t segmentSize, const hash_function& hash) {
        fs::path pp;

        auto result = checkDirectoryExistence(dirPath);
        if (!result.has_value()) {
            return ErrorOccured;
        }
        pp = result.value();

        auto dd = fs::canonical(pp).string();


        int res=ReturnSucess;

        auto res1=insertDirEntry(dd);
        if(res1!=ReturnSucess)
        {
            res=res1;
        }

        for (const auto &entry: fs::recursive_directory_iterator(pp)) {
            if (!fs::is_directory(entry)) {
                auto file = fs::canonical(entry.path()).string();
                gClk.tik();
                auto results = this->template processFile<strategy, false>(file, segmentSize, hash);
                gClk.tak();

                if (results == AlreadyExists) {
                    res=AlreadyExists;
                    continue;
                }
                if (results == ErrorOccured) {
                    return results;
                }
            } else
            {
                auto dd = fs::canonical(entry.path()).string();
                auto results=insertDirEntry(dd);
                if (results == AlreadyExists) {
                    res=AlreadyExists;
                }
            }
        }
        return res;
    }
}

#endif //DATA_DEDUPLICATION_SERVICE_FILESERVICE_H
