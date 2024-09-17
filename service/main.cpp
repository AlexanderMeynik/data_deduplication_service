#include <iostream>
#include <pqxx/pqxx>
#include <ostream>
#include <fstream>
#include "dbUtils/dbManager.h"


int main() {
    using namespace db_services;

    auto Cstring=default_configuration();
    Cstring.set_dbname("deduplication640");
    dbManager dd(Cstring);
    dd.create();
    dd.fill_schemas();

    return 0;
}