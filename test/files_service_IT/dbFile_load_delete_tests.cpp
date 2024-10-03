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
template<size_t size=64,check_from cs>
void get_file_from_temp_table(trasnactionType &txn,fs::path & original_file,fs::path &  file_path)
{
    char buff[size];
    std::ifstream in(original_file.c_str());
    auto res=db_services::check_file_existence(txn,original_file.c_str());

    auto file_size=res.one_row()["size_in_bytes"].as<index_type>();

    ASSERT_EQ(fs::file_size(original_file),file_size);

    auto hash_str=get_table_name(txn,original_file.c_str());
    std::string table_name=vformat("temp_file_%s",hash_str.c_str());



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



static fs::path fix_dir="../test_data/fixture/";
static fs::path res_dir="../test_data/res/";


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

        i1.readsome(a1, size);
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
        ASSERT_TRUE(manager_.check_connection());
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

TEST_P(DbFile_Dir_tests,insert_segments)
{
    auto f_path= GetParam();
    auto f_in=/*get_normal_abs*/(fix_dir/f_path);
    auto  f_out=/*get_normal_abs*/(res_dir/f_path);

    manager_.create_file(f_in.c_str(),index_vals::empty_parameter_value,fs::file_size(f_in));
    std::ifstream in(f_in);

    manager_.insert_file_from_stream(f_in.c_str(),in,fs::file_size(f_in));
    in.close();

    ASSERT_EQ(wrap_trans_function(conn_,&get_file_from_temp_table<64,check_from::temporary_table>,f_in,f_out),return_codes::return_sucess);
    manager_.delete_file<delete_strategy::only_record>(f_in.c_str());

    auto result= wrap_trans_function(conn_,&db_services::check_file_existence,{f_in.string()});
    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->no_rows());



}



TEST_P(DbFile_Dir_tests,insert_segments_process_retrieve)
{
    auto f_path= GetParam();
    auto f_in=get_normal_abs(fix_dir/f_path);
    auto  f_out=get_normal_abs(res_dir/f_path);

    auto file_id=manager_.create_file(f_in.c_str(),index_vals::empty_parameter_value,fs::file_size(f_in));
    std::ifstream in(f_in);

    manager_.insert_file_from_stream(f_in.c_str(),in,fs::file_size(f_in));
    in.close();

    manager_.finish_file_processing(f_in.c_str(),file_id);


    ASSERT_EQ(wrap_trans_function(conn_,&get_file_from_temp_table<64,check_from::concolidate_from_saved>,f_in,f_out),return_codes::return_sucess);

    manager_.delete_file(f_in.c_str());

    auto result= wrap_trans_function(conn_,&db_services::check_file_existence,{f_in.string()});
    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->no_rows());
}
template<size_t N>
std::string trimNsymbols(std::string&&trimmee)
{
    if(trimmee.size()<=2*N) {
        VLOG(1) << vformat("Trim Value %d is to large for string %s!", N, trimmee.c_str());
        return "";
    }
    return trimmee.substr(N,trimmee.length()-(1+N));
}
template<>
std::string trimNsymbols<0>(std::string&&trimmee)
{
    return trimmee;
}
TEST_P(DbFile_Dir_tests,check_very_long_file_pathes)
{
    auto d_path= "very/very/long/directory/path/containing/more/than/57/symbols/or/more";
    trimNsymbols<0>(d_path);

    fs::path f_path=GetParam();

    f_path=d_path/f_path;

    auto file_id=manager_.create_file(f_path.c_str(),index_vals::empty_parameter_value,index_vals::empty_parameter_value);

    auto res=wrap_trans_function(conn_,&check_file_existence,{f_path.c_str()});

    ASSERT_TRUE(res.has_value());
    ASSERT_EQ(trimNsymbols<1>(res.value()[0][1].as<std::string>()),f_path);



    res=wrap_trans_function(conn_,&check_t_existence,{f_path.c_str()});



    ASSERT_TRUE(res.has_value());
    ASSERT_NO_THROW(res->one_row());




    manager_.delete_file(f_path.c_str());

    res=wrap_trans_function(conn_,&check_t_existence,{f_path.c_str()});
    ASSERT_TRUE(res.has_value());
    ASSERT_NO_THROW(res->no_rows());

}






