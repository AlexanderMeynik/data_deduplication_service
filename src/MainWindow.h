
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
#include <QValidator>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QXmlStreamAttribute>
#include <QSortFilterProxyModel>

#include "common.h"
#include "FileService.h"
#include "FileLineEdit.h"
#include "SettingsWindow.h"
#include "MyPqxxModel.h"

namespace windows {




    class MainWindow : public QMainWindow {
    Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = nullptr);


        ~MainWindow() override;
        void readConfiguration();
        void resizeEvent(QResizeEvent* event);
    signals:
        void connectionChanged(bool old);
    private slots:

        void activateButtonsd();

        void onImport();

        void onExport();

        void onDelete();

        void onSettings();

        void onloadDatabase();


        void onConnectionChanged(bool old);

        void updateModel(size_t index);


    private:

        void writeLog(QString qss,LogLevel lg=RESULT)
        {
            ::writeLog(logTextField,qss,lg);
        }

        FileLineEditWithOption *inputFile;
        FileLineEditWithOption *outputFile;

        QComboBox *hashComboBox;
        QComboBox *segmentSizeComboBox;


        QPushButton *importButton;
        QPushButton *exportButton;
        QPushButton *deleteButton;
        QPushButton *loadDb;
        QPushButton *dropDb;

        QTableView *dataTable;
        QTextEdit *logTextField;

        QVBoxLayout *mainLayout;

        QHBoxLayout *optionsLayout;
        QHBoxLayout *buttonLayout;
        QHBoxLayout *hbl;
        //todo filter by contents of this file
        // https://doc.qt.io/qt-6/qtwidgets-itemviews-customsortfiltermodel-example.html
        //todo add auto complete using model
        QLineEdit *fileExportField;

        QAction *settingsAction;

        QLabel *labelSegmentSize;
        QLabel *labelHashFunction;

        QGroupBox* includeOptionsArea;
        QGroupBox * exportOptionsArea;
        QGroupBox * databaseConfiguration;

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
        QLCDNumber * errorCount;//todo run checks on the resulting files
        QCheckBox* createNewDbCB;
        QLineEdit * dataseName;

        bool dbConnection;
        db_services::myConnString c_str;


        SettingsWindow * settingsWindow;


        MyPqxxModel* mydmod;
        QSortFilterProxyModel *proxyModel;
        void setupUI();

        file_services::FileParsingService fileService;
    };
}


#endif //DATA_DEDUPLICATION_SERVICE_MAINWINDOW_H
