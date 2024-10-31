#include "FileService.h"
#include <string>

namespace file_services {

    int FileService::deleteFile(std::string_view filePath) {
        fs::path file_abs_path = getNormalAbs(filePath);
        gClk.tik();
        auto res = manager_.deleteFile(file_abs_path.string());
        gClk.tak();
        if (res == returnCodes::ErrorOccured) {
            VLOG(1) << vformat("Error occurred during directory \"%s\" removal.\n", file_abs_path.c_str());
        }
        return res;
    }


    int FileService::deleteDirectory(std::string_view dirPath) {
        fs::path dir_abs_path = getNormalAbs(dirPath);
        gClk.tik();
        auto res = manager_.deleteDirectory(dir_abs_path.string());
        gClk.tak();
        if (res == returnCodes::ErrorOccured) {
            VLOG(1) << vformat("Error occurred during directory \"%s\" removal.\n", dir_abs_path.c_str());
        }
        return res;
    }

    int FileService::clearSegments() {
        gClk.tik();
        auto rr = manager_.clearSegments();
        gClk.tak();
        return rr;
    }

    int FileService::insertDirEntry(std::string_view dirPath) {
        gClk.tik();
        auto file_id = manager_.createDirectory(dirPath);
        gClk.tak();
        return file_id;
    }

    tl::expected<double, int> FileService::getCoefficient() {
        try {
            std::function < resType(trasnactionType & ) > ff = [](trasnactionType &txn) {

                std::string q = "with sumCount as(\n"
                                "select sum(segment_count) as segments,count(*) "
                                "    as unique_segments from public.segments) "
                                "select segments,unique_segments,"
                                ""
                                "(unique_segments)::double precision/segments*100 as percentage  from sumCount;";
                return txn.exec(q);
            };
            auto res =this->executeInTransaction(ff);
            return tl::expected<double,int>{res.value()[0][2].as<double>()};
        }
        catch (std::exception&ex)
        {
            VLOG(1)<<ex.what();
            return tl::unexpected{ErrorOccured};
        }
    }

}