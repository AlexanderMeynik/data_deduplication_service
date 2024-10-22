#include <vector>
#include <filesystem>
#include <iostream>


#include "common.h"
#include "FileService.h"


int main() {

    std::ifstream in("../test/bench_timers.txt");
    if (!in)
        return -1;
    while (!in.eof()) {
        std::string aa;
        std::getline(in, aa);
        std::cout << aa << '\n';
    }
/*
    auto resss = getHashStr("3");
    db_services::dbManager sample;
    auto c=db_services::defaultConfiguration();
    c.setDbname("testbytea");
    sample.setCString(c);
    sample.connectToDb();
    //auto resss2=sample.execute_in_transaction(db_services::get_hash_str, {"3"}).value_or("");
    auto resss2= getHashStr<SHA_256>("3");
    constexpr size_t N=100;
    *//*auto ressssss=[]<std::size_t ... Is>(std::index_sequence<Is...>)*//*

    auto func=[]<size_t N>(const std::array<std::string,N>&arr)
    {
        return [&arr] <std::size_t... Is> (std::index_sequence<Is...>)
        { return (arr[Is]+...); }
                (std::make_index_sequence<N>{});
    };
    auto a= makeArray<std::string,N>("1");
    auto ressssss=func(a);*/

    return 0;
}