
#include "testClasses.h"
#define FileServiceTest
//todo compare files/directories


class ServiceFileTest : public ::testing::Test {
public:
    void SetUp() override
    {
        auto res=serv.db_load<create>(this->dbName,"res/config.txt");
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

TEST_F(ServiceFileTest,test_created_db_acess)
{

}

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    google::SetVLOGLevel("*",3);
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);


    return RUN_ALL_TESTS();
}