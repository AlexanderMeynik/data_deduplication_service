
#ifndef DATA_DEDUPLICATION_SERVICE_MAINWINDOW_H
#define DATA_DEDUPLICATION_SERVICE_MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>

#include <QLCDNumber>

#include "FileService.h"
#include "FileLineEdit.h"
#include "myConnString.h"

namespace windows {

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
    class MainWindow : public QMainWindow {
    Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = nullptr);


        ~MainWindow() override;

    private slots:

        void chekConditions();

        void onImport();

        void onExport();

        void onDelete();

        void onSettings();

        void writeLog(QString qss,LogLevel lg=RESULT);
    //todo at osme point we need either to create or to use existing database
    private:

        FileLineEditWithOption *inputFile;
        FileLineEditWithOption *outputFile;

        QComboBox *hashComboBox;
        QComboBox *segmentSizeComboBox;
        QPushButton *importButton;
        QPushButton *exportButton;
        QPushButton *deleteButton;
        QTableWidget *dataTable;
        QTextEdit *logTextField;

        QVBoxLayout *mainLayout;

        QHBoxLayout *optionsLayout;
        QHBoxLayout *buttonLayout;
        QHBoxLayout *hbl;

        QComboBox *fileExportField;

        QAction *settingsAction;

        QLabel *labelSegmentSize;
        QLabel *labelHashFunction;
        /*file_services::FileParsingService fileService;*/
        db_services::myConnString c_str;

        QGroupBox* includeOptionsArea;

        QCheckBox *replaceCB;
        QCheckBox* createMain;
        QCheckBox* deleteFiles;

        QLCDNumber * numberBlocks;
        QLCDNumber * totalSize;
        QLCDNumber * numbErSegments;
        QLCDNumber * totalRepeatedBlocks;
        QLCDNumber * totalRepeatedpercentage;

        QLCDNumber * importTime;

        QLCDNumber * exportTime;

        void setupUI();
    };
}


#endif //DATA_DEDUPLICATION_SERVICE_MAINWINDOW_H
