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


template<unsigned long segment_size,char fill=23>
requires is_divisible<segment_size, SHA256size>
std::istream &operator>>(std::istream &source_, segvec<segment_size> &buf);
template<unsigned long segment_size,char fill=23>
requires is_divisible<segment_size, SHA256size>
std::ostream &operator<<(std::ostream &target_, const segvec<segment_size> &buf);
enum segmenting_strategy
{
    use_blocks,
    full_file_send,
    other
};

enum db_usage_strategy
{
    use,
    create
};

enum data_retrieval_strategy
{
    persist,
    remove_
};

using db_services::dbManager;
template<unsigned long segment_size, char fileEndDelim=23>
requires is_divisible<segment_size, SHA256size> && is_divisible<total_block_size,segment_size>
class FileParsingService
{
public:
    static constexpr unsigned long long block_size= total_block_size / segment_size;
    FileParsingService()
    = default;

    using CurrBlock = block<segment_size,block_size>;
    template<verbose_level verbose = 0,db_usage_strategy str=use>
    int db_load (std::string &dbName);


    template<verbose_level verbose = 0,segmenting_strategy ss=use_blocks>
    int process_directory(std::string& trainDir);

    template<verbose_level verbose = 0,segmenting_strategy ss=use_blocks,data_retrieval_strategy rr=persist>
    int load_directory(std::string& trainDir,std::string &to_dir);
private:
    dbManager<segment_size> manager_;
};

template<unsigned long segment_size, char fileEndDelim>
requires is_divisible<segment_size, SHA256size>
         && is_divisible<total_block_size,segment_size>
template<verbose_level verbose,segmenting_strategy ss,data_retrieval_strategy rr>
int FileParsingService<segment_size, fileEndDelim>::load_directory(std::string &trainDir,std::string &to_dir) {
    namespace fs = std::filesystem;
    fs::path new_abs;
    fs::path curr_abs=fs::canonical(trainDir);
    try {
        new_abs=fs::canonical(to_dir);

        if(!fs::exists(new_abs)) {
            LOG_IF(ERROR, verbose >= 1) << vformat("\"%s\" no such file or directory\n", new_abs.string().c_str());
            return -1;
        }

        if(!fs::is_directory(new_abs)) {
            LOG_IF(ERROR, verbose >= 1) << vformat("\"%s\" is not a directory use procesFile for files\n", new_abs.string().c_str());
            return -2;
        }

    } catch (const fs::filesystem_error& e) {
        LOG_IF(ERROR, verbose >= 1) <<vformat("Filesystem error : %s , error code %d\n",e.what(),e.code());
        return -3;
    }

    auto files=manager_.template get_all_files<verbose>(curr_abs.string());
    //todo проверить на сложных иерархиях
    //todo метод(в бд?) для считывания файла по его имени/пути(gin или какй-нибудь бругой индекс тут пригодяться+tsvector)
    for (const auto & e:files) {
        auto p_t_s=new_abs/fs::path(e).lexically_relative(curr_abs);

        fs::path parent_dir = p_t_s.parent_path();
        if (!fs::exists(parent_dir)) {
            fs::create_directories(parent_dir);
            LOG_IF(INFO, verbose >= 2) <<vformat("Creating directories:  %s \n",parent_dir.string().c_str());

        }
        std::ofstream out(p_t_s);

        if constexpr (ss==segmenting_strategy::use_blocks)
        {
            CurrBlock block;
            manager_.template get_file_streamed<verbose>(e, out);
        }
        else
        {
            segvec<segment_size> buff=manager_.get_file_segmented(e);//todo test
            out<<buff;


        }
        out.close();
        if constexpr (rr==data_retrieval_strategy::remove_)
        {
            //todo delete files/directories
        }
    }
    return 0;
}

template<unsigned long segment_size, char fileEndDelim>
requires is_divisible<segment_size, SHA256size>
         && is_divisible<total_block_size,segment_size>
template<verbose_level verbose, db_usage_strategy str>
int FileParsingService<segment_size, fileEndDelim>::db_load(std::string &dbName) {
    auto CString=db_services::default_configuration();
    CString.set_dbname(dbName);

    manager_=dbManager<segment_size>(CString);

    if constexpr (str==create)
    {
        manager_.template create<verbose>();
        manager_.template fill_schemas<verbose>();
    } else
    {
        manager_.template connectToDb<verbose>();
    }
    LOG_IF(INFO,verbose>=2)<<((manager_.checkConnection())?"connection esablished\n":"cannot connect\n");
    return 0;
}



