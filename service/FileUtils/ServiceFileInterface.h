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


template<unsigned long segment_size,hash_function hash=SHA_256>
        requires is_divisible<segment_size, hash_function_size[hash]> &&
        is_divisible<total_block_size, segment_size>
class FileParsingService {
public:
    static constexpr unsigned long long block_size = total_block_size / segment_size;

    FileParsingService()
    = default;


    template<verbose_level verbose = 0, db_usage_strategy str = use>
    int db_load(std::string &dbName);

    template<verbose_level verbose = 0, data_insetion_strategy strategy = preserve_old>
    int process_directory(std::string &trainDir);


    template<verbose_level verbose = 0, directory_handling_strategy dir_s = no_create_main, data_retrieval_strategy rr = persist>
    int load_directory(std::string &from_dir, std::string &to_dir);


private:
    dbManager<segment_size> manager_;
};

template<unsigned long segment_size,hash_function hash>
requires is_divisible<segment_size, hash_function_size[hash]>
         && is_divisible<total_block_size, segment_size>
template<verbose_level verbose, directory_handling_strategy dir_s, data_retrieval_strategy rr>
int FileParsingService<segment_size,hash>::load_directory(std::string &from_dir, std::string &to_dir) {
    namespace fs = std::filesystem;
    fs::path new_abs;
    fs::path curr_abs = fs::canonical(from_dir);
    try {


        if (!fs::exists(to_dir)) {
            if constexpr (dir_s == create_main) {
                fs::create_directories(to_dir);
                LOG_IF(INFO, verbose >= 2)
                                << vformat("Root directory \"%s\" was created successfully", to_dir.c_str());
            } else {
                LOG_IF(ERROR, verbose >= 1) << vformat("\"%s\" no such file or directory\n", to_dir.c_str());
                return return_codes::error_occured;
            }
        }
        if constexpr (dir_s == no_create_main) {

            if (!fs::is_directory(to_dir)) {
                LOG_IF(ERROR, verbose >= 1) << vformat("\"%s\" is not a directory use procesFile for files\n",
                                                       to_dir.c_str());
                return return_codes::error_occured;
            }
        }
        new_abs = fs::canonical(to_dir);

    } catch (const fs::filesystem_error &e) {
        LOG_IF(ERROR, verbose >= 1) << vformat("Filesystem error : %s , error code %d\n", e.what(), e.code());
        return -3;
    }

    auto files = manager_.template get_all_files<verbose>(curr_abs.string());//get
    //todo метод(в бд?) для считывания файла по его имени/пути(gin или какй-нибудь бругой индекс тут пригодяться+tsvector)
    for (const std::pair<db_services::index_type, std::string> &pair: files) {
        std::string file_path = pair.second;

        auto p_t_s = new_abs / fs::path(pair.second).lexically_relative(curr_abs);

        fs::path parent_dir = p_t_s.parent_path();
        if (!fs::exists(parent_dir)) {
            fs::create_directories(parent_dir);
            LOG_IF(INFO, verbose >= 2) << vformat("Creating parent directories:  %s \n", parent_dir.string().c_str());

        }
        std::basic_ofstream<symbol_type> out(p_t_s);

        manager_.template get_file_streamed<verbose>(pair.second, out);

        out.close();
    }

    if constexpr (rr == data_retrieval_strategy::remove_) {
        manager_.template delete_directory<verbose>(curr_abs.string());
    }
    return 0;
}

template<unsigned long segment_size,hash_function hash>
requires is_divisible<segment_size, hash_function_size[hash]>
         && is_divisible<total_block_size, segment_size>
template<verbose_level verbose, db_usage_strategy str>
int FileParsingService<segment_size,hash>::db_load(std::string &dbName) {
    auto CString = db_services::default_configuration();
    CString.set_dbname(dbName);

    manager_ = dbManager<segment_size>(CString);

    if constexpr (str == create) {
        auto reusult = manager_.template create<verbose>();
        if (reusult == return_codes::error_occured) {
            LOG_IF(ERROR, verbose >= 1) << vformat("Error occurred during database \"%s\" creation\n", dbName.c_str());
            return return_codes::error_occured;
        }
        reusult = manager_.template fill_schemas<verbose,hash>();

        if (reusult == return_codes::error_occured) {
            LOG_IF(ERROR, verbose >= 1)
                            << vformat("Error occurred during database's \"%s\" schema's creation\n", dbName.c_str());
            return return_codes::error_occured;
        }
    } else {
        manager_.template connectToDb<verbose>();
    }
    LOG_IF(INFO, verbose >= 2) << ((manager_.checkConnection()) ? "connection esablished\n" : "cannot connect\n");
    return 0;
}


template<unsigned long segment_size,hash_function hash>
requires is_divisible<segment_size, hash_function_size[hash]>
         && is_divisible<total_block_size, segment_size>
template<verbose_level verbose, data_insetion_strategy strategy>
int FileParsingService<segment_size,hash>::process_directory(std::string &trainDir) {
    namespace fs = std::filesystem;
    fs::path pp;
    try {
        pp = fs::canonical(trainDir);

        if (!fs::exists(pp)) {
            LOG_IF(ERROR, verbose >= 1) << vformat("\"%s\" no such file or directory\n", pp.string().c_str());
            return return_codes::error_occured;

        }
        if (!fs::is_directory(pp)) {
            LOG_IF(ERROR, verbose >= 1)
                            << vformat("\"%s\" is not a directory use procesFile for files\n", pp.string().c_str());
            return return_codes::error_occured;
        }

    } catch (const fs::filesystem_error &e) {
        LOG_IF(ERROR, verbose >= 1) << vformat("Filesystem error : %s , error code %d\n", e.what(), e.code());
        return -3;
    }

    auto dir_id = manager_.template create_directory<2>(pp.string());
    if (dir_id == return_codes::error_occured || dir_id == return_codes::already_exists) {
        LOG_IF(ERROR, verbose >= 1) << vformat("Error occurred during directory creation.\nDirectory path \"%s\"!",
                                               trainDir.c_str());
        return dir_id;
    }


    for (const auto &entry: fs::recursive_directory_iterator(pp)) {
        if (!fs::is_directory(entry)) {
            auto file = fs::canonical(entry.path()).string();//todo create method for file
            auto size = fs::file_size(entry);
            auto file_id = manager_.template create_file<verbose>(file, dir_id, size);

            if (file_id == return_codes::already_exists) {
                if (strategy == preserve_old) {
                    continue;
                } else {
                    auto res = manager_.template delete_file<verbose>(file, file_id);

                    if (res == return_codes::error_occured) {
                        LOG_IF(ERROR, verbose >= 1)
                                        << vformat("Error occurred during insert/replace.\n File path \"%s\"!",
                                                   file.c_str());
                        return file_id;
                    }
                    file_id = manager_.template create_file<verbose>(file, dir_id, size);
                }
            }

            if (file_id == return_codes::error_occured) {
                LOG_IF(ERROR, verbose >= 1)
                                << vformat("Error occurred during file creation.\n File path \"%s\"!", file.c_str());
                return file_id;
            }
            std::basic_ifstream<symbol_type> in(entry.path());

            manager_.template insert_file_from_stream<verbose>(file, in);
            manager_.template finish_file_processing<verbose>(file, file_id);//todo try to integrate/remove
        }
    }

    return 0;


}


#endif //SERVICE_SERVICEFILEINTERFACE_H
