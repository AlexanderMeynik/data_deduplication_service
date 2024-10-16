
#include "testClasses.h"

#define FileServiceTest

#include <filesystem>


static fs::path fix_dir = "../test_data/fixture/";
static fs::path res_dir = "../test_data/res/";

class ServiceFileTest : public ::testing::Test {
public:
    void SetUp() override {
        dbName = "dedup_test_" + std::to_string(2);
        c_str = default_configuration();
        c_str.set_dbname(dbName);
        file_service_.db_load<db_usage_strategy::create>(dbName);
        ASSERT_TRUE(file_service_.check_connection());
        conn_ = connect_if_possible(c_str).value_or(nullptr);

    }

    void TearDown() override {
        file_service_.db_drop(dbName);
        diconnect(conn_);
    }

protected:
    inline static FileParsingService<64> file_service_;
    inline static std::string dbName;
    inline static my_conn_string c_str;
    inline static conPtr conn_;

};

TEST_F(ServiceFileTest, test_created_db_acess) {
    ASSERT_TRUE(file_service_.check_connection());
}



int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    google::SetVLOGLevel("*", 3);
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);


    return RUN_ALL_TESTS();
}