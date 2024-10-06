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
db_services::check_directory_existence(db_services::trasnactionType &txn, std::string_view dir_path) {
    std::string query = "select * from directories "
                        "where dir_path=\'%s\';";//todo to dotted path
    auto r_q = vformat(query.c_str(), dir_path.data());

    return txn.exec(r_q);
}

db_services::ResType db_services::check_file_existence(db_services::trasnactionType &txn, std::string_view file_name) {
    std::string query = "select files.* from files "
                        "where file_name=\'%s\';";//todo to dotted path
    auto r_q = vformat(query.c_str(), file_name.data());
    return txn.exec(r_q);
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
                        "WHERE files.file_name IN (%s)";//todo to dotted path
    std::stringstream ss;
    int i = 0;
    for (; i < files.size() - 1; i++) {
        ss << '\'' << files[i].string() << "\',";
    }
    ss << '\'' << files[i].string() << "\'";
    auto r_q = vformat(query.c_str(), ss.str().c_str());

    return txn.exec(r_q);
}


