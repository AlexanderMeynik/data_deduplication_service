
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
//todo remake into tests for dircetory data intgrity + file deletion checks
/*TEST_F(ServiceFileTest, process_nested_directory_file_id_set) {
    auto directory_path = get_normal_abs((fix_dir / "nested_directories/n1"));
    auto nested = directory_path / "n2";
    index_type dir_id;
    file_service_.process_directory(directory_path.string());
    file_service_.process_directory(nested.string());

    auto res = wrap_trans_function(conn_, get_files_for_directory, {directory_path.string()});
    EXPECT_TRUE(res.has_value());
    EXPECT_NO_THROW(res->one_row());

    res = wrap_trans_function(conn_, get_files_for_directory, {nested.string()});
    EXPECT_TRUE(res.has_value());
    EXPECT_NO_THROW(res->no_rows());
    std::vector<fs::path> files;
    std::vector<fs::path> files_nest;
    for (auto &entry: fs::recursive_directory_iterator(directory_path)) {
        if (!entry.is_directory()) {
            files.push_back(entry);
            if (entry.path().string().contains(nested.string())) {
                files_nest.push_back(entry);
            }
        }
    }
    res = wrap_trans_function(conn_, check_files_existence, files);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res->size(), files.size());
    for (auto row: res.value()) {
        EXPECT_EQ(row["dir_id"].as<index_type>(), dir_id);//todo remove
    }

    file_service_.delete_directory(directory_path.string());

    res = wrap_trans_function(conn_, get_files_for_directory, {directory_path.string()});//todo check files
    EXPECT_TRUE(res.has_value());
    EXPECT_NO_THROW(res->no_rows());


}

TEST_F(ServiceFileTest, process_nested_directory_up) {
    std::vector<fs::path> nest_arr;
    auto ns3 = get_normal_abs(fix_dir / "nested_directories/N_3");
    auto ns2 = ns3 / "N_2";
    auto ns1 = ns2 / "N_1";
    auto ns0 = ns1 / "N_0";
//todo check tfiles integrity(all are present, non  is rewrited
//todo add timestamp field array to file={create,filled, last time loaded}
    index_type n0, n1, n2, n3;
    file_service_.process_directory(ns0.string());
    file_service_.process_directory(ns1.string());
    file_service_.process_directory(ns2.string());
    file_service_.process_directory(ns3.string());

    auto res = wrap_trans_function(conn_, get_files_for_directory, {ns0.string()});
    EXPECT_TRUE(res.has_value());
    EXPECT_NO_THROW(res->no_rows());

    res = wrap_trans_function(conn_, get_files_for_directory, {ns1.string()});
    EXPECT_TRUE(res.has_value());
    EXPECT_NO_THROW(res->no_rows());

    res = wrap_trans_function(conn_, get_files_for_directory, {ns2.string()});
    EXPECT_TRUE(res.has_value());
    EXPECT_NO_THROW(res->no_rows());

    res = wrap_trans_function(conn_, get_files_for_directory, {ns3.string()});
    EXPECT_TRUE(res.has_value());
    EXPECT_NO_THROW(res->one_row());


    std::vector<fs::path> files;
    for (auto &entry: fs::recursive_directory_iterator(ns3)) {
        if (!entry.is_directory()) {
            files.push_back(entry);

        }
    }
    res = wrap_trans_function(conn_, check_files_existence, files);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(res->size(), files.size());
    for (auto row: res.value()) {
        EXPECT_EQ(row["dir_id"].as<index_type>(), n3);
    }

    file_service_.delete_directory(ns3.string());

    res = wrap_trans_function(conn_, get_files_for_directory, {ns3.string()});
    EXPECT_TRUE(res.has_value());
    EXPECT_NO_THROW(res->no_rows());
}*/


int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    google::SetVLOGLevel("*", 3);
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);


    return RUN_ALL_TESTS();
}