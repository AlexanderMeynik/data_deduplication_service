

#ifndef DATA_DEDUPLICATION_SERVICE_FILEUTILS_H
#define DATA_DEDUPLICATION_SERVICE_FILEUTILS_H

#include <filesystem>
#include <concepts>
#include <fstream>
#include <unordered_set>

#include "expected.hpp"
#include "myConcepts.h"


/// file services namespace
namespace file_services {

    using myConcepts::symbolType;
    namespace fs = std::filesystem;

    using myConcepts::gClk;
    /**
     * Database usage strategy
     */
    enum dbUsageStrategy {
        /// if database exist connection will be established
        use,
        /// create new database if it doesn't exist
        create
    };

    /**
     * File/directories insertion startegy
     */
    enum dataInsetionStrategy {
        /// will ignore files that already exist
        PreserveOld,
        /// will replace file contents
        ReplaceWithNew
    };

    /**
     * File/directory handling strategy during load process
     */
    enum dataRetrievalStrategy {
        /// will leave data as is
        Persist,
        /// will delete requested data from database
        Remove
    };

    /**
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

    namespace fs = std::filesystem;


    template<typename T, typename A>
    requires std::is_same_v<T, A> ||
             std::is_same_v<T, typename std::remove_const<A>::type> ||
             std::is_same_v<A, typename std::remove_const<T>::type>
    int compareBlock(size_t size, T *arr, A *arr2) {
        int error = 0;
        for (int arrElem = 0; arrElem < size; ++arrElem) {
            error += arr[arrElem] != arr2[arrElem];
        }
        return error;
    }

    /**
     * Compares to file paths segment by segment
     * @param file1
     * @param file2
     * @param segmentSize size of blocks to check
     * @return {e1,e2,e3,e4}
     * @details e1 - number of bytes that differ
     * @details e2 - number of segments that differ
     * @details e3 - total number of segments
     * @details e4 - total number of bytes
     */
    std::array<size_t, 4> compareFiles(const fs::path &file1, const fs::path &file2, size_t segmentSize);

    /**
     * Compares directory contents
     * @param file1
     * @param file2
     * @param segmentSize size of blocks to check
     * @return {e1,e2,e3,e4}
     * @details e1 - number of bytes that differ
     * @details e2 - number of segments that differ
     * @details e3 - total number of segments
     * @details e4 - total number of bytes
     */
    std::array<size_t, 4> compareDirectories(const fs::path &file1, const fs::path &file2, size_t segmentSize);

    /**
    * Calculates lexically normal absolute path
    * @param path
    * @return lexically normal absolute path
    */
    fs::path getNormalAbs(const fs::path &path);


}
#endif //DATA_DEDUPLICATION_SERVICE_FILEUTILS_H
