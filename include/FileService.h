#ifndef DATA_DEDUPLICATION_SERVICE_FILESERVICE_H
#define DATA_DEDUPLICATION_SERVICE_FILESERVICE_H

#include <vector>
#include <array>
#include <string>
#include <iostream>
#include <openssl/sha.h>
#include <common.h>
#include "dbManager.h"
#include <sstream>
#include <iomanip>
#include <filesystem>
#include "common.h"
/// file services namespace
namespace file_services {

    /**
     * @ingroup utility
     * Database usage strategy
     */
    enum db_usage_strategy {
        /// if database exist connection will be established
        use,
        /// create new database if it doesn't exist
        create
    };
    /**
     * @ingroup utility
     * File/directories insertion startegy
     */
    enum data_insetion_strategy {
        /// will ignore files that already exist
        preserve_old,
        /// will replace file conents
        replace_with_new
    };
    /**
     * @ingroup utility
     * File/directory handling strategy during load process
     */
    enum data_retrieval_strategy {
        ///will leave data as is
        persist,
        ///will delete requested data from database
        remove_
    };
    /**
     * @ingroup utility
     * Defines what will be done if destination directory does not exist
     */
    enum root_directory_handling_strategy {
        //will return an error code
        no_create_main,
        //will create this directory using create_directories
        create_main
    };

    using db_services::dbManager;

    /**
     * This function checks existence of a canonical file path for file_path
     * @param file_path
     * @return canonical file_path or error code
     */
    tl::expected<std::string, int> check_file_existence_(std::string_view file_path);

    /**
     * This function checks existence of a  canonical directory path for dir_path
     * @param dir_path
     * @return canonical dir_path or error code
     */
    tl::expected<std::string, int> check_directory_existence_(std::string_view dir_path);

    namespace fs = std::filesystem;

    /**
     * Calculates lexically normal absolute path
     * @param path
     */
    inline fs::path get_normal_abs(fs::path &path) {
        return fs::absolute(path).lexically_normal();
    }


    inline fs::path get_normal_abs(fs::path &&path) {
        return fs::absolute(path).lexically_normal();
    }

    /**
     * this class handles file/directory management and uses @ref db_services::dbManager "dbManager" to perform calls
     * @tparam segment_size
     */
    template<unsigned long segment_size>
    class FileParsingService {
    public:
        using index_type = db_services::index_type;

        FileParsingService()
        = default;


        /**
         * This function handles database open/crete action
         * If db_usage_str==create this function will create new database
         * othervice it'll try to open existing database.
         * @tparam db_usage_str
         * @tparam hash
         * @param dbName
         * @param configuration_file
         */
        template<db_usage_strategy db_usage_str = use, hash_function hash = SHA_256>
        requires is_divisible<segment_size, hash_function_size[hash]>
        int db_load(std::string &dbName, std::string_view configuration_file = db_services::cfile_name);

        int db_drop(std::string_view dbName) {
            auto res = manager_.drop_database(dbName);
            return res;
        };

        /**
         * Processes all files in the given directory runs @ref process_file() "process_file()" for each file
         * @tparam data_insertion_str
         * @tparam hash
         * @param dir_path
         */
        template<data_insetion_strategy data_insertion_str = preserve_old, hash_function hash = SHA_256>
        requires is_divisible<segment_size, hash_function_size[hash]>
        int process_directory(std::string_view dir_path);

        /** @details Creates entry for file in database.
         * @details Load file segments into temp table.
         * @details Upsets segment entiries
         * @details Add file data.
         * @tparam data_insertion_str
         * @tparam existence_checks
         * @tparam hash
         * @param file_path
         * @typedef ss
         */
        template<data_insetion_strategy data_insertion_str = preserve_old, bool existence_checks = true, hash_function hash = SHA_256>
        requires is_divisible<segment_size, hash_function_size[hash]>
        int process_file(std::string_view file_path);


        /**
         * Retrieves directory from database to to_dir
         * @tparam root_directory_str
         * @tparam data_retr_str
         * @param from_dir
         * @param to_dir
         */
        template<root_directory_handling_strategy root_directory_str = no_create_main,
                data_retrieval_strategy data_retr_str = persist>
        int load_directory(std::string_view from_dir, std::string_view to_dir);


        /**
         * Retrieves file from database to to_file
         * @tparam dir_s
         * @tparam data_retr_str
         * @param from_file
         * @param to_file
         */
        template<root_directory_handling_strategy dir_s = no_create_main,
                data_retrieval_strategy data_retr_str = persist, bool from_load_dir = false>
        int load_file(std::string_view from_file, std::string_view to_file,
                      index_type file_id = index_vals::empty_parameter_value);

        /**
         * Deletes file entry and data from database
         * @param file_path
         */
        int delete_file(std::string_view file_path);

        /**
         * Deletes database entry and data for directory
         * @param dir_path
         */
        int delete_directory(std::string_view dir_path);

        bool check_connection() {
            return manager_.check_connection();
        }

        int clear_segments() {
            clk.tik();
            auto rr = manager_.clear_segments();
            clk.tak();
            return rr;
        }

