#ifndef DATA_DEDUPLICATION_SERVICE_FILESERVICE_H
#define DATA_DEDUPLICATION_SERVICE_FILESERVICE_H

#include <vector>
#include <array>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <filesystem>

#include <openssl/sha.h>

#include "dbManager.h"


using myConcepts::SymbolType;
using db_services::dbManager;
namespace fs = std::filesystem;

/// file services namespace
namespace file_services {
    using myConcepts::gClk;
    /**
     * @ingroup utility
     * Database usage strategy
     */
    enum dbUsageStrategy {
        /// if database exist connection will be established
        use,
        /// create new database if it doesn't exist
        create
    };

    /**
     * @ingroup utility
     * File/directories insertion startegy
     */
    enum dataInsetionStrategy {
        /// will ignore files that already exist
        PreserveOld,
        /// will replace file contents
        ReplaceWithNew
    };

    /**
     * @ingroup utility
     * File/directory handling strategy during load process
     */
    enum dataRetrievalStrategy {
        /// will leave data as is
        Persist,
        /// will delete requested data from database
        Remove
    };

    /**
     * @ingroup utility
     * Defines what will be done if destination directory does not exist
     */
    enum rootDirectoryHandlingStrategy {
        /// will return an error code
        NoCreateMain,
        /// will create this directory using create_directories
        CreateMain
    };

    /**
     * This function checks existence of a canonical file path for file_path
     * @param filePath
     * @return canonical filePath or error code
     */
    tl::expected<std::string, int> checkFileExistence(std::string_view filePath);

    /**
     * This function checks existence of a canonical directory path for dir_path
     * @param dirPath
     * @return canonical dirPath or error code
     */
    tl::expected<std::string, int> checkDirectoryExistence(std::string_view dirPath);

    /**
     * Calculates lexically normal absolute path
     * @param path
     * @return lexically normal absolute path
     */
    fs::path getNormalAbs(fs::path &path);

    fs::path getNormalAbs(fs::path &&path);

    /**
     * this class handles file/directory management and uses @ref db_services::dbManager "dbManager" to perform calls
     */
    
    class FileParsingService {
    public:
        using index_type = db_services::indexType;

        FileParsingService()
        = default;

        /**
         * This function handles database open/crete action
         * If db_usage_str==create this function will create new database
         * othervice it'll try to open existing database.
         * @tparam db_usage_str
         * @tparam hash
         * @param dbName
         * @param configurationFile
         */
        template<dbUsageStrategy db_usage_str = use>
        int dbLoad(std::string &dbName, std::string_view configurationFile = db_services::cfileName);

        int dbDrop(std::string_view dbName) {
            auto res = manager_.dropDatabase(dbName);
            return res;
        };

        /**
         * Processes all files in the given directory runs @ref process_file() "processFile()" for each file
         * @tparam data_insertion_str
         * @tparam hash
         * @param dirPath
         * @param segment_size
         */
        template<dataInsetionStrategy data_insertion_str = PreserveOld, hash_function hash = SHA_256>
        int processDirectory(std::string_view dirPath, size_t segment_size);

