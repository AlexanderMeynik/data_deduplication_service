#include <iostream>
#include <pqxx/pqxx>
#include <ostream>
#include <fstream>
#include "dbUtils/lib.h"


int main() {
    using namespace db_services;

    auto Cstring=basic_configuration();
    Cstring.dbname="deduplication640";
    Cstring.update_format();
    dbManager dd(Cstring);
    dd.create();
    dd.fill_schemas();

    return 0;
}