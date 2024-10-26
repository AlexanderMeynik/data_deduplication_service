#ifndef DATA_DEDUPLICATION_SERVICE_COMMON_H
#define DATA_DEDUPLICATION_SERVICE_COMMON_H

#include "qnamespace.h"

#include <QTextEdit>
#include "myConnString.h"

static const char *const confName = "configuration.xml";

static const QString parentTag = "fileInfo";
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


void inline writeLog(QTextEdit *logTextField,QString qss,LogLevel lg=RESULT)
{
    logTextField->setTextColor(colourLookUp[lg]);
    logTextField->append(QString(logLevelLookUp[lg]).arg(qss));
}
#endif //DATA_DEDUPLICATION_SERVICE_COMMON_H
