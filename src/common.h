#ifndef DATA_DEDUPLICATION_SERVICE_COMMON_H
#define DATA_DEDUPLICATION_SERVICE_COMMON_H


#include <QRegularExpression>
#include <QTextEdit>
#include <unordered_set>

#include "myConnString.h"
#include "FileService.h"

namespace common {
    static const char *const confName = "configuration.xml";

    static const QString parentTag = "fileInfo";
    enum LogLevel {
        INFO,
        WARNING,
        ERROR,
        RESULT
    };

    static constexpr std::array<const char *, 4> logLevelLookUp =
            {
                    "[INFO] %1",
                    "[WARNING] %1",
                    "[ERROR] %1",
                    "[RESULT] %1",
            };
    static constexpr std::array<Qt::GlobalColor, 4> colourLookUp
            {
                    Qt::black,
                    Qt::darkYellow,
                    Qt::red,
                    Qt::darkGreen
            };

    using file_services::FileService;

    typedef int (FileService::* dirLoad)(std::basic_string_view<char> fromDir,
                                         std::basic_string_view<char> toDir);

    typedef int (FileService::*fileLoad)(std::basic_string_view<char> fromDir,
                                         std::basic_string_view<char> toDir,
                                         db_services::indexType fileId);

    extern std::unordered_map<int, dirLoad>
            dirs;

    extern std::unordered_map<int, fileLoad>
            files;


    void writeLog(QTextEdit *logTextField, const QString &qss, LogLevel lg = RESULT);


    bool isDirName(const QString &path);

    double smartCeil(double vaL, uint num);

}
#endif //DATA_DEDUPLICATION_SERVICE_COMMON_H
