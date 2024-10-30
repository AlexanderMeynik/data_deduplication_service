#include "common.h"

namespace common {


    extern std::unordered_map<int, dirLoad>
            dirs =
            {
                    {0, &FileParsingService::loadDirectory<file_services::NoCreateMain, file_services::Persist>},
                    {1, &FileParsingService::loadDirectory<file_services::NoCreateMain, file_services::Remove>},
                    {2, &FileParsingService::loadDirectory<file_services::CreateMain, file_services::Persist>},
                    {3, &FileParsingService::loadDirectory<file_services::CreateMain, file_services::Remove>},
            };

    std::unordered_map<int, fileLoad>
            files =
            {
                    {0, &FileParsingService::loadFile<file_services::NoCreateMain, file_services::Persist>},
                    {1, &FileParsingService::loadFile<file_services::NoCreateMain, file_services::Remove>},
                    {2, &FileParsingService::loadFile<file_services::CreateMain, file_services::Persist>},
                    {3, &FileParsingService::loadFile<file_services::CreateMain, file_services::Remove>},
            };

    void writeLog(QTextEdit *logTextField, const QString &qss, LogLevel lg) {
        logTextField->setTextColor(colourLookUp[lg]);
        logTextField->append(QString(logLevelLookUp[lg]).arg(qss));
    }

    bool isDirName(const QString &path) {
        return !QRegularExpression(R"(\.\w+$)").match(path).hasMatch();
    }

    double smartCeil(double vaL, uint num) {
        if (num == 0) {
            return vaL;
        }

        double c = pow(10, num);
        return ((int64_t) (vaL * c)) / c;
    }
}