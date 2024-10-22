
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
#include "myConnString.h"

namespace windows {
    /*QT_BEGIN_NAMESPACE
    namespace Ui { class MainWindow; }
    QT_END_NAMESPACE*/


    class MainWindow : public QMainWindow {
    Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = nullptr);

        ~MainWindow() override;

    private slots:

        void onImport();

        void onExport();

        void onDelete();

        void onSettings();

    private:
        // Widgets
        QWidget *centralWidget;
        QLineEdit *inputPathChooser;
        QLineEdit *outputPathChooser;
        QComboBox *hashComboBox;
        QComboBox *segmentSizeComboBox;
        QPushButton *importButton;
        QPushButton *exportButton;
        QPushButton *deleteButton;
        QPushButton *settingsButton;
        QTableWidget *dataTable;
        QTextEdit *logTextField;

        // Layouts
        QVBoxLayout *mainLayout;
        QHBoxLayout *fileChooserLayout;
        QHBoxLayout *outputChooserLayout;
        QHBoxLayout *optionsLayout;
        QHBoxLayout *buttonLayout;

        /*file_services::FileParsingService fileService;*/
        db_services::myConnString c_str;

        void setupUI();
    };
}


#endif //DATA_DEDUPLICATION_SERVICE_MAINWINDOW_H
