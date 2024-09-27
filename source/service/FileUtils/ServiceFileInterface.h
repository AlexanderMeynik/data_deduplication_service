#ifndef SERVICE_SERVICEFILEINTERFACE_H
#define SERVICE_SERVICEFILEINTERFACE_H

#include <vector>
#include <array>
#include <string>
#include <iostream>
#include <openssl/sha.h>
#include "../common/myconcepts.h"
#include "../dbUtils/dbManager.h"
#include <sstream>
#include <iomanip>
#include <filesystem>
#include "../common/expected.hpp"

enum db_usage_strategy {
    use,
    create
};

enum data_insetion_strategy {
    preserve_old,
    replace_with_new
};

enum data_retrieval_strategy {
    persist,
    remove_
};
enum directory_handling_strategy {
    no_create_main,
    create_main
};

using db_services::dbManager;


template<verbose_level verbose>
tl::expected<std::string, int> check_file_existence(std::string_view file_path) {
    std::string file;
    try {
        file = std::filesystem::canonical(file_path).string();

        if (!std::filesystem::exists(file_path)) {
            LOG_IF(ERROR, verbose >= 1) << vformat("\"%s\" no such file or directory\n", file_path.data());
            return tl::unexpected{return_codes::error_occured};

        }
        if (std::filesystem::is_directory(file_path)) {
            LOG_IF(ERROR, verbose >= 1)
                            << vformat("\"%s\" is not a file, use processDirectory for directories!\n",
                                       file_path.data());
            return tl::unexpected{return_codes::error_occured};
        }

    } catch (const std::filesystem::filesystem_error &e) {
        LOG_IF(ERROR, verbose >= 1) << vformat("Filesystem error : %s , error code %d\n", e.what(), e.code());
        return tl::unexpected{return_codes::error_occured};
    }
    return tl::expected<std::string, int>{file};
}


template<verbose_level verbose>
tl::expected<std::string, int> check_directory_existence(std::string_view dir_path) {
    std::string directory;
    try {
        directory = std::filesystem::canonical(dir_path).string();


        if (!std::filesystem::exists(dir_path)) {
            LOG_IF(ERROR, verbose >= 1) << vformat("\"%s\" no such file or directory\n", dir_path.data());
            return tl::unexpected{return_codes::error_occured};

        }
        if (!std::filesystem::is_directory(dir_path)) {
            LOG_IF(ERROR, verbose >= 1)
                            << vformat("\"%s\" is not a directory use procesFile for files!\n", dir_path.data());
            return tl::unexpected{return_codes::error_occured};
        }


    } catch (const std::filesystem::filesystem_error &e) {
        LOG_IF(ERROR, verbose >= 1) << vformat("Filesystem error : %s , error code %d\n", e.what(), e.code());
        return tl::unexpected{return_codes::error_occured};
    }
    return tl::expected<std::string, int>{directory};
}
namespace fs= std::filesystem;
fs::path get_normal_abs(fs::path  pwd)
{
    return fs::absolute(pwd).lexically_normal();
}
using ds=db_services::delete_strategy;
template<unsigned long segment_size> requires is_divisible<total_block_size, segment_size>
class FileParsingService {
public:
    using index_type = db_services::index_type;
    static constexpr unsigned long long block_size = total_block_size / segment_size;

    FileParsingService()
    = default;


    template<verbose_level verbose = 0, db_usage_strategy str = use, hash_function hash = SHA_256>
    requires is_divisible<segment_size, hash_function_size[hash]>
    int db_load(std::string &dbName);

    template<verbose_level verbose = 0, data_insetion_strategy strategy = preserve_old>
    int process_directory(std::string_view dir_path);

    template<verbose_level verbose = 0, data_insetion_strategy strategy = preserve_old, bool existence_checks = true, hash_function hash = SHA_256>
    requires is_divisible<segment_size, hash_function_size[hash]>
    int process_file(std::string_view file_path, index_type dir_id = index_vals::empty_parameter_value);