        /**
         * Wrapper for assoctiated dbManger member function
         * @see @ref  dbManager::execute_in_transaction() "execute_in_transaction()"
        */
        template<typename ResType1, typename ... Args>
        tl::expected<ResType1, int> execute_in_transaction(ResType1
                                                           (*call)(db_services::trasnactionType &, Args ...),
                                                           Args &&... args) {
            return manager_.execute_in_transaction(call, std::forward<Args>(args)...);
        }

        /**
         * Wrapper for assoctiated dbManger member function
         * @see @ref  dbManager::execute_in_transaction(std::function< ResultType(trasnactionType &, Args ...)> &  call,Args &&... 	args ) "execute_in_transaction()"
        */
        template<typename ResType1, typename ... Args>
        tl::expected<ResType1, int>
        execute_in_transaction(std::function<ResType1(db_services::trasnactionType &, Args ...)> &call,
                               Args &&... args) {
            return manager_.execute_in_transaction(call, std::forward<Args>(args)...);
        }


    private:
        dbManager<segment_size> manager_;
    };

    template<unsigned long segment_size>
    int FileParsingService<segment_size>::delete_file(std::string_view file_path) {
        fs::path file_abs_path = get_normal_abs(file_path);

        auto res = manager_.delete_file(file_abs_path.string());

        if (res == return_codes::error_occured) {
            VLOG(1) << vformat("Error occurred during directory \"%s\" removal.\n", file_abs_path.c_str());
        }
        return res;
    }

    template<unsigned long segment_size>

    int FileParsingService<segment_size>::delete_directory(std::string_view dir_path) {
        fs::path dir_abs_path = get_normal_abs(dir_path);


        auto res = manager_.delete_directory(dir_abs_path.string());

        if (res == return_codes::error_occured) {
            VLOG(1) << vformat("Error occurred during directory \"%s\" removal.\n", dir_abs_path.c_str());
        }
        return res;
    }


