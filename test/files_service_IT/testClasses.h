#ifndef SOURCE_SERVICE_TESTCLASSES_H
#define SOURCE_SERVICE_TESTCLASSES_H

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ServiceFileInterface.h"
#include "dbManager.h"
#include "../testUtils.h"//todo include?
using namespace db_services;

template <size_t sz,pqxx::result(dbManager<sz>::*call)(nonTransType&)>
tl::expected<ResType,int> wrap_non_trans_method(dbManager<sz>&obj)
{

    auto tString = default_configuration();
    tString.set_dbname(sample_temp_db);

    auto result= connect_if_possible(tString);

    auto temp_connection=result.value_or(nullptr);

    if (!result.has_value()) {
        VLOG(1)
                        << vformat("Unable to connect by url \"%s\"\n", (tString).operator std::string().c_str());
        return tl::unexpected(return_codes::error_occured);
    }
    VLOG(1) << vformat("Connected to database %s\n", tString.getDbname().c_str());


    nonTransType no_trans_exec(*temp_connection);
    pqxx::result res;
    try {
        res=(obj.*call)(no_trans_exec);
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
    return tl::expected<ResType ,int>{res};
};


template <size_t sz,pqxx::result(dbManager<sz>::*call)(trasnactionType&)>
tl::expected<ResType,int> wrap_trans_method(dbManager<sz>&obj)
{

    auto tString = obj.getCString();

    auto result= connect_if_possible(tString);

    auto temp_connection=result.value_or(nullptr);

    if (!result.has_value()) {
        VLOG(1)
                        << vformat("Unable to connect by url \"%s\"\n", (tString).operator std::string().c_str());
        return tl::unexpected(return_codes::error_occured);
    }
    VLOG(1) << vformat("Connected to database %s\n", tString.getDbname().c_str());


    trasnactionType no_trans_exec(*temp_connection);
    pqxx::result res;
    try {
        res=(obj.*call)(no_trans_exec);
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
    return tl::expected<ResType ,int>{res};
};






#endif //SOURCE_SERVICE_TESTCLASSES_H
