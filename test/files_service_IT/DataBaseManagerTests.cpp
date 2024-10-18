#include "testClasses.h"


class DbMangement_tests : public ::testing::Test {
public:

    static void SetUpTestSuite() {
        dbName = "dedup_test_" + std::to_string(0);
        manager_ = dbManager<64>();
        c_str = defaultConfiguration();
        c_str.setDbname(dbName);
        manager_.createDatabase(dbName);
        ASSERT_TRUE(manager_.checkConnection());
    }

    static auto get_c_str() {
        return c_str;
    }

    static void TearDownTestSuite() {
        manager_.dropDatabase(dbName);
    }

protected:
    inline static mType manager_;
    inline static std::string dbName;
    inline static myConnString c_str;
};


TEST_F(DbMangement_tests, test_db_acess) {
    auto a = wrap_non_trans_function(&checkDatabaseExistence, {dbName});
    ASSERT_TRUE(a.has_value());
    ASSERT_EQ(a.value().size(), 1);
}

TEST_F(DbMangement_tests, testDb_disconnect) {
    manager_.disconnect();
    ASSERT_FALSE(manager_.checkConnection());
}

TEST_F(DbMangement_tests, test_schema_creation) {
    conPtr conn_ = connectIfPossible(c_str).value_or(nullptr);
    ASSERT_TRUE(checkConnection(conn_));
    auto res = manager_.connectToDb();
    ASSERT_EQ(res, returnCodes::ReturnSucess);

    auto pqres = wrapTransFunction(conn_, &checkSchemas);
    ASSERT_TRUE(pqres.has_value());
    ASSERT_EQ(pqres.value().size(), 0);

    manager_.fillSchemas();

    pqres = wrapTransFunction(conn_, &checkSchemas);
    ASSERT_TRUE(pqres.has_value());
    ASSERT_THROW(pqres.value().no_rows(), pqxx::unexpected_rows);

    manager_.disconnect();
    db_services::diconnect(conn_);
}

TEST_F(DbMangement_tests, test_create_drop_db) {
    std::string_view new_db_name = {DbMangement_tests::dbName + "_"};

    auto c_str_ = manager_.getCString();
    c_str_.setDbname(new_db_name);
    manager_.setCString(c_str_);

    auto a = wrap_non_trans_function(&checkDatabaseExistence, new_db_name);
    ASSERT_TRUE(a.has_value());
    ASSERT_EQ(a.value().size(), 0);//no db

    manager_.createDatabase(new_db_name);

    a = wrap_non_trans_function(&checkDatabaseExistence, new_db_name);
    ASSERT_TRUE(a.has_value());
    ASSERT_EQ(a.value().size(), 1);//new_db

    manager_.dropDatabase(new_db_name);

    a = wrap_non_trans_function(&checkDatabaseExistence, new_db_name);
    ASSERT_TRUE(a.has_value());
    ASSERT_EQ(a.value().size(), 0);//no db

    manager_.setCString(DbMangement_tests::c_str);
}
