#include <vector>
#include <filesystem>
#include <iostream>

#include <fstream>

#include <common.h>
#include "FileService.h"


int main() {
//todo hash segments on client side


    auto c_string=db_services::default_configuration();
    c_string.set_dbname("testbytea");


    db_services::dbManager<43> manager(c_string);
    auto res=manager.connectToDb();

    std::string hashable_thing("my_hash");

    std::vector<std::basic_string<unsigned char>> hashes;

    for (int i = 0; i < funcs.size(); ++i) {
        std::basic_string<unsigned char>md(hash_function_size[MD_5],' ');


        funcs[i](reinterpret_cast<const unsigned char *>(hashable_thing.data()), hashable_thing.size(),
                 md.data());
        hashes.push_back(md);

        std::ofstream file(std::string {"hash_output"}+hash_function_name[i]+".txt", std::ios::binary);
        file.write(reinterpret_cast<const char *>(hashes[i].data()), hash_function_size[i]);
       //file<<hashes[i];

        file.close();
    }
    std::function<db_services::ResType(db_services::trasnactionType&)> aa=[&hashes,&hashable_thing]
            (db_services::trasnactionType&txn)
    {
        txn.exec("delete from hash_test;");
        pqxx::stream_to copy_stream = pqxx::stream_to::raw_table(txn, "hash_test");

        for (int i = 0; i <hashes.size() ; ++i) {
            copy_stream
                    << std::make_tuple(
                            i,
                            pqxx::binary_cast(hashes[i]),
                            hashable_thing);

        }
        copy_stream.complete();
        return db_services::ResType();
    };

    std::function<std::vector<std::tuple<int, std::basic_string<unsigned char>,std::basic_string<unsigned char>>>
    (db_services::trasnactionType&)> aa2=[&hashes,&hashable_thing]
            (db_services::trasnactionType&txn)
    {
        std::vector<std::tuple<int, std::basic_string<unsigned char>,std::basic_string<unsigned char>>> res;
        std::string qq="select * from hash_test";

        for (auto [pos,hash,text]:
        txn.stream<int, pqxx::binarystring ,pqxx::binarystring>(qq)) {
            auto sz=hash_function_size[pos];
            std::basic_string<unsigned char> rr1(hash.data(),sz);

            res.emplace_back(pos,rr1,text.data());
        }
        return res;
    };
    std::vector<std::tuple<int, std::basic_string<unsigned char>,std::basic_string<unsigned char>>> rss;
    try {
        manager.execute_in_transaction(aa);
        rss=manager.execute_in_transaction(aa2).value();
    }
    catch (std::exception&ex)
    {
        std::cout<<ex.what()<<'\n';
    }
    int pos=0;
    for (auto elem:rss) {
        auto ss=get<1>(elem);
        auto ss2=get<2>(elem);
        std::cout<<ss.compare(hashes[pos++]);
    }




    return 0;
}