    template<verbose_level verbose = 0, directory_handling_strategy dir_s = no_create_main, data_retrieval_strategy rr = persist>
    int load_directory(std::string_view from_dir, std::string_view to_dir);


    template<verbose_level verbose = 0, directory_handling_strategy dir_s = no_create_main, data_retrieval_strategy rr = persist, bool from_load_dir = false>
    int load_file(std::string_view from_file, std::string_view to_file);

    template<verbose_level verbose = 0,ds delS=ds::cascade>
    int delete_file(std::string_view file_path);
    template<verbose_level verbose = 0,ds delS=ds::cascade>
    int delete_directory(std::string_view dir_path);


private:
    dbManager<segment_size> manager_;
};

template<unsigned long segment_size>
requires is_divisible<total_block_size, segment_size>
template<verbose_level verbose, ds delS>
int FileParsingService<segment_size>::delete_file(std::string_view file_path) {
    fs::path file_abs_path= get_normal_abs(file_path);
    MEASURE_TIME(
    auto res=manager_.template delete_file<verbose COMMA delS>(file_abs_path.string());
    )
    if(res==return_codes::error_occured)
    {
        LOG_IF(ERROR,verbose>=1)<<vformat("Error occurred during directory \"%s\" removal.\n",file_abs_path.c_str());
    }
    return res;
}

template<unsigned long segment_size>
requires is_divisible<total_block_size, segment_size>
template<verbose_level verbose, ds delS>
int FileParsingService<segment_size>::delete_directory(std::string_view dir_path) {
    fs::path dir_abs_path= get_normal_abs(dir_path);

    MEASURE_TIME(
    auto res=manager_.template delete_directory<verbose COMMA delS>(dir_abs_path.string());
    )
    if(res==return_codes::error_occured)
    {
        LOG_IF(ERROR,verbose>=1)<<vformat("Error occurred during directory \"%s\" removal.\n",dir_abs_path.c_str());
    }
    return res;
}



template<unsigned long segment_size>
requires is_divisible<total_block_size, segment_size>
template<verbose_level verbose, directory_handling_strategy dir_s, data_retrieval_strategy rr, bool from_load_dir>
int FileParsingService<segment_size>::load_file(std::string_view from_file, std::string_view to_file) {
    namespace fs = std::filesystem;
    fs::path to_file_path;
    fs::path from_file_path;
    fs::path parent_dir_path;
    try {
        to_file_path = get_normal_abs(to_file);
        parent_dir_path = to_file_path.parent_path();
        from_file_path = get_normal_abs(from_file);

        if (!fs::exists(parent_dir_path)) {
            if constexpr (dir_s == create_main) {
                fs::create_directories(parent_dir_path);
                LOG_IF(INFO, verbose >= 2)
                                << vformat("Root directory \"%s\" was created successfully",
                                           parent_dir_path.c_str());
            } else {
                LOG_IF(ERROR, verbose >= 1)
                                << vformat("\"%s\" no such file or directory\n", parent_dir_path.c_str());
                return return_codes::error_occured;
            }
        }
        if constexpr (dir_s == no_create_main) {

            if (!fs::is_directory(parent_dir_path)) {
                LOG_IF(ERROR, verbose >= 1) << vformat("\"%s\" is not a directory use procesFile for files\n",
                                                       parent_dir_path.c_str());
                return return_codes::error_occured;
            }
        }

    }
    catch (const fs::filesystem_error &e) {
        LOG_IF(ERROR, verbose >= 1) << vformat("Filesystem error : %s , error code %d\n", e.what(), e.code());
        return return_codes::error_occured;
    }

    std::basic_ofstream<symbol_type> out(to_file_path.c_str());

    MEASURE_TIME(
    auto stream_res = manager_.template get_file_streamed<verbose>(from_file_path.string(), out);
    )

    out.close();

    if (stream_res == return_codes::error_occured) {
        LOG_IF(ERROR, verbose >= 1) << vformat("Error occurred during "
                                               "file \"%s\" streaming",
                                               from_file_path.c_str());
        return stream_res;
    }


    if constexpr (!from_load_dir && rr == data_retrieval_strategy::remove_) {
        MEASURE_TIME(
        auto del_res = manager_.template delete_file<verbose>(from_file_path.string());
        )
        if (del_res == return_codes::error_occured) {
            LOG_IF(ERROR, verbose >= 1) << vformat("Error occurred during "
                                                   "file \"%s\" deletion",
                                                   from_file_path.c_str());
            return del_res;
        }
    }

    return 0;

}


