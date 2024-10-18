#ifndef DATA_DEDUPLICATION_SERVICE_MYCONNSTRING_H
#define DATA_DEDUPLICATION_SERVICE_MYCONNSTRING_H

#include <string_view>
#include <string>
#include <memory>

#include "myConcepts.h"

namespace db_services {

    struct myConnString {
        myConnString() : port(5432) {}

        myConnString(std::string_view user,
                     std::string_view password,
                     std::string_view host,
                     std::string_view dbname, unsigned port)
                : user(user), password(password),
                  host(host),
                  dbname(dbname),
                  port(port) {
            updateFormat();
        }

        explicit operator std::string() {
            return formattedString;
        }

        operator std::string_view() {
            return formattedString;
        }

        [[nodiscard]] const char *c_str() const {
            return formattedString.c_str();
        }

        void setUser(std::string_view newUser) {
            user = std::forward<std::string_view>(newUser);
            updateFormat();
        }

        void setPassword(std::string_view newPassword) {
            password = newPassword;
            updateFormat();
        }

        void setHost(std::string_view newHost) {
            host = newHost;
            updateFormat();
        }

        void setPort(unsigned newPort) {
            port = newPort;
            updateFormat();
        }

        void setDbname(std::string_view newDbname) {
            dbname = newDbname;
            updateFormat();
        }

        [[nodiscard]] const std::string &getUser() const {
            return user;
        }

        [[nodiscard]] const std::string &getPassword() const {
            return password;
        }

        [[nodiscard]] const std::string &getHost() const {
            return host;
        }

        [[nodiscard]] const std::string &getDbname() const {
            return dbname;
        }

        [[nodiscard]] unsigned int getPort() const {
            return port;
        }

    private:
        void updateFormat() {
            formattedString = myConcepts::vformat("postgresql://%s:%s@%s:%d/%s",
                                                  user.c_str(), password.c_str(), host.c_str(), port, dbname.c_str());
        }

        std::string user, password, host, dbname;
        unsigned port;
        std::string formattedString;
    };
}

#endif //DATA_DEDUPLICATION_SERVICE_MYCONNSTRING_H
