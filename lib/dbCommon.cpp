#include "dbCommon.h"

#include <algorithm>
#include <string>
#include <memory>
#include <vector>

namespace db_services {

    tl::expected<conPtr, returnCodes> connectIfPossible(std::string_view cString) {
        conPtr c;
        std::string css = cString.data();
        try {
            c = std::make_shared<pqxx::connection>(css);
            if (!c->is_open()) {
                VLOG(1) << vformat("Unable to connect by url \"%s\"\n", cString.data());
                return tl::unexpected(returnCodes::ErrorOccured);

            } else {

                VLOG(2) << "Opened database successfully: " << c->dbname() << '\n';
            }
        } catch (const pqxx::sql_error &e) {
            VLOG(1) << "SQL Error: " << e.what()
                    << "Query: " << e.query()
                    << "SQL State: " << e.sqlstate() << '\n';
            return tl::unexpected(returnCodes::ErrorOccured);
        } catch (const std::exception &e) {
            VLOG(1) << "Error: " << e.what() << '\n';
            return tl::unexpected(returnCodes::ErrorOccured);
        }
        return tl::expected<conPtr, returnCodes>{c};
    }


    resType
    terminateAllDbConnections(nonTransType &noTransExec, std::string_view dbName) {
        std::string qq = vformat("SELECT pg_terminate_backend(pg_stat_activity.pid)\n"
                                 "FROM pg_stat_activity "
                                 "WHERE pg_stat_activity.datname = \'%s\' "
                                 " AND pid <> pg_backend_pid();", dbName.data());
        resType r = noTransExec.exec(qq);

        VLOG(2) << vformat("All connections to %s were terminated!", dbName.data());
        return r;
    }

    resType
    checkDatabaseExistence(nonTransType &noTransExec, std::string_view dbName) {
        std::string qq = vformat("SELECT 1 FROM pg_database WHERE datname = \'%s\';", dbName.data());
        return noTransExec.exec(qq);
    }

    resType checkSchemas(trasnactionType &txn) {
        return txn.exec("select tablename "
                        "from pg_tables "
                        "where schemaname = 'public';");
    }

    resType
    getFilesForDirectory(trasnactionType &txn, std::string_view dirPath) {
        std::string query = "select * from files "
                            "where to_tsvector('simple',replace(file_name,'_', '/'))"
                            "@@ \'%s\' and file_name LIKE \'%s %%\'";
        auto formattedQuery = vformat(query.c_str(), toTsquerablePath(dirPath).c_str(),
                                      toSpacedPath(dirPath).c_str());

        return txn.exec(formattedQuery);
    }

    std::vector<indexType>
    getFileIdVector(trasnactionType &txn, std::string_view dirPath) {
        std::vector<indexType> res;

        resType rr = getFilesForDirectory(txn, dirPath);

        for (const auto &row: rr)
            res.push_back(row[0].as<indexType>());
        return res;
    }

    resType checkFileExistence(trasnactionType &txn, std::string_view fileName) {
        std::string query = "select files.* from files "
                            "where file_name=\'%s\';";
        auto formattedQuery = vformat(query.c_str(), toSpacedPath(fileName).c_str());
        return txn.exec(formattedQuery);
    }

    indexType getFileId(trasnactionType &txn, std::string_view fileName) {
        return checkFileExistence(txn, fileName).one_row()[0].as<indexType>();
    }

    bool doesFileExist(trasnactionType &txn, std::string_view fileName) {
        try {
            getFileId(txn, fileName);
            return true;
        }
        catch (pqxx::unexpected_rows &rr) {
            VLOG(2) << vformat("File %s does not exist!", fileName.data());
        }
        return false;
    }


    myConnString loadConfiguration(std::string_view filename) {
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

        auto res = myConnString(user, password, host, dbname1, port1);
        VLOG(2) << vformat("Configuration was loaded from file %s", filename.data());

        return res;
    }

    resType
    checkFilesExistence(trasnactionType &txn, const std::vector<std::filesystem::path> &files) {
        std::string query = "SELECT * "
                            "FROM files "
                            "WHERE files.file_name IN (%s)";
        std::stringstream ss;
        int i = 0;
        for (; i < files.size() - 1; i++) {
            ss << '\'' << toSpacedPath(files[i].string()) << "\',";
        }
        ss << '\'' << toSpacedPath(files[i].string()) << "\'";
        auto formattedQuery = vformat(query.c_str(), ss.str().c_str());

        return txn.exec(formattedQuery);
    }

    std::string toTsquerablePath(std::string_view path) {
        std::string res(path.size(), '\0');
        std::transform(path.begin(), path.end(), res.begin(),
                       [](char c) {
                           switch (c) {
                               case ' ':
                               case '/':
                                   return '&';
                               case '_':
                                   return '/';
                               default:
                                   return static_cast<char>(std::tolower(c));
                           }
                       });
        if (res[0] == '&')
            res = res.substr(1);

        return res;
    }

    std::string fromSpacedPath(std::string_view path) {
        std::string res(path.size() + 1, '/');
        std::replace_copy_if(path.begin(), path.end(), res.begin() + 1,
                             [](auto n) { return n == ' '; }, '/');
        return res;
    }

