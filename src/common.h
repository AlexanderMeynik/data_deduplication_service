#ifndef DATA_DEDUPLICATION_SERVICE_COMMON_H
#define DATA_DEDUPLICATION_SERVICE_COMMON_H


#include <QRegularExpression>
#include <QTextEdit>
#include <unordered_set>

#include "myConnString.h"
#include "FileService.h"

namespace common {
    using file_services::FileService;

    /**
     * Configuration .xml path
     */
    static const char *const confName = "configuration.xml";


    static const QString parentTag = "fileInfo";
    /**
     * Log level enum
     */
    enum LogLevel {
        INFO,
        WARNING,
        ERROR,
        RESULT
    };

    /**
     * Lookup for format strings
     */
    static constexpr std::array<const char *, 4> logLevelLookUp =
            {
                    "[INFO] %1",
                    "[WARNING] %1",
                    "[ERROR] %1",
                    "[RESULT] %1",
            };
    /**
     * Lookup for text color
     */
    static constexpr std::array<Qt::GlobalColor, 4> colorLookUp
            {
                    Qt::black,
                    Qt::darkYellow,
                    Qt::red,
                    Qt::darkGreen
            };


    /**
     * Typedef for load directory
     * @see @ref file_services::FileService::loadDirectory "loadDirectory"
     */
    typedef int (FileService::* dirLoad)(std::basic_string_view<char> fromDir,
                                         std::basic_string_view<char> toDir);
    /**
     * Typedef for load directory
     * @see @ref file_services::FileService::loadFile "loadFile"
     */
    typedef int (FileService::*fileLoad)(std::basic_string_view<char> fromDir,
                                         std::basic_string_view<char> toDir,
                                         db_services::indexType fileId);

    extern std::unordered_map<int, dirLoad>
            dirs;

    extern std::unordered_map<int, fileLoad>
            files;

    /**
     * Writes formated log message to logTextField
     * @param logTextField
     * @param qLogMessage
     * @param lg
     */
    void writeLog(QTextEdit *logTextField, const QString &qLogMessage, LogLevel lg = RESULT);

    /**
     * Check that path is directory path with regex
     * @param path
     */
    bool isDirName(const QString &path);

    /**
     * Returns value rounded up to num symbols after ,
     * @param vaL
     * @param num
     */
    double smartCeil(double vaL, uint num);

}
#endif //DATA_DEDUPLICATION_SERVICE_COMMON_H
