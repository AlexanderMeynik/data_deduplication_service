#include "testClasses.h"

#include <fstream>
#include <filesystem>

template<typename T,typename A>
requires std::is_same_v<T,A>||
        std::is_same_v<T,typename std::remove_const<A>::type>||
        std::is_same_v<A,typename std::remove_const<T>::type>
void compareArr(size_t size,T *arr,A* arr2)
{
    for (int arr_elem = 0; arr_elem < size; ++arr_elem) {
        EXPECT_EQ(arr[arr_elem], arr2[arr_elem]);
        SCOPED_TRACE(arr_elem);
    }
}
enum check_from
{
    temporary_table,
    concolidate_from_saved
};
template<size_t size=64,check_from cs=temporary_table>
void get_file_from_temp_table(trasnactionType &txn,fs::path & original_file,fs::path &  file_path)
{
    char buff[size];
    std::ifstream in(original_file.c_str());
    auto res=db_services::check_file_existence(txn,original_file.c_str());

    auto file_size=res.one_row()["size_in_bytes"].as<index_type>();

    ASSERT_EQ(fs::file_size(original_file),file_size);

    std::string table_name = vformat("temp_file_%s", file_path.c_str());

    int index_num=0;
    auto block_count=file_size/size;
    std::string query;
    if constexpr (cs==temporary_table) {
        query = vformat("select t.data from \"%s\" t "
                        "ORDER by t.pos", table_name.c_str());
    }
    else
    {
        query= vformat("select s.segment_data "
                                    "from public.data"
                                    "         inner join public.segments s on s.segment_hash = public.data.segment_hash "
                                    "        inner join public.files f on f.file_id = public.data.file_id "
                                    "where file_name=\'%s\'::tsvector "
                                    "order by segment_num", file_path.c_str());
    }
    for (auto [name]: txn.stream<pqxx::binarystring>(query)) {
        //out<<name;
        auto size_1=in.readsome(buff, size);
        if(index_num<block_count) {
            compareArr(size,buff,name.str().c_str());
        }
        else
        {
            compareArr(size_1,buff,name.str().c_str());
        }
        SCOPED_TRACE(index_num);
        index_num++;
    }
}


template<size_t size=64>
void get_file_from_temp_table2(trasnactionType &txn,fs::path & original_file,fs::path &  file_path)
{

    char buff[size];
    std::ifstream in(original_file.c_str());
    auto res=db_services::check_file_existence(txn,file_path.c_str());

    auto file_size=res.one_row()["size_in_bytes"].as<index_type>();

    ASSERT_EQ(fs::file_size(original_file),file_size);


    int index_num=0;
    auto block_count=file_size/size;

    std::string query = vformat("select s.segment_data "
                                "from public.data"
                                "         inner join public.segments s on s.segment_hash = public.data.segment_hash "
                                "        inner join public.files f on f.file_id = public.data.file_id "
                                "where file_name=\'%s\'::tsvector "
                                "order by segment_num", file_path.c_str());

    for (auto [name]: txn.stream<pqxx::binarystring>(query)) {
        //out<<name;
        auto size_1=in.readsome(buff, size);
        if(index_num<block_count) {
            compareArr(size,buff,name.str().c_str());
        }
        else
        {
            in.readsome(buff, size);
            compareArr(size_1,buff,name.str().c_str());
        }
        SCOPED_TRACE(index_num);
        index_num++;
    }
}

static fs::path fix_dir="../fixture/fixture/";
static fs::path res_dir="../fixture/res/";


template<size_t size=64>
void compare_files(fs::path &f1,fs::path& f2)
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
        compareArr(size,a1, a2);
    }

    auto sz=i1.readsome(a1, size);
    i2.readsome(a2, size);
    SCOPED_TRACE(j);
    compareArr(sz,a1, a2);



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
        conn_= connect_if_possible(c_str).value_or(nullptr);
    }

    static void TearDownTestSuite()
    {
        manager_.drop_database(dbName);
        diconnect(conn_);
    }
protected:
    inline static dbManager<64> manager_;
    inline static std::string dbName;
    inline static my_conn_string c_str;
    inline static conPtr conn_;
};
TEST_F(DbFile_Dir_tests,test_file_eq)
{
    fs::path filename="../../testDirectories/documentation/architecture.md";

    //std::string filename2="../../testDirectoriesRes/documentation/architecture.md";

    compare_files(filename,filename);
}

