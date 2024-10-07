#ifndef SOURCE_SERVICE_TESTCLASSES_H
#define SOURCE_SERVICE_TESTCLASSES_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "FileService.h"
#include "dbManager.h"
#include "../testUtils.h"

using namespace db_services;
using m_type = dbManager<64>;


template<typename Ret, typename ... Args>
tl::expected<Ret, int> wrap_non_trans_function(Ret (*call)(nonTransType &, Args ...), Args ... args) {

    auto tString = default_configuration();
    tString.set_dbname(sample_temp_db);

    auto result = connect_if_possible(tString);

    auto temp_connection = result.value_or(nullptr);

    if (!result.has_value()) {
        VLOG(1)
                        << vformat("Unable to connect by url \"%s\"\n", (tString).operator std::string().c_str());
        return tl::unexpected(return_codes::error_occured);
    }
    VLOG(1) << vformat("Connected to database %s\n", tString.getDbname().c_str());


    nonTransType no_trans_exec(*temp_connection);
    Ret res;
    try {
        res = call(no_trans_exec, args ...);
    } catch (const pqxx::sql_error &e) {
        VLOG(1) << "SQL Error: " << e.what()
                << "Query: " << e.query()
                << "SQL State: " << e.sqlstate() << '\n';
        return tl::unexpected(return_codes::error_occured);
    }
    catch (const pqxx::unexpected_rows &r) {
        VLOG(1) << "Unexpected rows";
        VLOG(2) << "    exception message: " << r.what();
        return tl::unexpected(return_codes::already_exists);
    }
    catch (const std::exception &e) {
        VLOG(1) << "Error: " << e.what() << '\n';
        return tl::unexpected(return_codes::error_occured);
    }
    return tl::expected<Ret, int>{res};
};


template<typename ResType1, typename ... Args>
requires  (!std::is_void_v<ResType1>)
tl::expected<ResType1, int>
wrap_trans_function(conPtr &conn, ResType1 (*call)(trasnactionType &, Args ...), Args &&... args) {
    if (!checkConnection(conn)) {
        VLOG(1)
                        << vformat("Unable to open conncetion");
        return tl::unexpected(return_codes::error_occured);
    }
    trasnactionType no_trans_exec(*conn);
    ResType1 res;

    try {
        res = call(no_trans_exec, std::forward<Args>(args) ...);

    } catch (const pqxx::sql_error &e) {
        VLOG(1) << "SQL Error: " << e.what()
                << "Query: " << e.query()
                << "SQL State: " << e.sqlstate() << '\n';
        return tl::unexpected(return_codes::error_occured);
    }
    catch (const pqxx::unexpected_rows &r) {
        VLOG(1) << "Unexpected rows";
        VLOG(2) << "    exception message: " << r.what();
        return tl::unexpected(return_codes::already_exists);
    }
    catch (const std::exception &e) {
        VLOG(1) << "Error: " << e.what() << '\n';
        return tl::unexpected(return_codes::error_occured);
    }
    return tl::expected<ResType1, int>{res};
};


template<typename ... Args>
int wrap_trans_function(conPtr &conn, void (*call)(trasnactionType &, Args ...), Args &&... args) {
    if (!checkConnection(conn)) {
        VLOG(1)
                        << vformat("Unable to open conncetion");
        return return_codes::error_occured;
    }
    trasnactionType no_trans_exec(*conn);

    try {
        call(no_trans_exec, std::forward<Args>(args) ...);
        return return_codes::return_sucess;
    } catch (const pqxx::sql_error &e) {
        VLOG(1) << "SQL Error: " << e.what()
                << "Query: " << e.query()
                << "SQL State: " << e.sqlstate() << '\n';
    }
    catch (const pqxx::unexpected_rows &r) {
        VLOG(1) << "Unexpected rows";
        VLOG(2) << "    exception message: " << r.what();
    }
    catch (const std::exception &e) {
        VLOG(1) << "Error: " << e.what() << '\n';
    }
    return return_codes::error_occured;
};


#endif //SOURCE_SERVICE_TESTCLASSES_H