    std::string toSpacedPath(std::string_view path) {
        std::string res(path.size(), '\0');
        std::replace_copy_if(path.begin(), path.end(), res.begin(),
                             [](auto n) { return n == '/'; }, ' ');
        if (res[0] == ' ') {
            return res.substr(1);
        }
        return res;
    }

    resType deleteUnusedSegments(trasnactionType &txn) {
        return txn.exec("delete from public.segments where segment_count=0");
    }

    indexType checkSegmentCount(trasnactionType &txn) {
        std::string qq = "select segment_hash,count(segment_hash) from data "
                         "group by  segment_hash except "
                         "select segment_hash,segment_count "
                         "from public.segments; ";
        resType rr = txn.exec(qq);
        if (rr.empty()) {
            return ReturnSucess;
        }
        return ErrorOccured;
    }

    resType
    getDedupCharacteristics(trasnactionType &txn, indexType segmentSize) {
        std::string query = "with segments as( "
                            "select f.file_name,d.segment_hash ,count(d.segment_hash),f.created_at from files f "
                            "        inner join public.data d on f.file_id = d.file_id "
                            "group by f.file_name,f.created_at,d.segment_hash "
                            "order by f.file_name) "
                            ", "
                            "unique_segments_count as( "
                            "select file_name,count(segment_hash) as unique_count,created_at  from segments "
                            "group by file_name, created_at) "
                            " "
                            " "
                            "select '/'||replace(f.file_name,' ','/') as path,size_in_bytes,aa.unique_count, "
                            "       ceil(size_in_bytes::float8/$1 "
                            "       ) as segment_count,(aa.unique_count /ceil(size_in_bytes::float8/$1 "
                            "                          )::float8)*100 as unique_percenatage "
                            ", aa.created_at "
                            "from files f\n"
                            "inner join  unique_segments_count aa on f.file_name=aa.file_name;";
        return txn.exec(query, pqxx::params(segmentSize));
    }

    resType
    getFileSizes(trasnactionType &txn) {
        std::string query = "with data as( "
                            "select '/'||replace(file_name,' ','/') as path, sum(octet_length(data.segment_hash)+8) as data_size, "
                            "       size_in_bytes as original_size "
                            "from data "
                            "inner join public.files f on data.file_id = f.file_id "
                            "group by f.file_name,size_in_bytes "
                            "order by file_name) "
                            "select *,data_size::float8/original_size*100 as ratio "
                            "from data;";
        return txn.exec(query);
    }
    void printRes(resType &rss, std::ostream &out) {
        int i = 0;
        for (; i < rss[0].size() - 1; ++i) {
            out << rss.column_name(i) << '\t';
        }
        out << rss.column_name(i) << '\n';
        for (const auto &row: rss) {
            i = 0;
            for (; i < row.size() - 1; ++i) {
                out << row[i].as<std::string>() << '\t';
            }
            out << row[i].as<std::string>() << '\n';
        }
    }

    resType getTotalSchemaSizes(trasnactionType &txn) {
        std::string qq = "SELECT\n"
                         "    indexrelname, s.relname, "
                         "    pg_size_pretty(pg_relation_size(indexrelid)) AS index_size, "
                         "    pg_size_pretty(pg_table_size(s.relname::text)) as t_size, "
                         "    pg_size_pretty(pg_relation_size(s.relname::text)) as rel_size, "
                         "    pg_size_pretty(pg_total_relation_size(s.relname::text)) as t_rel_size "
                         "FROM "
                         "    pg_stat_user_indexes s/*,pg_stat_all_tables*/\n"
                         "WHERE "
                         "        schemaname = 'public' order by index_size desc;";

        return txn.exec(qq);
    }

    tl::expected<indexType, int> getTotalFileSize(trasnactionType &txn) {
        try
        {
            return tl::expected<indexType, int>(txn.query_value<indexType>("select sum(size_in_bytes) from files;"));
        }
        catch (...)
        {
            return ErrorOccured;
        }
    }

    void printRowsAffected(resType &res) {
        VLOG(3) << vformat("Rows affected by latest request %d\n", res.affected_rows());
    }

    void printRowsAffected(resType &&res) {
        printRowsAffected(res);
    }

    resType checkTExistence(trasnactionType &txn, std::string_view fileName) {
        auto hashStr = getHashStr(fileName);
        std::string tableName = vformat("temp_file_%s", hashStr.c_str());

        std::string query = "select 1 from pg_tables "
                            "where tablename=\'%s\' and schemaname='public';";
        auto formattedQuery = vformat(query.c_str(), tableName.data(), tableName.c_str());
        return txn.exec(formattedQuery);
    }

    bool checkConnection(const conPtr &conn) {
        return conn && conn->is_open();
    }

    bool checkConnString(const myConnString &connString) {
        bool check = false;
        try {
            auto c = std::make_shared<pqxx::connection>(connString.c_str());
            check = checkConnection(c);
            c->close();
            c = nullptr;
        }
        catch (...) {
        }
        return check;
    }
}











