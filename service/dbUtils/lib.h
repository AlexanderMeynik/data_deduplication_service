#ifndef SERVICE_LIB_H
#define SERVICE_LIB_H

#include <pqxx/pqxx>
#include <vector>
#include <array>
#include <iostream>
#include "../common/myconcepts.h"

template<unsigned long segment_size>
requires IsDiv<segment_size, SHA256size>
void insert_bulk_segments(pqxx::connection &conn, const segvec<segment_size> &segments, std::string &filename) {
    try {
        pqxx::work txn(conn);
        std::string table_name="temp_file_"+filename;
        std::string q1 = "CREATE TABLE " + table_name + " (pos bigint, data char(" + std::to_string(segment_size) + "));";
        pqxx::result r2 = txn.exec(q1);
        //txn.exec("CREATE TABLE temp_file_data (pos bigint,data char(64));");

        pqxx::stream_to copy_stream(txn, table_name);

        for (size_t i = 0; i < segments.size(); ++i) {
            //std::cout<<std::string(segments[i].data(), 64)<<'\n';
            copy_stream << std::make_tuple(static_cast<int64_t>(i + 1), std::string(segments[i].data(), segment_size));
        }

        copy_stream.complete();
        std::string query = "select process_file_data2($1)";
        pqxx::result res = txn.exec_params(query, filename);
        txn.commit();
    } catch (const std::exception &e) {
        std::cerr << "Error inserting bulk data: " << e.what() << std::endl;
    }
}


template<typename T, unsigned long size>
//tdoo а надо ли
std::array<T, size> from_string(std::basic_string<T> &string) {
    std::array<T, size> res;
    for (int i = 0; i < size; ++i) {
        res[i] = string[i];
    }
    return res;
}


template<unsigned long segment_size>
requires IsDiv<segment_size, SHA256size>
segvec<segment_size> get_file_segmented(pqxx::connection &conn, std::string &filename) {
    segvec<segment_size> vector;
    try {
        pqxx::work txn(conn);
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


std::string get_file_contents(pqxx::connection &conn, std::string &filename)
{
    try {
        pqxx::work txn(conn);
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