TEST_P(DbFile_Dir_tests,insert_segments_process_load)
{
    auto f_path= GetParam();
    auto f_in=/*get_normal_abs*/(fix_dir/f_path);
    auto  f_out=/*get_normal_abs*/(res_dir/f_path);

    auto file_id=manager_.create_file(f_in.c_str(),index_vals::empty_parameter_value,fs::file_size(f_in));
    std::ifstream in(f_in);

    manager_.insert_file_from_stream(f_in.c_str(),in,fs::file_size(f_in));
    in.close();

    manager_.finish_file_processing(f_in.c_str(),file_id);

    std::ofstream out(f_out);
    manager_.get_file_streamed(f_in.c_str(),out);
    out.close();

    compare_files(f_in,f_out);
    manager_.delete_file(f_in.c_str());

    auto result= wrap_trans_function(conn_,&db_services::check_file_existence,{f_in.string()});
    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->no_rows());
}


TEST_F(DbFile_Dir_tests,insert_files_from_directory)
{
    auto d_path="block_size";
    auto dd_path=fix_dir/d_path;
    auto f_path= "block_size/32blocks.txt";
    std::vector< fs::path> files= {"block_size/0_5_block.txt",
                                   "block_size/1block.txt",
                                   "block_size/1_5_block.txt",
                                   "block_size/32blocks.txt"};

    auto dir_id=manager_.create_directory(dd_path.string());
    for (auto&entry:files) {

        auto f_in=/*get_normal_abs*/(fix_dir/entry);

        auto file_id=manager_.create_file(f_in.c_str(),dir_id,fs::file_size(f_in));
        std::ifstream in(f_in);

        manager_.insert_file_from_stream(f_in.c_str(),in,fs::file_size(f_in));
        in.close();

        manager_.finish_file_processing(f_in.string(),file_id);



    }

    auto dirs=manager_.get_all_files(dd_path.c_str());

    for (int j = 0; j < dirs.size(); ++j) {
        EXPECT_EQ(dirs[j].second,/*get_normal_abs*/((fix_dir/files[j])));
        SCOPED_TRACE(j);
    }


    auto ress=manager_.delete_directory<delete_strategy::only_record>(dd_path.string());//todo split
    ASSERT_EQ(ress,warning_message);

    auto result=wrap_trans_function(conn_,&db_services::check_directory_existence,{dd_path.string()});

    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->one_row());
    ASSERT_NE(manager_.get_all_files(dd_path.c_str()).size(),0);

    manager_.delete_directory(dd_path.string());

    result=wrap_trans_function(conn_,&db_services::check_directory_existence,{dd_path.string()});
    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->no_rows());
}


TEST_F(DbFile_Dir_tests,insert_segments_process_delete_err)
{
    auto f_path= "block_size/32blocks.txt";//todo fix
    auto f_in=(fix_dir/f_path);

    auto file_id=manager_.create_file((f_in).c_str(),index_vals::empty_parameter_value,fs::file_size(f_in));
    //todo new table name
    //manager_.create_file(f_in.c_str(),index_vals::empty_parameter_value,fs::file_size(f_in))
    std::ifstream in(f_in);


    manager_.insert_file_from_stream(f_in.string(),in,fs::file_size(f_in));
    in.close();

    manager_.finish_file_processing(f_in.c_str(),file_id);

    auto ress=manager_.delete_file<delete_strategy::only_record>(f_in.c_str());
    ASSERT_EQ(ress,warning_message);

    auto result= wrap_trans_function(conn_,&db_services::check_file_existence,{f_in.string()});
    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->one_row());

    manager_.delete_file(f_in.c_str());

    result=wrap_trans_function(conn_,&db_services::check_file_existence,{f_in.string()});
    ASSERT_TRUE(result.has_value());
    ASSERT_NO_THROW(result->no_rows());
}


//todo negative tests
