#include <fstream>
#include <filesystem>

#include "testClasses.h"

static fs::path fix_dir = "../test_data/fixture/";
static fs::path res_dir = "../test_data/res/";

class DbFile_Dir_tests : public ::testing::TestWithParam<fs::path> {
public:
    static void SetUpTestSuite() {
        dbName = "dedup_test_" + std::to_string(1);
        manager_ = dbManager();
        c_str = defaultConfiguration();
        c_str.setDbname(dbName);
        manager_.createDatabase(dbName);
        ASSERT_TRUE(manager_.checkConnection());
        manager_.fillSchemas();
        conn_ = connectIfPossible(c_str).value_or(nullptr);
    }

    static void TearDownTestSuite() {
        manager_.dropDatabase(dbName);
        diconnect(conn_);
    }

protected:
    inline static mType manager_;
    inline static std::string dbName;
    inline static myConnString c_str;
    inline static conPtr conn_;
    inline static size_t segmentSize=64;
};

TEST_F(DbFile_Dir_tests, test_file_eq) {
    fs::path filename = "../test_data/fixture/block_size/32blocks.txt";

    compare_files(filename, filename);
}

TEST_F(DbFile_Dir_tests, create_delete_file_test) {
    std::string_view filename = "sample_file_name";

    auto file_id = manager_.createFile(filename,1);
    auto result = wrapTransFunction(conn_, &db_services::checkFileExistence, std::string_view(filename));

    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->one_row());

    ASSERT_EQ(file_id, result.value()[0][0].as<indexType>());

    ASSERT_EQ(manager_.deleteFile(filename),
              returnCodes::ReturnSucess);

    result = wrapTransFunction(conn_, &db_services::checkFileExistence, std::string_view(filename));
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

    manager_.createFile(f_in.c_str(), fs::file_size(f_in), 0);
    std::ifstream in(f_in);

    manager_.insertFileFromStream(segmentSize, f_in.c_str(), in, fs::file_size(f_in));
    in.close();

    ASSERT_EQ(wrapTransFunction(conn_, &get_file_from_temp_table<64, check_from::temporary_table>, f_in),
              returnCodes::ReturnSucess);
    manager_.deleteFile(f_in.c_str());

    auto result = wrapTransFunction(conn_, &db_services::checkFileExistence, {f_in.string()});
    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->no_rows());
}


TEST_P(DbFile_Dir_tests, insert_segments_process_retrieve) {
    auto f_path = GetParam();
    auto f_in = getNormalAbs(fix_dir / f_path);
    auto f_out = getNormalAbs(res_dir / f_path);

    auto file_id = manager_.createFile(f_in.c_str(), fs::file_size(f_in), 0);
    std::ifstream in(f_in);

    manager_.insertFileFromStream(segmentSize, f_in.c_str(), in, fs::file_size(f_in));
    in.close();

    manager_.finishFileProcessing(f_in.c_str(), file_id);


    ASSERT_EQ(wrapTransFunction(conn_, &get_file_from_temp_table<64, check_from::concolidate_from_saved>, f_in),
              returnCodes::ReturnSucess);

    manager_.deleteFile(f_in.c_str());

    auto result = wrapTransFunction(conn_, &db_services::checkFileExistence, {f_in.string()});
    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->no_rows());
}

TEST_P(DbFile_Dir_tests, check_very_long_file_pathes) {
    auto d_path = "/very/very/long/directory/path/containing/more/than/57/symbols/or/more";
    fs::path f_path = GetParam();

    f_path = d_path / f_path;

    indexType file_id = manager_.createFile(f_path.c_str(),1);

    auto res = wrapTransFunction(conn_, &checkFileExistence, {f_path.c_str()});

    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(fromSpacedPath(res.value()[0][1].as<std::string>()), f_path);


    res = wrapTransFunction(conn_, &checkTExistence, {f_path.c_str()});


    ASSERT_TRUE(res.has_value());
    ASSERT_NO_THROW(res->one_row());


    manager_.deleteFile(f_path.c_str());

    res = wrapTransFunction(conn_, &checkTExistence, {f_path.c_str()});
    ASSERT_TRUE(res.has_value());
    ASSERT_NO_THROW(res->no_rows());
}


TEST_P(DbFile_Dir_tests, insert_segments_process_load) {
    auto f_path = GetParam();
    auto f_in =/*get_normal_abs*/(fix_dir / f_path);
    auto f_out =/*get_normal_abs*/(res_dir / f_path);

    auto file_id = manager_.createFile(f_in.c_str(), fs::file_size(f_in), 0);
    std::ifstream in(f_in);

    manager_.insertFileFromStream(segmentSize, f_in.c_str(), in, fs::file_size(f_in));
    in.close();

    manager_.finishFileProcessing(f_in.c_str(), file_id);

    std::ofstream out;
    create_hierarhy_for_file(f_out.c_str(), out);
    manager_.getFileStreamed(f_in.c_str(), out);
    out.close();

    compare_files(f_in, f_out);
    manager_.deleteFile(f_in.c_str());

    auto result = wrapTransFunction(conn_, &db_services::checkFileExistence, {f_in.string()});
    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->no_rows());
}



