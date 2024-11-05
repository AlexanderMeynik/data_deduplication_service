#include "common.h"

namespace common {


    std::unordered_map<int, dirLoad>
            dirs =
            {
                    {0, &FileService::loadDirectory<file_services::NoCreateMain, file_services::Persist>},
                    {1, &FileService::loadDirectory<file_services::NoCreateMain, file_services::Remove>},
                    {2, &FileService::loadDirectory<file_services::CreateMain, file_services::Persist>},
                    {3, &FileService::loadDirectory<file_services::CreateMain, file_services::Remove>},
            };

    std::unordered_map<int, fileLoad>
            files =
            {
                    {0, &FileService::loadFile<file_services::NoCreateMain, file_services::Persist>},
                    {1, &FileService::loadFile<file_services::NoCreateMain, file_services::Remove>},
                    {2, &FileService::loadFile<file_services::CreateMain, file_services::Persist>},
                    {3, &FileService::loadFile<file_services::CreateMain, file_services::Remove>},
            };

    void writeLog(QTextEdit *logTextField, const QString &qLogMessage, logLevel lg) {
        logTextField->setTextColor(colorLookUp[lg]);
        logTextField->append(QString(logLevelLookUp[lg]).arg(qLogMessage));
        logTextField->update();
    }

    bool isDirName(const QString &path) {
        return !QRegularExpression(R"(\.\w+$)").match(path).hasMatch();
    }

    double smartCeil(double vaL, uint num) {
        if (num == 0) {
            return vaL;
        }

        double c = pow(10, num);
        return static_cast<unsigned >( (vaL * c)) / c;
    }
}