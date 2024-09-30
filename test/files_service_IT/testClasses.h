#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ServiceFileInterface.h"

using namespace db_services;
#ifndef SOURCE_SERVICE_TESTCLASSES_H
#define SOURCE_SERVICE_TESTCLASSES_H
class DbManagerTest : public ::testing::Test {
public:
    void SetUp() override
    {
        auto c_string= default_configuration();
        c_string.set_dbname(dbName);
        manager_.setCString(c_string);
        auto res =manager_.create_database();

    }
    void TearDown() override
    {
        manager_.drop_database(dbName);
        time+=1;
        dbName=dbName_+std::to_string(time);
    }
protected:
    dbManager<64> manager_;
    std::string dbName_="dedup_test_";
    int time=0;
    std::string dbName=dbName_+std::to_string(time);

};

class ServiceFileTest : public ::testing::Test {
public:
    void SetUp() override
    {
        auto res=serv.db_load<create>(dbName,"res/config.txt");
        std::ifstream in("../res/config.txt");

    }
    void TearDown() override
    {

        serv.db_drop(dbName);
        dbName=dbName_+std::to_string(++time);
    }
protected:
    FileParsingService<64> serv;
    std::string dbName_="dedup_test_";
    int time=0;
    std::string dbName=dbName_+std::to_string(time);

};

#endif //SOURCE_SERVICE_TESTCLASSES_H
