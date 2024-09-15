#include <iostream>
#include <ostream>
#include "dbUtils/lib.h"
#include "cassert"
#include "FileUtils/ServiceFileInterface.h"



std::stringstream ss{"1111111111111111"
                     "2222222222222222"
                     "3333333333333333"
                     "4444444444444444"
                     "5555555555555555"
                     "6666666666666666"
                     "7777777777777777"
                     "8888888888888888"
                     "9999999999999999"
                     "1111111111111111"
                     "2222222222222222"
                     "3333333333333333"
                     "4444444444444444"
                     "5555555555555555"
                     "6666666666666666"
                     "7777777777777777"
                     "8888888888888888"
                     "9999999999999999"
                     "1111111111111111"
                     "2222222222222222"
                     "3333333333333333"
                     "4444444444444444"
                     "5555555555555555"
                     "6666666666666666"
                     "7777777777777777"
                     "8888888888888888"
                     "9999999999999999"
                     "1111111111111111"
                     "2222222222222222"
                     "3333333333333333"
                     "4444444444444444"
                     "5555555555555555"
                     "6666666666666666"
                     "7777777777777777"
                     "8888888888888888"
                     "9999999999999999"
                     "1111111111111111"
                     "2222222222222222"
                     "3333333333333333"
                     "4444444444444444"
                     "5555555555555555"
                     "6666666666666666"
                     "7777777777777777"
                     "8888888888888888"
                     "9999999999999999"
                     "1111111111111111"
                     "2222222222222222"
                     "3333333333333333"
                     "4444444444444444"
                     "5555555555555555"
                     "6666666666666666"
                     "7777777777777777"
                     "8888888888888888"
                     "9999999999999999"
                     "1111111111111111"
                     "2222222222222222"
                     "3333333333333333"
                     "4444444444444444"
                     "5555555555555555"
                     "6666666666666666"
                     "7777777777777777"
                     "8888888888888888"
                     "9999999999999999"
                     "1111111111111111"
                     "2222222222222222"
                     "3333333333333333"
                     "4444444444444444"
                     "5555555555555555"
                     "6666666666666666"
                     "7777777777777777"
                     "8888888888888888"
                     "9999999999999999"
                     "1111111111111111"
                     "2222222222222222"
                     "3333333333333333"
                     "4444444444444444"
                     "5555555555555555"
                     "6666666666666666"
                     "7777777777777777"
                     "8888888888888888"
                     "9999999999999999"
                     "1111111111111111"
                     "2222222222222222"
                     "3333333333333333"
                     "4444444444444444"
                     "5555555555555555"
                     "6666666666666666"
                     "7777777777777777"
                     "8888888888888888"
                     "9999999999999999"
                     "1111111111111111"
                     "2222222222222222"
                     "3333333333333333"
                     "4444444444444444"
                     "5555555555555555"
                     "6666666666666666"
                     "7777777777777777"
                     "8888888888888888"
                     "9999999999999999"
                     "1111111111111111"
                     "2222222222222222"
                     "3333333333333333"
                     "4444444444444444"
                     "5555555555555555"
                     "6666666666666666"
                     "7777777777777777"
                     "8888888888888888"
                     "9999999999999999"
                     "1111111111111111"
                     "2222222222222222"
                     "3333333333333333"
                     "4444444444444444"
                     "5555555555555555"
                     "6666666666666666"
                     "7777777777777777"
                     "8888888888888888"
                     "9999999999999999"
                     "1111111111111111"
                     "2222222222222222"
                     "3333333333333333"
                     "4444444444444444"
                     "5555555555555555"
                     "6666666666666666"
                     "7777777777777777"
                     "8888888888888888"
                     "9999999999999999"
                     "1111111111111111"
                     "2222222222222222"
};

std::stringstream ss2{"1111111111111111"
                      "2222222222222222"
                      "3333333333333333"
                      "4444444444444444"
                      "1111111111111111"
                      "2222222222222222"
                      "3333333333333333"
                      "4444444444444444"};
std::stringstream ss3{"1111111111111111"
                      "2222222222222222"
                      "3333333333333333"
                      "4444444444444444"};

int main() {
    using namespace db_services;

    auto Cstring=basic_configuration();
    Cstring.dbname="deduplication642";
    Cstring.update_format();
    dbManager dd(Cstring);
    dd.create<2>();
    dd.fill_schemas<2>();



    std::stringstream& sss=ss;
    buf64 buf;
    sss >> buf;
    std::string ffname = "test";



    dd.insert_bulk_segments<2>(buf,ffname);



    buf64 out = dd.get_file_segmented<2>(ffname);


    std::stringstream os;
    os<<out;

    assert(os.str()==sss.str());

    auto str= dd.get_file_contents(ffname);

    assert(os.str()==str);
    std::cout<<ss.str();
    return 0;



    return 0;
}