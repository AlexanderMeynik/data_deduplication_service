
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

#include "common.h"
#include "FileService.h"
#include "FileLineEdit.h"
#include "SettingsWindow.h"

namespace windows {


    class MyPqxxModel: public QAbstractTableModel
    {
    public:
        //todo execute transcation
    private:
        int rowCount(const QModelIndex &parent) const
        {
           return res.size();
        }
        int columnCount(const QModelIndex &parent) const
        {
            return res.columns();
        }
        QVariant data(const QModelIndex &index, int role) const
        {
            if (role == Qt::DisplayRole) {
                QString unswer = QString("row = ") + QString::number(index.row()) + "  col = " + QString::number(index.column());
                // строкой выше мы формируем ответ. QString::number преобразует число в текст
                return QVariant(unswer);
            }
            return QVariant();
        }
        pqxx::result& res;
    };


    class MainWindow : public QMainWindow {
    Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = nullptr);


        ~MainWindow() override;

    private slots:

        void activateButtonsd();

        void onImport();

        void onExport();

        void onDelete();

        void onSettings();

        void writeLog(QString qss,LogLevel lg=RESULT);

        void onloadDatabase();
    //todo at osme point we need either to create or to use existing database
    private:

        FileLineEditWithOption *inputFile;
        FileLineEditWithOption *outputFile;

        QComboBox *hashComboBox;
        QComboBox *segmentSizeComboBox;


        QPushButton *importButton;
        QPushButton *exportButton;
        QPushButton *deleteButton;
        QPushButton *loadDb;

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
        void setupUI();

        /*file_services::FileParsingService fileService;*/
    };
}


#endif //DATA_DEDUPLICATION_SERVICE_MAINWINDOW_H
