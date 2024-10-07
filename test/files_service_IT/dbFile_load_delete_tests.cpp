#include "testClasses.h"

#include <fstream>
#include <filesystem>


static fs::path fix_dir = "../test_data/fixture/";
static fs::path res_dir = "../test_data/res/";

class DbFile_Dir_tests : public ::testing::TestWithParam<fs::path> {
public:
    static void SetUpTestSuite() {
        dbName = "dedup_test_" + std::to_string(1);
        manager_ = dbManager<64>();
        c_str = default_configuration();
        c_str.set_dbname(dbName);
        manager_.create_database(dbName);
        ASSERT_TRUE(manager_.check_connection());
        manager_.fill_schemas();
        conn_ = connect_if_possible(c_str).value_or(nullptr);
    }

    static void TearDownTestSuite() {
        manager_.drop_database(dbName);
        diconnect(conn_);
    }

protected:
    inline static dbManager<64> manager_;
    inline static std::string dbName;
    inline static my_conn_string c_str;
    inline static conPtr conn_;
};

TEST_F(DbFile_Dir_tests, test_file_eq) {
    fs::path filename = "../test_data/fixture/block_size/32blocks.txt";

    compare_files(filename, filename);
}

TEST_F(DbFile_Dir_tests, create_delete_file_test) {
    std::string_view filename = "sample_file_name";

    auto file_id = manager_.create_file(filename, index_vals::empty_parameter_value);
    auto result = wrap_trans_function(conn_, &db_services::check_file_existence, std::string_view(filename));

    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->one_row());

    ASSERT_EQ(file_id, result.value()[0][0].as<index_type>());

    ASSERT_EQ(manager_.delete_file(filename),
              return_codes::return_sucess);

    result = wrap_trans_function(conn_, &db_services::check_file_existence, std::string_view(filename));
    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->no_rows());
}




INSTANTIATE_TEST_SUITE_P(
        file_segmented_tests,
        DbFile_Dir_tests,
        ::testing::Values(
                "block_size/0_5_block.txt",
                "block_size/1block.txt",
                "block_size/1_5_block.txt",
                "block_size/32blocks.txt"
        ));

TEST_P(DbFile_Dir_tests, insert_segments) {
    auto f_path = GetParam();
    auto f_in =/*get_normal_abs*/(fix_dir / f_path);

    manager_.create_file(f_in.c_str(), fs::file_size(f_in));
    std::ifstream in(f_in);

    manager_.insert_file_from_stream(f_in.c_str(), in, fs::file_size(f_in));
    in.close();

    ASSERT_EQ(wrap_trans_function(conn_, &get_file_from_temp_table<64, check_from::temporary_table>, f_in),
              return_codes::return_sucess);
    manager_.delete_file(f_in.c_str());

    auto result = wrap_trans_function(conn_, &db_services::check_file_existence, {f_in.string()});
    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->no_rows());


}


TEST_P(DbFile_Dir_tests, insert_segments_process_retrieve) {
    auto f_path = GetParam();
    auto f_in = get_normal_abs(fix_dir / f_path);
    auto f_out = get_normal_abs(res_dir / f_path);

    auto file_id = manager_.create_file(f_in.c_str(), fs::file_size(f_in));
    std::ifstream in(f_in);

    manager_.insert_file_from_stream(f_in.c_str(), in, fs::file_size(f_in));
    in.close();

    manager_.finish_file_processing(f_in.c_str(), file_id);


    ASSERT_EQ(wrap_trans_function(conn_, &get_file_from_temp_table<64, check_from::concolidate_from_saved>, f_in),
              return_codes::return_sucess);

    manager_.delete_file(f_in.c_str());

    auto result = wrap_trans_function(conn_, &db_services::check_file_existence, {f_in.string()});
    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->no_rows());
}

TEST_P(DbFile_Dir_tests, check_very_long_file_pathes) {
    auto d_path = "/very/very/long/directory/path/containing/more/than/57/symbols/or/more";
    fs::path f_path = GetParam();

    f_path = d_path / f_path;

    index_type file_id = manager_.create_file(f_path.c_str(), index_vals::empty_parameter_value);

    auto res = wrap_trans_function(conn_, &check_file_existence, {f_path.c_str()});

    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(from_spaced_path(res.value()[0][1].as<std::string>()), f_path);


    res = wrap_trans_function(conn_, &check_t_existence, {f_path.c_str()});


    ASSERT_TRUE(res.has_value());
    ASSERT_NO_THROW(res->one_row());


    manager_.delete_file(f_path.c_str());

    res = wrap_trans_function(conn_, &check_t_existence, {f_path.c_str()});
    ASSERT_TRUE(res.has_value());
    ASSERT_NO_THROW(res->no_rows());

}


TEST_P(DbFile_Dir_tests, insert_segments_process_load) {
    auto f_path = GetParam();
    auto f_in =/*get_normal_abs*/(fix_dir / f_path);
    auto f_out =/*get_normal_abs*/(res_dir / f_path);

    auto file_id = manager_.create_file(f_in.c_str(), fs::file_size(f_in));
    std::ifstream in(f_in);

    manager_.insert_file_from_stream(f_in.c_str(), in, fs::file_size(f_in));
    in.close();

    manager_.finish_file_processing(f_in.c_str(), file_id);

    std::ofstream out;
    create_hierarhy_for_file(f_out.c_str(), out);
    manager_.get_file_streamed(f_in.c_str(), out);
    out.close();

    compare_files(f_in, f_out);
    manager_.delete_file(f_in.c_str());

    auto result = wrap_trans_function(conn_, &db_services::check_file_existence, {f_in.string()});
    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->no_rows());
}




//todo negative tests
