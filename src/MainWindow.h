
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

#include "FileService.h"
#include "FileLineEdit.h"
#include "myConnString.h"

namespace windows {


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

    private:
        QWidget *centralWidget;

        FileLineEditWithOption *inputFile;
        FileLineEditWithOption *outputFile;

        QComboBox *hashComboBox;
        QComboBox *segmentSizeComboBox;
        QPushButton *importButton;
        QPushButton *exportButton;
        QPushButton *deleteButton;
        /* QPushButton *settingsButton;*/
        QTableWidget *dataTable;
        QTextEdit *logTextField;

        QVBoxLayout *mainLayout;

        QHBoxLayout *optionsLayout;
        QHBoxLayout *buttonLayout;


        QAction *settingsAction;

        /*file_services::FileParsingService fileService;*/
        db_services::myConnString c_str;

        void setupUI();
    };
}


#endif //DATA_DEDUPLICATION_SERVICE_MAINWINDOW_H