    template<unsigned long segment_size>
    //todo create regular expression grabber for files
    template<root_directory_handling_strategy dir_s, data_retrieval_strategy rr, bool from_load_dir>
    int FileParsingService<segment_size>::load_file(std::string_view from_file, std::string_view to_file,
                                                    index_type file_id) {
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
                    VLOG(2)
                                    << vformat("Root directory \"%s\" was created successfully",
                                               parent_dir_path.c_str());
                } else {
                    VLOG(1)
                                    << vformat("\"%s\" no such file or directory\n", parent_dir_path.c_str());
                    return return_codes::error_occured;
                }
            }
            if constexpr (dir_s == no_create_main) {

                if (!fs::is_directory(parent_dir_path)) {
                    VLOG(1) << vformat("\"%s\" is not a directory use procesFile for files\n",
                                       parent_dir_path.c_str());
                    return return_codes::error_occured;
                }
            }

        }
        catch (const fs::filesystem_error &e) {
            VLOG(1) << vformat("Filesystem error : %s , error code %d\n", e.what(), e.code());
            return return_codes::error_occured;
        }

        std::basic_ofstream<symbol_type> out(to_file_path.c_str());


        auto stream_res = manager_.get_file_streamed(from_file_path.string(), out, file_id);


        out.close();

        if (stream_res == return_codes::error_occured) {
            VLOG(1) << vformat("Error occurred during "
                               "file \"%s\" streaming",
                               from_file_path.c_str());
            return stream_res;
        }


        if constexpr (!from_load_dir && rr == data_retrieval_strategy::remove_) {

            auto del_res = manager_.delete_file(from_file_path.string());

            if (del_res == return_codes::error_occured) {
                VLOG(1) << vformat("Error occurred during "
                                   "file \"%s\" deletion",
                                   from_file_path.c_str());
                return del_res;
            }
        }

        return 0;

    }


    template<unsigned long segment_size>
    template<root_directory_handling_strategy dir_s, data_retrieval_strategy rr>
    int FileParsingService<segment_size>::load_directory(std::string_view from_dir, std::string_view to_dir) {
        namespace fs = std::filesystem;
        fs::path new_dir_path;
        fs::path from_dir_path = get_normal_abs(from_dir);

        auto files = manager_.get_all_files(from_dir_path.string());
        if (files.empty()) {
            VLOG(1) << vformat("No files found for directory %s", from_dir.data());
        }
        try {
            if (!fs::exists(to_dir)) {
                if constexpr (dir_s == create_main) {
                    fs::create_directories(to_dir);
                    VLOG(2)
                                    << vformat("Root directory \"%s\" was created successfully", to_dir.data());
                } else {
                    VLOG(1) << vformat("\"%s\" no such file or directory\n", to_dir.data());
                    return return_codes::error_occured;
                }
            }
            if constexpr (dir_s == no_create_main) {

                if (!fs::is_directory(to_dir)) {
                    VLOG(1) << vformat("\"%s\" is not a directory change to_dir path\n",
                                       to_dir.data());
                    return return_codes::error_occured;
                }
            }
            new_dir_path = get_normal_abs(to_dir);

        } catch (const fs::filesystem_error &e) {
            VLOG(1) << vformat("Filesystem error : %s , error code %d\n", e.what(), e.code());
            return return_codes::error_occured;
        }


        for (const std::pair<db_services::index_type, std::string> &pair: files) {
            std::string file_path = pair.second;

            auto p_t_s = new_dir_path / fs::path(pair.second).lexically_relative(from_dir_path);


            auto result = this->template load_file<root_directory_handling_strategy::create_main, rr, true>(pair.second,
                                                                                                            p_t_s.string(),
                                                                                                            pair.first);
            if (result == return_codes::error_occured) {
                VLOG(1) << vformat("Error occurred during "
                                   "file \"%s\" retrieval",
                                   from_dir_path.c_str());
                continue;
            }
        }

        if constexpr (rr == data_retrieval_strategy::remove_) {
            manager_.delete_directory(from_dir_path.string());
        }
        return 0;
    }

    template<unsigned long segment_size>
    template<db_usage_strategy str, hash_function hash>
    requires is_divisible<segment_size, hash_function_size[hash]>
    int FileParsingService<segment_size>::db_load(std::string &dbName, std::string_view configuration_file) {
        auto CString = db_services::load_configuration(configuration_file);
        CString.set_dbname(dbName);

        manager_ = dbManager<segment_size>(CString);


        if constexpr (str == create) {

            auto reusult = manager_.create_database(dbName);

            if (reusult == return_codes::error_occured) {
                VLOG(1) << vformat("Error occurred during database \"%s\" creation\n", dbName.c_str());
                return return_codes::error_occured;
            }

            reusult = manager_.template fill_schemas<hash>();


            if (reusult == return_codes::error_occured) {
                VLOG(1)
                                << vformat("Error occurred during database's \"%s\" schema's creation\n",
                                           dbName.c_str());
                return return_codes::error_occured;
            }
        } else {

            auto res = manager_.connectToDb();
            if (res == return_codes::error_occured) {
                VLOG(1)
                                << vformat("Error occurred during database's \"%s\" schema's creation\n",
                                           dbName.c_str());
                return return_codes::error_occured;
            }

        }
        VLOG(2) << ((manager_.check_connection()) ? "connection established\n" : "cannot connect\n");
        return 0;
    }


    template<unsigned long segment_size>
    template<data_insetion_strategy strategy, bool existence_checks, hash_function hash>
    requires is_divisible<segment_size, hash_function_size[hash]>
    int FileParsingService<segment_size>::process_file(std::string_view file_path) {

        namespace fs = std::filesystem;
        std::string file;
        if constexpr (existence_checks) {
            auto result = check_file_existence_(file_path);
            if (!result.has_value()) {
                return return_codes::error_occured;
            }
            file = result.value();
        } else {
            file = file_path;
        }

        auto size = fs::file_size(file);


        clk.tik();
        auto file_id = manager_.create_file(file, size);
        clk.tak();


        if (file_id == return_codes::already_exists) {
            if (strategy == preserve_old) {
                return return_codes::already_exists;
            }

            auto res = manager_.delete_file(file, file_id);


            if (res == return_codes::error_occured) {
                VLOG(1)
                                << vformat("Error occurred during insert/replace.\n File path \"%s\"!",
                                           file.c_str());
                return return_codes::error_occured;
            }
            file_id = manager_.create_file(file, size);
        }

        if (file_id == return_codes::error_occured) {
            VLOG(1)
                            << vformat("Error occurred during file creation.\n File path \"%s\"!", file.c_str());
            return file_id;
        }
        std::basic_ifstream<symbol_type> in(file);

        clk.tik();
        auto res1 = manager_.insert_file_from_stream(file, in, size);
        clk.tak();

        if (res1 == return_codes::error_occured) {
            VLOG(1)
                            << vformat("Error occurred during file contents streaming.\n File path \"%s\"!",
                                       file.c_str());
            return res1;
        }
        clk.tik();
        res1 = manager_.template finish_file_processing<hash>(file, file_id);
        clk.tak();

        if (res1 == return_codes::error_occured) {
            VLOG(1)
                            << vformat("Error occurred during file contents processing.\n File path \"%s\"!",
                                       file.c_str());
            return res1;
        }
        return 0;
    }


    template<unsigned long segment_size>
    template<data_insetion_strategy strategy, hash_function hash>
    requires is_divisible<segment_size, hash_function_size[hash]>
    int FileParsingService<segment_size>::process_directory(std::string_view dir_path) {
        namespace fs = std::filesystem;
        fs::path pp;

        auto result = check_directory_existence_(dir_path);
        if (!result.has_value()) {
            return return_codes::error_occured;
        }
        pp = result.value();

        for (const auto &entry: fs::recursive_directory_iterator(pp)) {
            if (!fs::is_directory(entry)) {
                auto file = fs::canonical(entry.path()).string();
                clk.tik();
                auto results = this->template process_file<strategy, false, hash>(file);
                clk.tak();
                if (results == return_codes::already_exists) {
                    continue;
                } else if (results == return_codes::error_occured) {
                    return results;
                }
            }
        }

        return 0;


    }

}
#endif //DATA_DEDUPLICATION_SERVICE_FILESERVICE_H
