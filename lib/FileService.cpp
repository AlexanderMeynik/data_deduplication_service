#include "FileService.h"
#include <string>

tl::expected<std::string, int> file_services::checkFileExistence(std::string_view filePath) {
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

tl::expected<std::string, int> file_services::checkDirectoryExistence(std::string_view dirPath) {
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

std::filesystem::path file_services::getNormalAbs(std::filesystem::path &&path) {
    return fs::absolute(path).lexically_normal();
}

std::filesystem::path file_services::getNormalAbs(std::filesystem::path &path) {
    return fs::absolute(path).lexically_normal();
}

