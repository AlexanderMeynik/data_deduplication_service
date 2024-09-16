#ifndef SERVICE_SERVICEFILEINTERFACE_H
#define SERVICE_SERVICEFILEINTERFACE_H

#include <vector>
#include <array>
#include <string>
#include <iostream>
#include <openssl/sha.h>
#include "../common/myconcepts.h"
#include "../dbUtils/lib.h"
#include <sstream>
#include <iomanip>
#include <filesystem>
constexpr int block_size=2048;
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
using db_services::dbManager;
template<unsigned long segment_size, char fileEndDelim=23>
requires IsDiv<segment_size, SHA256size>
class FileParsingService
{
    using CurrBlock = block<segment_size,block_size>;
    template<unsigned short verbose = 0,db_usage_strategy str=use>
    explicit FileParsingService(std::string &dbName);


    template<segmenting_strategy ss=use_blocks,unsigned short verbose = 0>
    int process_directory(std::string& trainDir);

    template<segmenting_strategy ss=use_blocks,unsigned short verbose = 0>
    int load_directory(std::string& trainDir);
private:
    dbManager<segment_size> manager_;
};

template<unsigned long segment_size, char fileEndDelim>
requires IsDiv<segment_size, SHA256size>
template<segmenting_strategy ss, unsigned short verbose>
int FileParsingService<segment_size, fileEndDelim>::load_directory(std::string &trainDir) {//todo добавить дополнительный параметр куда
    /*
     * fs::path parent_dir = full_path.parent_path();

    // Check if the parent directory exists, if not, create it
    if (!fs::exists(parent_dir)) {
        std::cout << "Creating directories: " << parent_dir << '\n';
        fs::create_directories(parent_dir);
    }
     */
    //todo создать метод для получения всех имён файлов
    //todo создать метод для построения файловой иерархии(за счёт fs::create_directories можно все папки потсроить заранее)
    //todo метод(в бд) для считывания файла по его имени/пути(gin или какй-нибудь бругой индекс тут пригодяться+tsvector)

    return 0;
}

template<unsigned long segment_size, char fileEndDelim>
requires IsDiv<segment_size, SHA256size>
template<unsigned short verbose, db_usage_strategy str>
FileParsingService<segment_size, fileEndDelim>::FileParsingService(std::string &dbName){
    auto CString=db_services::basic_configuration();
    CString.dbname=dbName;
    CString.update_format();

    manager_=dbManager<segment_size>(CString);

    if constexpr (str==create)
    {
        manager_.template create<verbose>();
        manager_.template fill_schemas<verbose>();
    } else
    {
        manager_.template connectToDb<verbose>();
    }
}



template<unsigned long segment_size, char fileEndDelim>
requires IsDiv<segment_size, SHA256size>
template<segmenting_strategy ss,unsigned short verbose>
int FileParsingService<segment_size, fileEndDelim>::process_directory(std::string& trainDir) {


    namespace fs = std::filesystem;

    fs::path pp;
    try {
        pp=fs::canonical(trainDir);//todo сохранить это добро в таблицу с директориями
        //todo добавить в таблицу  с файлами строку с директориями
        //todo в файлах будет храниться их абсолютный путь

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

    for (const auto& entry : fs::recursive_directory_iterator(pp)) {
        if(!fs::is_directory(pp)){
            //todo добавить ограничение(в бд) на уникальность файлов
            //todo реализовать процедуру поблочной отправки файла
            //todo создать файл
            if constexpr (ss==segmenting_strategy::use_blocks)
            {
                //std::cout<<"blocks"<<'\n';
                CurrBlock block;
                auto size=fs::file_size(entry);//проверить соответствие реальному
                unsigned long blocks=fs::file_size(entry)/block_size;
                std::ifstream in(entry.path());//todo проверить освобождение in
                for (int i = 0; i < blocks; ++i) {
                    for (int j = 0; j < block_size; ++j) {
                        segment<segment_size>& curr_s =&block[i][j];
                        //curr_s.fill(0);
                        for (int k = 0; k < segment_size; ++k) {
                            curr_s[k] = in.get();
                        }

                    }
                    manager_.process_file_block(block,pp.string());
                }
                segvec<segment_size> last_block;

                while (!in.eof()) {
                    auto curr_s = segment<segment_size>();
                    curr_s.fill(0);
                    for (int j = 0; j < segment_size; ++j) {
                        if (in.eof() || in.peek() == -1) {
                            curr_s[j] = 23;//ETB symbol
                            if (j == 0) {
                                //return source_;
                            }
                            last_block.push_back(curr_s);
                           // return source_;
                        }
                        curr_s[j] = in.get();
                    }
                    last_block.push_back(curr_s);
                }
            }
            else
            {
                //todo copy the mthod ge_bulk_blocks
                std::cout<<"full"<<'\n';
            }
        }
    }

    return 0;


}




using buf64 = segvec<64>;

template<unsigned long segment_size>
requires IsDiv<segment_size, SHA256size>
std::istream &operator>>(std::istream &source_, segvec<segment_size> &buf) {
    while (!source_.eof()) {
        auto curr_s = segment<segment_size>();
        curr_s.fill(0);
        for (int j = 0; j < segment_size; ++j) {
            if (source_.eof() || source_.peek() == -1) {
                curr_s[j] = 23;//ETB symbol
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

template<unsigned long segment_size>
requires IsDiv<segment_size, SHA256size>
std::ostream &operator<<(std::ostream &target_, const segvec<segment_size> &buf) {
    for (auto &elem: buf) {
        for (auto &symbol: elem) {
            if (symbol == 23) {
                return target_;
            }
            target_.put(symbol);
        }
    }
    return target_;
}


#endif //SERVICE_SERVICEFILEINTERFACE_H
