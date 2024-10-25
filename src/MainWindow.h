
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
        MyPqxxModel(file_services::FileParsingService &fs):fsRef(fs)
        {
            res=pqxx::result();
            }
        template<typename ResType1, typename ... Args>
        void  executeInTransaction(ResType1
                                                         (*call)(db_services::trasnactionType &, Args ...),
                                                         Args &&... args) {
            auto ss=fsRef.executeInTransaction(call, std::forward<Args>(args)...);
            if(!ss.has_value())
            {
                res=pqxx::result();
            }
            res =ss.value();
        }

        template<typename ResType1, typename ... Args>
        void
        executeInTransaction(const std::function<ResType1(db_services::trasnactionType &, Args ...)> &call,
                             Args &&... args) {
            auto ss=fsRef.executeInTransaction(call, std::forward<Args>(args)...);
            if(!ss.has_value())
            {
                res=pqxx::result();
            }
            res =ss.value();
        }

        int rowCount(const QModelIndex &parent) const
        {
           return res.size();
        }
        int columnCount(const QModelIndex &parent) const
        {
            return res.columns();
        }
        QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const override
        {

            //qInfo(std::to_string(section).c_str());
            if(role == Qt::DisplayRole)
            {
                if(orientation==Qt::Orientation::Vertical)
                {
                    return QVariant(section);
                }
               return QVariant(QString::fromStdString(res.column_name(section)));
            }
            return QVariant();
        }
        QVariant data(const QModelIndex &index, int role) const
        {

            if (role == Qt::DisplayRole &&index.row()<res.size()&&index.column()<res.columns()) {
                /*QString unswer = QString("row = ") + QString::number(index.row()) + "  col = " + QString::number(index.column());*/
                return QVariant(QString::fromStdString(

                        res[index.row()][index.column()].as<std::string>()));
            }
            return QVariant();
        }
        pqxx::result res;
    file_services::FileParsingService& fsRef;
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

        void onModelSet();
    private:

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

        MyPqxxModel* mydmod;
        void setupUI();

        file_services::FileParsingService fileService;//todo save configuration path to xml(or copy one from form)
    };
}


#endif //DATA_DEDUPLICATION_SERVICE_MAINWINDOW_H