template<unsigned long segment_size>
requires is_divisible<total_block_size, segment_size>
template<verbose_level verbose, directory_handling_strategy dir_s, data_retrieval_strategy rr>
int FileParsingService<segment_size>::load_directory(std::string_view from_dir, std::string_view to_dir) {
    namespace fs = std::filesystem;
    fs::path new_dir_path;
    fs::path from_dir_path = get_normal_abs(from_dir);
    try {
        if (!fs::exists(to_dir)) {
            if constexpr (dir_s == create_main) {
                fs::create_directories(to_dir);
                LOG_IF(INFO, verbose >= 2)
                                << vformat("Root directory \"%s\" was created successfully", to_dir.data());
            } else {
                LOG_IF(ERROR, verbose >= 1) << vformat("\"%s\" no such file or directory\n", to_dir.data());
                return return_codes::error_occured;
            }
        }
        if constexpr (dir_s == no_create_main) {

            if (!fs::is_directory(to_dir)) {
                LOG_IF(ERROR, verbose >= 1) << vformat("\"%s\" is not a directory use procesFile for files\n",
                                                       to_dir.data());
                return return_codes::error_occured;
            }
        }
        new_dir_path = get_normal_abs(to_dir);

    } catch (const fs::filesystem_error &e) {
        LOG_IF(ERROR, verbose >= 1) << vformat("Filesystem error : %s , error code %d\n", e.what(), e.code());
        return return_codes::error_occured;
    }

    MEASURE_TIME(
    auto files = manager_.template get_all_files<verbose>(from_dir_path.string());//get
    )
    //todo метод(в бд?) для считывания файла по его имени/пути(gin или какй-нибудь бругой индекс тут пригодяться+tsvector)
    for (const std::pair<db_services::index_type, std::string> &pair: files) {
        std::string file_path = pair.second;

        auto p_t_s = new_dir_path / fs::path(pair.second).lexically_relative(from_dir_path);


        auto result = this->template load_file<verbose, directory_handling_strategy::create_main, rr, true>(pair.second,
                                                                                                            p_t_s.string());
        if (result == return_codes::error_occured) {
            LOG_IF(ERROR, verbose >= 1) << vformat("Error occurred during "
                                                   "file \"%s\" retrieval",
                                                   from_dir_path.c_str());
            continue;
        }
    }

    if constexpr (rr == data_retrieval_strategy::remove_) {
        MEASURE_TIME(
        manager_.template delete_directory<verbose>(from_dir_path.string());//todo leave be or replace
        )
    }
    return 0;
}

template<unsigned long segment_size>
requires is_divisible<total_block_size, segment_size>
template<verbose_level verbose, db_usage_strategy str, hash_function hash>
requires is_divisible<segment_size, hash_function_size[hash]>
int FileParsingService<segment_size>::db_load(std::string &dbName) {
    auto CString = db_services::default_configuration();
    CString.set_dbname(dbName);
    MEASURE_TIME(
    manager_ = dbManager<segment_size>(CString);
    )

    if constexpr (str == create) {
        MEASURE_TIME(
        auto reusult = manager_.template create<verbose>();
        )
        if (reusult == return_codes::error_occured) {
            LOG_IF(ERROR, verbose >= 1) << vformat("Error occurred during database \"%s\" creation\n", dbName.c_str());
            return return_codes::error_occured;
        }
        MEASURE_TIME(
        reusult = manager_.template fill_schemas<verbose COMMA hash>();
        )

        if (reusult == return_codes::error_occured) {
            LOG_IF(ERROR, verbose >= 1)
                            << vformat("Error occurred during database's \"%s\" schema's creation\n", dbName.c_str());
            return return_codes::error_occured;
        }
    } else {
        MEASURE_TIME(
        manager_.template connectToDb<verbose>();
        )
    }
    LOG_IF(INFO, verbose >= 2) << ((manager_.checkConnection()) ? "connection established\n" : "cannot connect\n");
    return 0;
}


