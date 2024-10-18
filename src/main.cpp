#include <vector>
#include <filesystem>
#include <iostream>

#include <fstream>

#include <common.h>
#include "FileService.h"


int main() {
//todo hash segments on client side

    auto resss = get_hash_str("3");
    db_services::dbManager<64> sample;
    auto c=db_services::default_configuration();
    c.set_dbname("testbytea");
    sample.setCString(c);
    sample.connectToDb();
    //auto resss2=sample.execute_in_transaction(db_services::get_hash_str, {"3"}).value_or("");
    auto resss2=get_hash_str<SHA_256>("3");

    return 0;
}