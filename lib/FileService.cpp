#include "FileService.h"
#include <string>

namespace file_services {

    int FileParsingService::deleteFile(std::string_view filePath) {
        fs::path file_abs_path = getNormalAbs(filePath);
        gClk.tik();
        auto res = manager_.deleteFile(file_abs_path.string());
        gClk.tak();
        if (res == returnCodes::ErrorOccured) {
            VLOG(1) << vformat("Error occurred during directory \"%s\" removal.\n", file_abs_path.c_str());
        }
        return res;
    }


    int FileParsingService::deleteDirectory(std::string_view dirPath) {
        fs::path dir_abs_path = getNormalAbs(dirPath);
        gClk.tik();
        auto res = manager_.deleteDirectory(dir_abs_path.string());
        gClk.tak();
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
        gClk.tak();
        return file_id;
    }

}