template<unsigned long segment_size>
requires is_divisible<total_block_size, segment_size>
template<verbose_level verbose, data_insetion_strategy strategy, bool existence_checks, hash_function hash>
requires is_divisible<segment_size, hash_function_size[hash]>
int FileParsingService<segment_size>::process_file(std::string_view file_path, index_type dir_id) {

    namespace fs = std::filesystem;
    std::string file;
    if constexpr (existence_checks) {
        auto result = check_file_existence<verbose>(file_path);
        if (!result.has_value()) {
            return return_codes::error_occured;
        }
        file = result.value();
    } else {
        file = file_path;
    }

    auto size = fs::file_size(file);
    MEASURE_TIME(
    auto file_id = manager_.template create_file<verbose>(file, dir_id, size);
    )

    if (file_id == return_codes::already_exists) {
        if (strategy == preserve_old) {
            return return_codes::already_exists;
        }
        MEASURE_TIME(
        auto res = manager_.template delete_file<verbose>(file, file_id);//todo replace or presereve
        )

        if (res == return_codes::error_occured) {
            LOG_IF(ERROR, verbose >= 1)
                            << vformat("Error occurred during insert/replace.\n File path \"%s\"!",
                                       file.c_str());
            return return_codes::error_occured;
        }
        MEASURE_TIME(
        file_id = manager_.template create_file<verbose>(file, dir_id, size);
        )
    }

    if (file_id == return_codes::error_occured) {
        LOG_IF(ERROR, verbose >= 1)
                        << vformat("Error occurred during file creation.\n File path \"%s\"!", file.c_str());
        return file_id;
    }
    std::basic_ifstream<symbol_type> in(file);

    MEASURE_TIME(
    auto res1 = manager_.template insert_file_from_stream<verbose>(file, in);
    )

    if (res1 == return_codes::error_occured) {
        LOG_IF(ERROR, verbose >= 1)
                        << vformat("Error occurred during file contents streaming.\n File path \"%s\"!", file.c_str());
        return res1;
    }
    MEASURE_TIME(
    res1 = manager_.template finish_file_processing<verbose COMMA hash>(file, file_id);//todo hash function template
    )

    if (res1 == return_codes::error_occured) {
        LOG_IF(ERROR, verbose >= 1)
                        << vformat("Error occurred during file contents processing.\n File path \"%s\"!", file.c_str());
        return res1;
    }
    return 0;
}


template<unsigned long segment_size>
requires is_divisible<total_block_size, segment_size>
template<verbose_level verbose, data_insetion_strategy strategy>
int FileParsingService<segment_size>::process_directory(std::string_view dir_path) {
    namespace fs = std::filesystem;
    fs::path pp;

    auto result = check_directory_existence<verbose>(dir_path);
    if (!result.has_value()) {
        return return_codes::error_occured;
    }
    pp = result.value();

    MEASURE_TIME(
    auto dir_id = manager_.template create_directory<verbose>(pp.string());
    )
    if (dir_id == return_codes::error_occured || dir_id == return_codes::already_exists) {
        LOG_IF(ERROR, verbose >= 1) << vformat("Error occurred during directory creation. Directory path \"%s\"!",
                                               dir_path.data());
        return dir_id;
    }


    for (const auto &entry: fs::recursive_directory_iterator(pp)) {
        if (!fs::is_directory(entry)) {
            auto file = fs::canonical(entry.path()).string();
            auto results = this->template process_file<verbose, strategy, false>(file, dir_id);
            if (results == return_codes::already_exists) {
                continue;
            } else if (results == return_codes::error_occured) {
                return results;
            }
        }
    }

    return 0;


}


#endif //SERVICE_SERVICEFILEINTERFACE_H
