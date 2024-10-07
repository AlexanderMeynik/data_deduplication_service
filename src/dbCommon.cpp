#include "dbCommon.h"

tl::expected<db_services::conPtr, return_codes> db_services::connect_if_possible(std::string_view cString) {
    conPtr c;
    std::string css = cString.data();
    try {
        c = std::make_shared<pqxx::connection>(css);
        if (!c->is_open()) {
            VLOG(1) << vformat("Unable to connect by url \"%s\"\n", cString.data());
            return tl::unexpected(return_codes::error_occured);

        } else {

            VLOG(2) << "Opened database successfully: " << c->dbname() << '\n';
        }
    } catch (const pqxx::sql_error &e) {
        VLOG(1) << "SQL Error: " << e.what()
                << "Query: " << e.query()
                << "SQL State: " << e.sqlstate() << '\n';
        return tl::unexpected(return_codes::error_occured);
    } catch (const std::exception &e) {
        VLOG(1) << "Error: " << e.what() << '\n';
        return tl::unexpected(return_codes::error_occured);
    }
    return tl::expected<db_services::conPtr, return_codes>{c};
}


db_services::ResType
db_services::terminate_all_db_connections(db_services::nonTransType &no_trans_exec, std::string_view db_name) {
    std::string qq = vformat("SELECT pg_terminate_backend(pg_stat_activity.pid)\n"
                             "FROM pg_stat_activity "
                             "WHERE pg_stat_activity.datname = \'%s\' "
                             " AND pid <> pg_backend_pid();", db_name.data());
    ResType r = no_trans_exec.exec(qq);

    VLOG(2) << vformat("All connections to %s were terminated!", db_name.data());
    return r;
}

db_services::ResType
db_services::check_database_existence(db_services::nonTransType &no_trans_exec, std::string_view db_name) {
    std::string qq = vformat("SELECT 1 FROM pg_database WHERE datname = \'%s\';", db_name.data());
    return no_trans_exec.exec(qq);
}

db_services::ResType db_services::check_schemas(db_services::trasnactionType &txn) {
    return txn.exec("select tablename "
                    "from pg_tables "
                    "where schemaname = 'public';");
}

db_services::ResType
db_services::get_files_for_directory(db_services::trasnactionType &txn, std::string_view dir_path) {

    std::string query = "select * from files "
                        "where to_tsvector('simple',replace(file_name,'_', '/'))"
                        "@@ \'%s\' and file_name LIKE \'%s %%\'";
    auto r_q = vformat(query.c_str(), to_tsquerable_path(dir_path).c_str(),
                       to_spaced_path(dir_path).c_str());

    return txn.exec(r_q);
}

std::vector<db_services::index_type> db_services::get_file_id_vector(db_services::trasnactionType &txn, std::string_view dir_path) {
    std::vector<index_type> res;

    ResType rr= get_files_for_directory(txn,dir_path);

    for (const auto &row:rr) {
        res.push_back(row[0].as<index_type>());
    }
    return res;
}

db_services::ResType db_services::check_file_existence(db_services::trasnactionType &txn, std::string_view file_name) {
    std::string query = "select files.* from files "
                        "where file_name=\'%s\';";
    auto r_q = vformat(query.c_str(), to_spaced_path(file_name).c_str());
    return txn.exec(r_q);
}

db_services::index_type db_services::get_file_id(db_services::trasnactionType &txn, std::string_view file_name) {
    return check_file_existence(txn,file_name).one_row()[0].as<index_type>();
}

bool db_services::does_file_exist(db_services::trasnactionType &txn, std::string_view file_name) {
    index_type res;
    try {
        res= get_file_id(txn,file_name);
        return true;
    }
    catch (pqxx::unexpected_rows& rr)
    {
        VLOG(2)<<vformat("File %s does not exist!",file_name.data());
    }
    return false;
}


db_services::my_conn_string db_services::load_configuration(std::string_view filename) {
    std::ifstream conf(filename.data());
    if (!conf.is_open()) {
        int count = 0;
        for (auto &entry: std::filesystem::recursive_directory_iterator(
                std::filesystem::current_path().parent_path())) {
            count++;
            VLOG(1) << entry.path();
            if (count > 20) {
                break;
            }
        }
    }
    std::string dbname1, user, password;
    conf >> dbname1 >> user >> password;
    std::string host;
    unsigned port1 = 5432;
    if (!conf.eof())
        conf >> host >> port1;

    auto res = db_services::my_conn_string(user, password, host, dbname1, port1);
    VLOG(2) << vformat("Configuration was loaded from file %s", filename.data());

    return res;
}

db_services::ResType
db_services::check_files_existence(db_services::trasnactionType &txn, std::vector<std::filesystem::path> &files) {
    std::string query = "SELECT * "
                        "FROM files "
                        "WHERE files.file_name IN (%s)";
    std::stringstream ss;
    int i = 0;
    for (; i < files.size() - 1; i++) {
        ss << '\'' << to_spaced_path(files[i].string()) << "\',";
    }
    ss << '\'' << to_spaced_path(files[i].string()) << "\'";
    auto r_q = vformat(query.c_str(), ss.str().c_str());

    return txn.exec(r_q);
}

std::string db_services::to_tsquerable_path(std::string_view path)
{
    std::string res(path.size(),'\0');
    std::transform(path.begin(), path.end(), res.begin(),
                   [](char c){
                       switch (c) {
                           case ' ':
                           case '/':
                               return '&';
                           case '_':
                               return '/';
                           default:
                               return (char)std::tolower(c);
                       } });
    if(res[0]=='&')
        res= res.substr(1);

    //todo test to spaced path + this=this
    return res;
}

std::string db_services::from_spaced_path(std::string_view path)
{
    std::string res(path.size()+1,'/');
    std::replace_copy_if(path.begin(), path.end(),res.begin()+1,
                         [](auto n){ return n==' '; }, '/');
    return res;
}

std::string db_services::to_spaced_path(std::string_view path)
{
    std::string res(path.size(),'\0');
    std::replace_copy_if(path.begin(), path.end(),res.begin(),
                         [](auto n){ return n=='/'; }, ' ');
    return res.substr(1);
}

db_services::ResType db_services::delete_unused_segments(db_services::trasnactionType &txn) {
    return txn.exec("delete from public.segments where segment_count=0");
}

db_services::index_type db_services::check_segment_count(db_services::trasnactionType &txn) {
    std::string qq="select segment_hash,count(segment_hash) from data "
                   "group by  segment_hash except "
                   "select segment_hash,segment_count "
                   "from public.segments; ";
    ResType rr=txn.exec(qq);
    if(rr.empty())
    {
        return return_sucess;
    }
    return error_occured;

}









