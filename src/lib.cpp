#include "lib.h"

tl::expected<db_services::conPtr,return_codes> db_services::connect_if_possible(std::string_view cString) {
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
    return tl::expected<db_services::conPtr,return_codes>{c};
}


