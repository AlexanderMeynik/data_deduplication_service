
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
#include <QCompleter>
#include <QTreeView>
#include <QProgressBar>
#include <QStatusBar>
#include <QTimer>
#include <QApplication>
#include <QDataWidgetMapper>

#include "common.h"
#include "FileService.h"
#include "FileLineEdit.h"
#include "SettingsWindow.h"
#include "myPqxxModel.h"

/// windows namespace
namespace windows {
    using namespace common;
    using namespace models;

    /**
     * @brief MainWindow class
     */
    class MainWindow : public QMainWindow {
    Q_OBJECT
    public:
        explicit MainWindow(QWidget *parent = nullptr);


        ~MainWindow() override;

        void readConfiguration();

        void resizeEvent(QResizeEvent *event) override;

    signals:

        void connectionChanged(bool old);

        void modelUpdate();

        void closed();

    private slots:

        void activateButtonsd();

        void onImport();

        void onExport();

        void onDelete();

        void onSettings();

        void onloadDatabase();

        void onConnectionChanged(bool old);

        void updateModel();

        void updateLeds(QModelIndex &idx);

        void resetLeds(int i);

        void calculateCoefficient();


    private:

        void writeLog(const QString &qss, logLevel lg = INFO) {
            common::writeLog(logTextField, qss, lg);
        }

        QAction *settingsAction;

        FileLineEditWithOption *inputFileLEWO;
        FileLineEditWithOption *outputFileLEWO;


        QLineEdit *fileExportLE;
        QLineEdit *dataseNameLE;

        QComboBox *hashFunctionCoB;
        QComboBox *segmentSizeCoB;

        QPushButton *importPB;
        QPushButton *exportPB;
        QPushButton *deletePB;
        QPushButton *connectPB;
        QPushButton *dropPB;

        deselectableTableView *dataTableView;

        QTextEdit *logTextField;


        QGroupBox *includeOptionsArea;
        QGroupBox *exportOptionsArea;
        QGroupBox *databaseConfigurationArea;
        QGroupBox *importFileArea;
        QGroupBox *exportFileArea;

        QGridLayout *mmLayout;

        QGridLayout *incudeOptionLay;
        QGridLayout *exportOptionLay;
        QGridLayout *dbOptionLay;
        QGridLayout *importFileAreaLay;
        QGridLayout *exportFileAreaLay;

        QLabel *labelSegmentSize;
        QLabel *labelHashFunction;


        QCheckBox *replaceFileCB;
        QCheckBox *createMainCB;
        QCheckBox *deleteFilesCB;
        QCheckBox *dbUsageCB;
        QCheckBox *compareCB;

        QLCDNumber *fileDataSizeLCD;
        QLCDNumber *segmentSizeLCD;
        QLCDNumber *totalSizeLCD;
        QLCDNumber *fileSegmentLCD;
        QLCDNumber *totalRepeatedBlocksLCD;
        QLCDNumber *dataToOriginalPercentageLCD;
        QLCDNumber *totalRepetitionPercentageLCD;
        QLCDNumber *importTimeLCD;

        QLCDNumber *exportTimeLCD;
        QLCDNumber *errorCountLCD;
        QLCDNumber *deleteTimeLCD;
        QLCDNumber *totalBlocksLCD;
        QLCDNumber * checkTimeLCD;
        QLCDNumber * uniquePercentage;
        QLCDNumber *deduplicationPercentage;
        QList<QLCDNumber *> list;



        QLedIndicator *qled;

        SettingsWindow *settingsWindow;

        deduplicationCharacteristicsModel *myViewModel;
        mySortFilterProxyModel *proxyModel;
        notNullFilterProxyModel *nNullProxyModel;


        QRegularExpression re;
        QRegularExpressionValidator *validator;
        QTimer *timer;


        file_services::FileService fileService;
        bool dbConnection;
        db_services::myConnString cStr;

        void setupUi();

        static QString toShortPath(const QString &qString);

        void updateStylesheet();


        void compareExport(const QString &exportee, const QString &output, bool isDirectory);
    };
}


#endif //DATA_DEDUPLICATION_SERVICE_MAINWINDOW_H
