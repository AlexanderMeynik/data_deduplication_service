#include "testClasses.h"

#include <fstream>
#include <filesystem>

//todo compare
//https://stackoverflow.com/questions/1460703/comparison-of-arrays-in-google-test


static fs::path fix_dir="../fixture/fixture/";
static fs::path res_dir="../fixture/res/";

template<typename T,size_t size=64>
void compareArr(T arr[size],T arr2[size])
{
    for (int arr_elem = 0; arr_elem < size; ++arr_elem) {
        EXPECT_EQ(arr[arr_elem], arr2[arr_elem]);//can those be injected?
        SCOPED_TRACE(arr_elem);
    }
}
template<size_t size=64>
void compare_files(fs::path &&f1,fs::path&& f2)
{
    ASSERT_TRUE(exists(f1));
    ASSERT_TRUE(exists(f2));
    auto fs=file_size(f1);
    ASSERT_EQ(fs,file_size(f2));
    size_t seg_count= fs / size;
    //https://stackoverflow.com/questions/50491833/how-do-you-read-n-bytes-from-a-file-and-put-them-into-a-vectoruint8-t-using-it


    std::ifstream i1(f1),i2(f2);
    char a1[size];
    char a2[size];
    int j = 0;
    for (; j < seg_count; ++j) {

        i1.readsome(a1, size);//todo try to do it for readfile
        i2.readsome(a2, size);

        SCOPED_TRACE(j);
        compareArr(a1, a2);
    }

    i1.readsome(a1, size);
    i2.readsome(a2, size);
    SCOPED_TRACE(j);

    compareArr(a1, a2);


}


class DbFile_Dir_tests : public ::testing::TestWithParam<fs::path> {
public:
    static void SetUpTestSuite()
    {
        dbName="dedup_test_"+std::to_string(1);
        manager_=dbManager<64>();
        c_str=default_configuration();
        c_str.set_dbname(dbName);
        manager_.create_database(dbName);
        manager_.fill_schemas();
    }

    static void TearDownTestSuite()
    {
        manager_.drop_database(dbName);
    }
protected:
    inline static dbManager<64> manager_;
    inline static std::string dbName;
    inline static my_conn_string c_str;
};
TEST_F(DbFile_Dir_tests,test_file_eq)
{
    std::string filename="../../testDirectories/documentation/architecture.md";

    //std::string filename2="../../testDirectoriesRes/documentation/architecture.md";

    compare_files(filename,filename);
}

TEST_F(DbFile_Dir_tests,create_delete_file_test)
{
    std::string_view filename="sample_file_name";

    auto file_id =manager_.create_file(filename,index_vals::empty_parameter_value);
    auto result= wrap_trans_function(c_str,&check_file_existence, filename);

    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->one_row());
    //todo checkfile_id
    ASSERT_EQ(manager_.delete_file<delete_strategy::only_record>(filename),
              return_codes::return_sucess);

    result= wrap_trans_function(c_str,&check_file_existence, filename);
    ASSERT_TRUE(result.has_value());
    ASSERT_THROW(result->one_row(),pqxx::unexpected_rows);
}

TEST_F(DbFile_Dir_tests,create_delete_dir_test)
{
    std::string_view dirname="sample_dir_name";

    auto dir_id=manager_.create_directory(dirname);
    auto result= wrap_trans_function(c_str,&check_directory_existence, dirname);
    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->one_row());
    //todo check_dir_id

    ASSERT_EQ(manager_.delete_directory<delete_strategy::only_record>(dirname),
            return_codes::return_sucess);
    result= wrap_trans_function(c_str,&check_directory_existence, dirname);
    ASSERT_TRUE(result.has_value());
    ASSERT_THROW(result->one_row(),pqxx::unexpected_rows);

}


/*INSTANTIATE_TEST_SUITE_P(
        insert_segments,
        DbFile_Dir_tests,
        ::testing::Values(
                "block_size/0_5_block.txt",
                "block_size/1block.txt",
                "block_size/1_5_block.txt",
                "block_size/32blocks.txt"
        ));

TEST_P(DbFile_Dir_tests,insert_segments)
{
    auto f_path= GetParam();
    auto f_in=fix_dir/f_path;
    auto  f_out=res_dir/f_path;

    manager_.create_file(f_in.c_str(),index_vals::empty_parameter_value);
    std::ifstream in(f_in);

    manager_.insert_file_from_stream(f_in.c_str(),in);
    in.close();
    //todo check data from temp table
    manager_.delete_file<delete_strategy::only_record>(f_in.c_str());
    //todo no file data


}


INSTANTIATE_TEST_SUITE_P(
        insert_segments_process_retrieve,
        DbFile_Dir_tests,
        ::testing::Values(
                "block_size/0_5_block.txt",
                "block_size/1block.txt",
                "block_size/1_5_block.txt",
                "block_size/32blocks.txt"
        ));

TEST_P(DbFile_Dir_tests,insert_segments_process_retrieve)
{
    auto f_path= GetParam();
    auto f_in=fix_dir/f_path;
    auto  f_out=res_dir/f_path;

    auto file_id=manager_.create_file(f_in.c_str(),index_vals::empty_parameter_value);
    std::ifstream in(f_in);

    manager_.insert_file_from_stream(f_in.c_str(),in);
    in.close();

    manager_.finish_file_processing(f_in.c_str(),file_id);

    //todo asserrt_file_from db (std::ifstream in,std::string_view filename)

    manager_.delete_file<delete_strategy::only_record>(f_in.c_str());
    //todo no file data


}*/


TEST_F(DbFile_Dir_tests,load_file_d_test)
{

}



TEST_F(DbFile_Dir_tests,load_dir_test)
{

}
//todo negative tests
//todo create fixture to test
//todo test all options(delete/cascade...)