        /** @details Creates entry for file in database.
         * @details Load file segments into temp table.
         * @details Upsets segment entiries
         * @details Add file data.
         * @tparam data_insertion_str
         * @tparam existence_checks
         * @tparam hash
         * @param filePath
         * @param segmentSize
         */
        template<dataInsetionStrategy data_insertion_str = PreserveOld, bool existence_checks = true,
                hash_function hash = SHA_256>
        int processFile(std::string_view filePath, size_t segmentSize);

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
         * @param from_file
         * @param toFile
         */
        template<rootDirectoryHandlingStrategy dir_s = NoCreateMain,
                dataRetrievalStrategy retrievalStrategy = Persist, bool from_load_dir = false>
        int loadFile(std::string_view from_file, std::string_view toFile,
                     index_type fileId = paramType::EmptyParameterValue);

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
    private:
        dbManager manager_;
    };

    



    
    //todo create regular expression grabber for files
    template<rootDirectoryHandlingStrategy dir_s, dataRetrievalStrategy rr, bool from_load_dir>
    int FileParsingService::loadFile(std::string_view from_file, std::string_view toFile,
                                                   index_type fileId) {
        fs::path toFilePath;
        fs::path fromFilePath;
        fs::path parentDirPath;
        try {
            toFilePath = getNormalAbs(toFile);
            parentDirPath = toFilePath.parent_path();
            fromFilePath = getNormalAbs(from_file);

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

        std::basic_ofstream<SymbolType> out(toFilePath.c_str());

        auto stream_res = manager_.getFileStreamed(fromFilePath.string(), out, fileId);

        out.close();

        if (stream_res == returnCodes::ErrorOccured) {
            VLOG(1) << vformat("Error occurred during "
                               "file \"%s\" streaming",
                               fromFilePath.c_str());
            return stream_res;
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
        return 0;
    }


    
    template<rootDirectoryHandlingStrategy dir_s, dataRetrievalStrategy rr>
    int FileParsingService::loadDirectory(std::string_view fromDir, std::string_view toDir) {
        fs::path newDirPath;
        fs::path fromDirPath = getNormalAbs(fromDir);

        auto files = manager_.getAllFiles(fromDirPath.string());
        if (files.empty()) {
            VLOG(1) << vformat("No files found for directory %s", fromDir.data());
        }
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
        return 0;
    }

    
    template<dbUsageStrategy str>
    int FileParsingService::dbLoad(std::string &dbName, std::string_view configurationFile) {
        auto CString = db_services::loadConfiguration(configurationFile);
        CString.setDbname(dbName);

        manager_ = dbManager(CString);

        if constexpr (str == create) {

            auto reusult = manager_.createDatabase(dbName);

            if (reusult == returnCodes::ErrorOccured) {
                VLOG(1) << vformat("Error occurred during database \"%s\" creation\n", dbName.c_str());
                return returnCodes::ErrorOccured;
            }

            reusult = manager_.fillSchemas();

            if (reusult == returnCodes::ErrorOccured) {
                VLOG(1)
                                << vformat("Error occurred during database's \"%s\" schema's creation\n",
                                           dbName.c_str());
                return returnCodes::ErrorOccured;
            }
        } else {

            auto res = manager_.connectToDb();
            if (res == returnCodes::ErrorOccured) {
                VLOG(1)
                                << vformat("Error occurred during database's \"%s\" schema's creation\n",
                                           dbName.c_str());
                return returnCodes::ErrorOccured;
            }
        }
        VLOG(2) << ((manager_.checkConnection()) ? "connection established\n" : "cannot connect\n");
        return 0;
    }


    
    template<dataInsetionStrategy strategy, bool existence_checks, hash_function hash>
    int FileParsingService::processFile(std::string_view filePath, size_t segmentSize) {
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

        gClk.tik();
        auto file_id = manager_.createFile(file, size);
        gClk.tak();

        if (file_id == returnCodes::AlreadyExists) {
            if (strategy == PreserveOld) {
                return returnCodes::AlreadyExists;
            }

            auto res = manager_.deleteFile(file, file_id);

            if (res == returnCodes::ErrorOccured) {
                VLOG(1)
                                << vformat("Error occurred during insert/replace.\n File path \"%s\"!",
                                           file.c_str());
                return returnCodes::ErrorOccured;
            }
            file_id = manager_.createFile(file, size);
        }

        if (file_id == returnCodes::ErrorOccured) {
            VLOG(1)
                            << vformat("Error occurred during file creation.\n File path \"%s\"!", file.c_str());
            return file_id;
        }
        std::basic_ifstream<SymbolType> in(file);

        gClk.tik();
        auto res1 = manager_.template insertFileFromStream<hash>(segmentSize, file, in, size);
        gClk.tak();

        if (res1 == returnCodes::ErrorOccured) {
            VLOG(1)
                            << vformat("Error occurred during file contents streaming.\n File path \"%s\"!",
                                       file.c_str());
            return res1;
        }
        gClk.tik();
        res1 = manager_.finishFileProcessing(file, file_id);
        gClk.tak();

        if (res1 == returnCodes::ErrorOccured) {
            VLOG(1)
                            << vformat("Error occurred during file contents processing.\n File path \"%s\"!",
                                       file.c_str());
            return res1;
        }
        return 0;
    }


    
    template<dataInsetionStrategy strategy, hash_function hash>
    int FileParsingService::processDirectory(std::string_view dirPath, size_t segment_size) {
        fs::path pp;

        auto result = checkDirectoryExistence(dirPath);
        if (!result.has_value()) {
            return ErrorOccured;
        }
        pp = result.value();

        for (const auto &entry: fs::recursive_directory_iterator(pp)) {
            if (!fs::is_directory(entry)) {
                auto file = fs::canonical(entry.path()).string();
                gClk.tik();
                auto results = this->template processFile<strategy, false, hash>(file, segment_size);
                gClk.tak();
                if (results == AlreadyExists) {
                    continue;
                } else if (results == ErrorOccured) {
                    return results;
                }
            }
        }
        return 0;
    }

}
#endif //DATA_DEDUPLICATION_SERVICE_FILESERVICE_H