TEST_F(DbFile_Dir_tests,create_delete_file_test)
{
    std::string_view filename="sample_file_name";

    auto file_id =manager_.create_file(filename,index_vals::empty_parameter_value);
    auto result= wrap_trans_function(conn_,&db_services::check_file_existence, filename);

    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->one_row());

    ASSERT_EQ(file_id,result.value()[0][0].as<index_type>());

    ASSERT_EQ(manager_.delete_file<delete_strategy::only_record>(filename),
              return_codes::return_sucess);

    result= wrap_trans_function(conn_,&db_services::check_file_existence, filename);
    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->no_rows());
}

TEST_F(DbFile_Dir_tests,create_delete_dir_test)
{
    std::string_view dirname="sample_dir_name";

    auto dir_id=manager_.create_directory(dirname);
    auto result= wrap_trans_function(conn_,&db_services::check_directory_existence, dirname);

    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->one_row());

    ASSERT_EQ(dir_id,result.value()[0][0].as<index_type>());

    ASSERT_EQ(manager_.delete_directory<delete_strategy::only_record>(dirname),
            return_codes::return_sucess);
    result= wrap_trans_function(conn_,&db_services::check_directory_existence, dirname);
    ASSERT_TRUE(result.has_value());
    ASSERT_THROW(result->one_row(),pqxx::unexpected_rows);

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

TEST_P(DbFile_Dir_tests,insert_segments)
{
    auto f_path= GetParam();
    auto f_in=get_normal_abs(fix_dir/f_path);
    auto  f_out=get_normal_abs(res_dir/f_path);

    manager_.create_file(f_in.c_str(),index_vals::empty_parameter_value,fs::file_size(f_in));
    std::ifstream in(f_in);

    manager_.insert_file_from_stream(f_in.c_str(),in);
    in.close();
    wrap_trans_function(conn_,&get_file_from_temp_table,f_in,f_out);
    manager_.delete_file<delete_strategy::only_record>(f_in.c_str());

    auto result= wrap_trans_function(conn_,&db_services::check_file_existence,{f_in.string()});
    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->no_rows());



}


/*INSTANTIATE_TEST_SUITE_P(
        insert_segments_process_retrieve,
        DbFile_Dir_tests,
        ::testing::Values(
                "block_size/0_5_block.txt",
                "block_size/1block.txt",
                "block_size/1_5_block.txt",
                "block_size/32blocks.txt"
        ));*/

TEST_P(DbFile_Dir_tests,insert_segments_process_retrieve)
{
    auto f_path= GetParam();
    auto f_in=get_normal_abs(fix_dir/f_path);
    auto  f_out=get_normal_abs(res_dir/f_path);

    auto file_id=manager_.create_file(f_in.c_str(),index_vals::empty_parameter_value,fs::file_size(f_in));
    std::ifstream in(f_in);

    manager_.insert_file_from_stream(f_in.c_str(),in);
    in.close();

    manager_.finish_file_processing(f_in.c_str(),file_id);

    wrap_trans_function(conn_,&get_file_from_temp_table<check_from::concolidate_from_saved>,f_in,f_out);

    //manager_.delete_file<delete_strategy::only_record>(f_in.c_str());//todo variant when delete fails
    manager_.delete_file(f_in.c_str());

    auto result= wrap_trans_function(conn_,&db_services::check_file_existence,{f_in.string()});
    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->no_rows());
}


TEST_P(DbFile_Dir_tests,insert_segments_process_load)
{
    auto f_path= GetParam();
    auto f_in=get_normal_abs(fix_dir/f_path);
    auto  f_out=get_normal_abs(res_dir/f_path);

    auto file_id=manager_.create_file(f_in.c_str(),index_vals::empty_parameter_value,fs::file_size(f_in));
    std::ifstream in(f_in);

    manager_.insert_file_from_stream(f_in.c_str(),in);
    in.close();

    manager_.finish_file_processing(f_in.c_str(),file_id);

    std::ofstream out(f_out);
    manager_.get_file_streamed(f_in.c_str(),out);//todo variant with record deletion
    out.close();

    compare_files(f_in,f_out);
    manager_.delete_file(f_in.c_str());

    auto result= wrap_trans_function(conn_,&db_services::check_file_existence,{f_in.string()});
    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->no_rows());
}





//todo negative tests
//todo create fixture to test
//todo test all options(delete/cascade...)