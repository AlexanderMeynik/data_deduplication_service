#ifndef DATA_DEDUPLICATION_SERVICE_COMMON_H
#define DATA_DEDUPLICATION_SERVICE_COMMON_H

#include "myConnString.h"
#include "qnamespace.h"

enum LogLevel
{
    INFO,
    WARNING,
    ERROR,
    RESULT
};

static constexpr std::array<const char*,4> logLevelLookUp=
        {
                "[INFO] %1",
                "[WARNING] %1",
                "[ERROR] %1",
                "[RESULT] %1",
        };
static constexpr std::array<Qt::GlobalColor,4> colourLookUp
        {
                Qt::black,
                Qt::darkYellow,
                Qt::red,
                Qt::darkGreen
        };
#endif //DATA_DEDUPLICATION_SERVICE_COMMON_H
