#include "FileService.h"
#include <string>

namespace file_services {
    tl::expected<std::string, int> checkFileExistence(std::string_view filePath) {
        std::string file;
        try {
            file = std::filesystem::canonical(filePath).string();

            if (!std::filesystem::exists(filePath)) {
                VLOG(1) << vformat("\"%s\" no such file or directory\n", filePath.data());
                return tl::unexpected{returnCodes::ErrorOccured};
            }
            if (std::filesystem::is_directory(filePath)) {
                VLOG(1)
                                << vformat("\"%s\" is not a file, use processDirectory for directories!\n",
                                           filePath.data());
                return tl::unexpected{returnCodes::ErrorOccured};
            }
        } catch (const std::filesystem::filesystem_error &e) {
            VLOG(1) << vformat("Filesystem error : %s , error code %d\n", e.what(), e.code());
            return tl::unexpected{returnCodes::ErrorOccured};
        }
        return tl::expected<std::string, int>{file};
    }

    tl::expected<std::string, int> checkDirectoryExistence(std::string_view dirPath) {
        std::string directory;
        try {
            directory = std::filesystem::canonical(dirPath).string();


            if (!std::filesystem::exists(dirPath)) {
                VLOG(1) << vformat("\"%s\" no such file or directory\n", dirPath.data());
                return tl::unexpected{returnCodes::ErrorOccured};
            }
            if (!std::filesystem::is_directory(dirPath)) {
                VLOG(1)
                                << vformat("\"%s\" is not a directory use procesFile for files!\n", dirPath.data());
                return tl::unexpected{returnCodes::ErrorOccured};
            }
        } catch (const std::filesystem::filesystem_error &e) {
            VLOG(1) << vformat("Filesystem error : %s , error code %d\n", e.what(), e.code());
            return tl::unexpected{returnCodes::ErrorOccured};
        }
        return tl::expected<std::string, int>{directory};
    }

    std::filesystem::path getNormalAbs(std::filesystem::path &&path) {
        return fs::absolute(path).lexically_normal();
    }

    std::filesystem::path getNormalAbs(std::filesystem::path &path) {
        return fs::absolute(path).lexically_normal();
    }


    int FileParsingService::deleteFile(std::string_view filePath) {
        fs::path file_abs_path = getNormalAbs(filePath);

        auto res = manager_.deleteFile(file_abs_path.string());

        if (res == returnCodes::ErrorOccured) {
            VLOG(1) << vformat("Error occurred during directory \"%s\" removal.\n", file_abs_path.c_str());
        }
        return res;
    }



    int FileParsingService::deleteDirectory(std::string_view dirPath) {
        fs::path dir_abs_path = getNormalAbs(dirPath);

        auto res = manager_.deleteDirectory(dir_abs_path.string());

        if (res == returnCodes::ErrorOccured) {
            VLOG(1) << vformat("Error occurred during directory \"%s\" removal.\n", dir_abs_path.c_str());
        }
        return res;
    }

    int FileParsingService::clearSegments() {
        gClk.tik();
        auto rr = manager_.clearSegments();
        gClk.tak();
        return rr;
    }

    int FileParsingService::insertDirEntry(std::string_view dirPath) {
        gClk.tik();
        auto file_id = manager_.createDirectory(dirPath);
        //todo create variant that doesnt create temporary tables
        gClk.tak();
        return file_id;
    }


}