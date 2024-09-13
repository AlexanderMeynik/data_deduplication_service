#ifndef SERVICE_LIB_H
#define SERVICE_LIB_H

#include <pqxx/pqxx>
#include <vector>
#include <array>
#include <iostream>
#include "../common/myconcepts.h"
#include <memory>

#include <vector>
#include <fstream>




struct MyCstring
{

    operator std::string(){
        return formatted_string;
    }

    const char* c_str() const {
        return formatted_string.c_str();
    }
    void update_format()
    {
        formatted_string=vformat("postgresql://%s:%s@%s:%d/%s",
                                 user.c_str(),password.c_str(),host.c_str(),port,dbname.c_str());
    }

    std::string user,password,host,dbname;
    unsigned port;
    std::string formatted_string;
};
MyCstring getSc(std::string filename,std::string dbName,unsigned port=5501) {

    std::ifstream conf(filename);
    std::string dbname, user, password;

    conf >> dbname >> user >> password;
    auto res=MyCstring{user,password,"localhost",dbname,port};
    res.update_format();
    return res;
}

template<bool verbose= false,typename str>
requires std::is_convertible<str,std::string>::value
//todo c_str() callable
std::shared_ptr<pqxx::connection> connect_if_possible(str&& cString)
{
    std::shared_ptr<pqxx::connection>c;
    try {
        c=std::make_shared<pqxx::connection>(cString);
        if (!c->is_open()) {
            if(verbose) {
                std::cout << vformat("Unable to connect by url \"%s\"\n", cString.c_str());
            }
            return nullptr;
        }
    } catch (const std::exception &e) {
        if(verbose) {
            std::cout << "Exception: " << e.what() << std::endl;
        }
        return nullptr;
    }
    if(verbose) {
        std::cout << "Opened database successfully: " << c->dbname() << std::endl;
    }
    return c;
}
//from https://stackoverflow.com/questions/49122358/libbqxx-c-api-to-connect-to-postgresql-without-db-name
template<unsigned long segment_size>
void createDB(MyCstring cString, std::string&dbName)
{
    cString.dbname="template1";
    cString.update_format();
    auto c=connect_if_possible(cString);

    if(!c->is_open())
    {
        std::cout <<vformat("Unable to connect by url \"%s\"\n",(cString).operator std::string().c_str());
        return;
    }
    std::cout <<vformat("Connected to database %s\n", cString.dbname.c_str());
    auto name=vformat("%s_%d", dbName.c_str(),segment_size);
    pqxx::nontransaction w(*c);
    try {
        w.exec(vformat("CREATE DATABASE %s;",name.c_str()));
    } catch (pqxx::sql_error &e) {
        std::string sqlErrCode = e.sqlstate();
        if (sqlErrCode == "42P04") { // catch duplicate_database
            std::cout <<vformat("Database: %s exists, proceeding further\n", cString.dbname.c_str());
            c->disconnect();
            return;
        }
        std::cerr <<vformat("Database error: %s, error code: %sSQL Query was: %s\n", cString.dbname.c_str());
        abort();
    }

    w.exec(vformat("GRANT ALL ON DATABASE %s TO %s;", name.c_str(), cString.user.c_str()));
    w.commit();
    c->disconnect();

   /* cString.dbname=name;
    * cString.update_format();
    c= connect_if_possible(cString);*/
}


template<unsigned long segment_size>
requires IsDiv<segment_size, SHA256size>
void insert_bulk_segments(const std::shared_ptr<pqxx::connection> & conn, const segvec<segment_size> &segments, std::string &filename) {
    try {
        pqxx::work txn(*conn);
        std::string table_name="temp_file_"+filename;
        std::string q1 = "CREATE TABLE " + table_name + " (pos bigint, data char(" + std::to_string(segment_size) + "));";
        pqxx::result r2 = txn.exec(q1);


        pqxx::stream_to copy_stream(txn, table_name);

        for (size_t i = 0; i < segments.size(); ++i) {
            copy_stream << std::make_tuple(static_cast<int64_t>(i + 1), std::string(segments[i].data(), segment_size));
        }

        copy_stream.complete();
        std::string query = "select process_file_data($1)";
        pqxx::result res = txn.exec_params(query, filename);
        txn.commit();
    } catch (const std::exception &e) {
        std::cerr << "Error inserting bulk data: " << e.what() << std::endl;
    }
}


template<typename T, unsigned long size>
std::array<T, size> from_string(std::basic_string<T> &string) {
    std::array<T, size> res;
    for (int i = 0; i < size; ++i) {
        res[i] = string[i];
    }
    return res;
}




template<unsigned long segment_size>
requires IsDiv<segment_size, SHA256size>
segvec<segment_size> get_file_segmented(const std::shared_ptr<pqxx::connection> & conn, std::string &filename) {
    segvec<segment_size> vector;
    try {
        pqxx::work txn(*conn);
        std::string query = "select get_file_data($1)";
        pqxx::result res = txn.exec_params(query, filename);
        txn.commit();
        for (const auto &re: res) {
            auto string = re[0].as<std::string>();
            vector.push_back(from_string<char, 64>(string));
        }

    } catch (const std::exception &e) {
        std::cerr << "get_file_segments" << e.what() << std::endl;
    }
    return vector;
}


std::string get_file_contents(const std::shared_ptr<pqxx::connection> & conn, std::string &filename)
{
    try {
        pqxx::work txn(*conn);
        std::string query = "select string_agg(ss.data,'')\n"
                            "from \n"
                            "    (\n"
                            "    select get_file_data($1) as data \n"
                            "    )as ss;";
        pqxx::result res = txn.exec_params(query, filename);
        txn.commit();
        return res[0][0].as<std::string>();

    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
    return "";
}

#endif //SERVICE_LIB_H
