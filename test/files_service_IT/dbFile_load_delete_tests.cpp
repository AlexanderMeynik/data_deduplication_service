#include "testClasses.h"

#include <fstream>
#include <filesystem>

//todo compare
//https://stackoverflow.com/questions/1460703/comparison-of-arrays-in-google-test



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


class DbFile_Dir_tests : public ::testing::Test {
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

    std::string filename2="../../testDirectoriesRes/documentation/architecture.md";

    compare_files(filename,filename);
}
TEST_F(DbFile_Dir_tests,create_file_test)
{

}
TEST_F(DbFile_Dir_tests,process_file_test)
{

}

TEST_F(DbFile_Dir_tests,load_file_test)
{

}

TEST_F(DbFile_Dir_tests,create_dir_test)
{

}

TEST_F(DbFile_Dir_tests,load_dir_test)
{

}
//todo negative tests
//todo create fixture to test
//todo test all options(delete/cascade...)