template<unsigned long segment_size, char fileEndDelim>
requires is_divisible<segment_size, SHA256size>
         && is_divisible<total_block_size,segment_size>
template<verbose_level verbose,segmenting_strategy ss>
int FileParsingService<segment_size, fileEndDelim>::process_directory(std::string& trainDir) {
    namespace fs = std::filesystem;
    fs::path pp;
    try {
        pp=fs::canonical(trainDir);

        if(!fs::exists(pp)) {
            LOG_IF(ERROR, verbose >= 1) << vformat("\"%s\" no such file or directory\n", pp.string().c_str());
            return -1;
        }

        if(!fs::is_directory(pp)) {
            LOG_IF(ERROR, verbose >= 1) << vformat("\"%s\" is not a directory use procesFile for files\n", pp.string().c_str());
            return -2;
        }

    } catch (const fs::filesystem_error& e) {
        LOG_IF(ERROR, verbose >= 1) <<vformat("Filesystem error : %s , error code %d\n",e.what(),e.code());
        return -3;
    }

    auto dir_id=manager_.template create_directory<2>(pp.string());


    for (const auto& entry : fs::recursive_directory_iterator(pp)) {
        if(!fs::is_directory(entry)){
            auto file=fs::canonical(entry.path()).string();
            auto size=fs::file_size(entry);//проверить соответствие реальному
            auto file_id=manager_.template create_file<verbose>(file,dir_id,size);



            if constexpr (ss==segmenting_strategy::use_blocks)
            {
                CurrBlock block;

                unsigned long blocks=size/(block_size*segment_size);
                std::ifstream in(entry.path());
                for (int i = 0; i < blocks; ++i) {
                    for (int j = 0; j < block_size; ++j) {
                        //curr_s.fill(0);
                        for (int k = 0; k < segment_size; ++k) {
                            block[j][k] = in.get();
                        }

                    }
                    manager_.template stream_segment_array<verbose>(block, file, i);
                }
                segvec<segment_size> last_block;

                while (!in.eof()) {
                    auto curr_s = segment<segment_size>();
                    curr_s.fill(23);
                    for (int j = 0; j < segment_size; ++j) {
                        if (in.eof() || in.peek() == -1) {
                            curr_s[j] = 23;//ETB symbol
                            if (j == 0) {
                                goto end;
                            }
                            last_block.push_back(curr_s);
                            goto end;
                        }
                        curr_s[j] = in.get();
                    }
                    last_block.push_back(curr_s);
                }
                end: in.close();
                manager_.template stream_segment_array<verbose>(last_block, file, blocks);
                manager_.template finish_file_processing<verbose>(file,file_id);
            }
            else
            {
                segvec<segment_size> buff;
                std::ifstream in(file);
                in>>buff;

                manager_.template stream_segment_array<verbose>(buff, file, 0);
                manager_.template finish_file_processing<verbose>(file,file_id);
                in.close();
            }
        }
    }

    return 0;


}




using buf64 = segvec<64>;

template<unsigned long segment_size,char fill>
requires is_divisible<segment_size, SHA256size>
std::istream &operator>>(std::istream &source_, segvec<segment_size> &buf) {
    while (!source_.eof()) {
        auto curr_s = segment<segment_size>();
        curr_s.fill(fill);
        for (int j = 0; j < segment_size; ++j) {
            if (source_.eof() || source_.peek() == -1) {
                curr_s[j] =fill;//ETB symbol
                if (j == 0) {
                    return source_;
                }
                buf.push_back(curr_s);
                return source_;
            }
            curr_s[j] = source_.get();
        }
        buf.push_back(curr_s);
    }
    return source_;
}

template<unsigned long segment_size,char fill>
requires is_divisible<segment_size, SHA256size>
std::ostream &operator<<(std::ostream &target_, const segvec<segment_size> &buf) {
    for (auto &elem: buf) {
        for (auto &symbol: elem) {
            if (symbol == fill) {
                return target_;
            }
            target_.put(symbol);
        }
    }
    return target_;
}


#endif //SERVICE_SERVICEFILEINTERFACE_H
