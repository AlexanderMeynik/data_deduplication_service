#include "fileUtils.h"
#include "common.h"
#include <cmath>

namespace file_services {
    std::array<size_t, 3> compareDirectories(const fs::path &file1, const fs::path &file2, size_t segmentSize) {
        size_t byteErrors = 0;
        size_t segmentErrors = 0;
        size_t segmentCount = 0;

        std::unordered_set<std::string> f1_s;
        std::unordered_set<std::string> f2_s;

        for (auto &entry: fs::recursive_directory_iterator(file1)) {
            if (!fs::is_directory(entry)) {
                auto path = entry.path().lexically_relative(file1).string();
                f1_s.insert(path);
            }
        }

        for (auto &entry: fs::recursive_directory_iterator(file2)) {
            if (!fs::is_directory(entry)) {
                auto path = entry.path().lexically_relative(file2).string();
                f2_s.insert(path);
            }
        }


        std::vector<std::string> inter;
        std::set_intersection(f1_s.begin(), f1_s.end(), f2_s.begin(), f2_s.end(), std::back_inserter(inter));
        if (
                inter.size() != f1_s.size()
                || !std::all_of(inter.begin(), inter.end(),
                               [&](const auto &item) {
                                   return f2_s.contains(item);
                               })
                ) {
            return {byteErrors, segmentErrors, segmentCount};
        }
        for (auto elem: inter) {
            auto in = file1 / elem;
            auto out = file2 / elem;
            auto res = compareFiles(in, out, segmentSize);
            byteErrors += res[0];
            segmentErrors += res[1];
            segmentCount += res[2];
        }

        return {byteErrors, segmentErrors, segmentCount};
    }

    std::array<size_t, 3> compareFiles(const fs::path &file1, const fs::path &file2, size_t segmentSize) {
        bool areDifferentSizes = false;
        size_t byteErrors = 0;
        size_t segmentErrors = 0;
        auto fs = file_size(file1);
        auto fs2 = file_size(file2);
        if (fs > fs2) {
            std::swap(fs, fs2);
        }
        areDifferentSizes = fs != fs2;

        size_t segmentCount = fs / segmentSize;
        size_t last = fs - segmentCount * segmentSize;

        std::ifstream i1(file1), i2(file2);
        char a1[segmentSize];
        char a2[segmentSize];
        int j = 0;
        for (; j < segmentCount; ++j) {

            i1.readsome(a1, segmentSize);
            i2.readsome(a2, segmentSize);
            auto res = compareArraa(segmentSize, a1, a2);
            byteErrors += res;
            segmentErrors += (res != 0);
        }

        if (last > 0) {
            auto sz = i1.readsome(a1, segmentSize);
            i2.readsome(a2, segmentSize);
            auto res = compareArraa(sz, a1, a2);
            byteErrors += res;
            segmentErrors += (res != 0);
            segmentCount++;
        }
        if (areDifferentSizes) {
            byteErrors += fs2 - fs;
            segmentErrors += std::ceil(((double) (fs2 - fs)) / segmentSize);
            segmentCount = std::ceil(((double) (fs2) / segmentSize));
        }
        return {byteErrors, segmentErrors, segmentCount};
    }

    std::filesystem::path getNormalAbs(std::filesystem::path &&path) {
        return fs::absolute(path).lexically_normal();
    }

    std::filesystem::path getNormalAbs(std::filesystem::path &path) {
        return fs::absolute(path).lexically_normal();
    }

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

}