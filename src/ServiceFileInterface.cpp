#include "ServiceFileInterface.h"
tl::expected<std::string, int> check_file_existence_(std::string_view file_path) {
    std::string file;
    try {
        file = std::filesystem::canonical(file_path).string();

        if (!std::filesystem::exists(file_path)) {
            VLOG(1) << vformat("\"%s\" no such file or directory\n", file_path.data());
            return tl::unexpected{return_codes::error_occured};

        }
        if (std::filesystem::is_directory(file_path)) {
            VLOG(1)
                            << vformat("\"%s\" is not a file, use processDirectory for directories!\n",
                                       file_path.data());
            return tl::unexpected{return_codes::error_occured};
        }

    } catch (const std::filesystem::filesystem_error &e) {
        VLOG(1) << vformat("Filesystem error : %s , error code %d\n", e.what(), e.code());
        return tl::unexpected{return_codes::error_occured};
    }
    return tl::expected<std::string, int>{file};
}

tl::expected<std::string, int> check_directory_existence_(std::string_view dir_path) {
        std::string directory;
        try {
            directory = std::filesystem::canonical(dir_path).string();


            if (!std::filesystem::exists(dir_path)) {
                VLOG(1) << vformat("\"%s\" no such file or directory\n", dir_path.data());
                return tl::unexpected{return_codes::error_occured};

            }
            if (!std::filesystem::is_directory(dir_path)) {
                VLOG(1)
                                << vformat("\"%s\" is not a directory use procesFile for files!\n", dir_path.data());
                return tl::unexpected{return_codes::error_occured};
            }


        } catch (const std::filesystem::filesystem_error &e) {
            VLOG(1) << vformat("Filesystem error : %s , error code %d\n", e.what(), e.code());
            return tl::unexpected{return_codes::error_occured};
        }
        return tl::expected<std::string, int>{directory};
}

