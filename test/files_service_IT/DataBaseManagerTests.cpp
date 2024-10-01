#include "testClasses.h"




class DbMangement_tests : public ::testing::Test {
public:

    static void SetUpTestSuite()
    {
        dbName="dedup_test_"+std::to_string(0);
        manager_=dbManager<64>();
        c_str=default_configuration();
        c_str.set_dbname(dbName);
        manager_.create_database(dbName);

    }
    static auto get_c_str()
    {
        return c_str;
    }
    static void TearDownTestSuite()
    {
        manager_.drop_database(dbName);
    }
protected:
    inline static m_type manager_;
    inline static std::string dbName;
    inline static my_conn_string c_str;



};




TEST_F(DbMangement_tests, test_db_acess)
{
   auto a= wrap_non_trans_function( &check_database_existence, {dbName});
   ASSERT_TRUE(a.has_value());
   ASSERT_EQ(a.value().size(),1);
}

TEST_F(DbMangement_tests, testDb_disconnect)
{
    manager_.disconnect();
    ASSERT_FALSE(manager_.check_connection());
}

TEST_F(DbMangement_tests, test_schema_creation)
{
    auto res=manager_.connectToDb();
    ASSERT_EQ(res, return_codes::return_sucess);

    auto pqres= wrap_trans_function(c_str,&check_schemas);
    ASSERT_TRUE(pqres.has_value());
    ASSERT_EQ(pqres.value().size(), 0);

    manager_.fill_schemas();

    pqres = wrap_trans_function(c_str,&check_schemas);
    ASSERT_TRUE(pqres.has_value());
    ASSERT_THROW(pqres.value().no_rows(),pqxx::unexpected_rows);

    manager_.disconnect();

}

TEST_F(DbMangement_tests, test_create_drop_db)
{
    std::string_view new_db_name={DbMangement_tests::dbName+"_"};

    auto c_str_=manager_.getCString();
    c_str_.set_dbname(new_db_name);
    manager_.setCString(c_str_);

    auto a= wrap_non_trans_function( &check_database_existence, new_db_name);
    ASSERT_TRUE(a.has_value());
    ASSERT_EQ(a.value().size(),0);//no db

    manager_.create_database(new_db_name);

    a= wrap_non_trans_function( &check_database_existence, new_db_name);
    ASSERT_TRUE(a.has_value());
    ASSERT_EQ(a.value().size(),1);//new_db

    manager_.drop_database(new_db_name);

    a= wrap_non_trans_function( &check_database_existence, new_db_name);
    ASSERT_TRUE(a.has_value());
    ASSERT_EQ(a.value().size(),0);//no db

    manager_.setCString(DbMangement_tests::c_str);

}
/*TEST_F(DbMangement_tests, test_kill_all_conenction)
{

    manager_.connectToDb();
    ASSERT_TRUE(manager_.checkConnection());

    int connectionPoolSize=10;
    std::vector<conPtr> connections;
    for (int i = 0; i < connectionPoolSize; ++i) {
        SLEEP(12ms);
        connections.push_back(connect_if_possible(c_str).value_or(nullptr));

    }
    ASSERT_TRUE(std::all_of(connections.begin(), connections.end(), [&](const auto &item) {
        return db_services::checkConnection(item);
    }));



    auto res= wrap_non_trans_method<64, &dbManager<64>::terminateAllDbConnections>(DbMangement_tests::manager_);
    for (int i = 0; i < connectionPoolSize; ++i) {
        auto reygs=connections[i]&&connections[i]->is_open();
        std::cout<<reygs<<'\n';
    }

    *//*ASSERT_TRUE(std::all_of(connections.begin(), connections.end(), [&](const auto &item) {
        return !db_services::checkConnection(item);
    }));*//*

    ASSERT_FALSE(manager_.checkConnection());
    manager_.disconnect();


}*/

