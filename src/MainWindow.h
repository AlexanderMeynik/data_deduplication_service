
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

#include "common.h"
#include "FileService.h"
#include "FileLineEdit.h"
#include "SettingsWindow.h"
#include "MyPqxxModel.h"

namespace windows {
    class MainWindow : public QMainWindow {
    Q_OBJECT
//todo check files(after export)
    public:
        explicit MainWindow(QWidget *parent = nullptr);


        ~MainWindow() override;

        void readConfiguration();

        void resizeEvent(QResizeEvent *event) override;

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

        void updateLEDS(QModelIndex &idx);

        void resetLeds();

    private:

        void writeLog(QString qss, LogLevel lg = RESULT) {
            ::writeLog(logTextField, qss, lg);
        }

        FileLineEditWithOption *inputFileLEWO;
        FileLineEditWithOption *outputFileLEWO;

        QComboBox *hashFunctionCoB;
        QComboBox *segmentSizeCoB;


        QPushButton *importPB;
        QPushButton *exportPB;
        QPushButton *deletePB;
        QPushButton *loadPB;
        QPushButton *dropPB;

        DeselectableTreeView *treeView;

        QTextEdit *logTextField;

        QVBoxLayout *mainLayout;


        QGridLayout * mmLayout;




        QLineEdit *fileExportLE;
        QAction *settingsAction;

        QLabel *labelSegmentSize;
        QLabel *labelHashFunction;
        //todo move some labels from source

        QGroupBox *includeOptionsArea;
        QGroupBox *exportOptionsArea;
        QGroupBox *databaseConfigurationArea;


        QGroupBox *importFileArea;
        QGroupBox *exportFileArea;

        QGridLayout * importFileAreaLay;
        QGridLayout * exportFileAreaLay;

        QCheckBox *replaceFileCB;
        QCheckBox *createMainCB;
        QCheckBox *deleteFilesCB;


        QLCDNumber *numberBlocksLCD;
        QLCDNumber *totalSizeLCD;
        QLCDNumber *fileSegmentLCD;
        QLCDNumber *totalRepeatedBlocksLCD;
        QLCDNumber *totalRepetitionPercentageLCD;
        QLCDNumber *importTimeLCD;
        QLCDNumber *exportTimeLCD;
        QLCDNumber *errorCountLCD;//todo run checks on the resulting files

        QCheckBox *dbUsageCB;
        QLineEdit *dataseLE;


        SettingsWindow *settingsWindow;

        MainTableModel *myViewModel;
        MySortFilterProxyModel *proxyModel;
        NotNullFilterProxyModel * nNullProxyModel;

        void setupUI();

        file_services::FileParsingService fileService;
        bool dbConnection;
        db_services::myConnString c_str;
    };
}


#endif //DATA_DEDUPLICATION_SERVICE_MAINWINDOW_H
