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
        gClk.tik();
        res=manager_.clearSegments();
        gClk.tak();
        if (res == returnCodes::ErrorOccured) {
            VLOG(1) << vformat("Error occurred during clearing file segments");
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
        gClk.tik();
        res=manager_.clearSegments();
        gClk.tak();
        if (res == returnCodes::ErrorOccured) {
            VLOG(1) << vformat("Error occurred during clearing file segments");
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

    tl::expected<std::array<size_t, 5>, int> FileService::getDataD() {
        try {
            std::function < resType(trasnactionType & ) > ff = [](trasnactionType &txn) {

                std::string q = "select sum(f.size_in_bytes)as totalSize,dataSize,segmentSize, "
                                "       (select pg_total_relation_size('segments')) as rel1, "
                                "       (select pg_total_relation_size('data')) as rel2 from files f "
                                "inner join (select sum(octet_length(d.segment_hash)+8) as dataSize from data d) as ss on true "
                                "inner join (select  sum(octet_length(s.segment_hash)+octet_length(s.segment_data)+8) as segmentSize "
                                "from segments s) as ss2 on true "
                                "group by dataSize,segmentSize; ";
                return txn.exec(q);
            };
            auto res =this->executeInTransaction(ff);
            if(!res.has_value())
            {
                return tl::unexpected{ErrorOccured};
            }
            std::array<size_t, 5> rr{};

            for (int i = 0; i < res->columns(); ++i) {
                rr[i]=res.value()[0][i].as<size_t>();
            }

            return tl::expected<std::array<size_t, 5>,int>{rr};
        }
        catch (std::exception&ex)
        {
            VLOG(1)<<ex.what();
            return tl::unexpected{ErrorOccured};
        }
    }

}