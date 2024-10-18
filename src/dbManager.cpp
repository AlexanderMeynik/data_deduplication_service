#include "dbManager.h"


void db_services::diconnect(db_services::conPtr &conn) {
    if (conn) {
        conn->close();
        conn = nullptr;
    }
}
