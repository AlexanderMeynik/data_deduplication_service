#include "MainWindow.h"

#include <QMenuBar>
#include <QFileInfo>
#include <QSqlQueryModel>


#include "SettingsWindow.h"

std::initializer_list<QString> ll = {"2", "4", "8", "16", "32"};//todo remake

/*std::initializer_list<QString>  hashes={ "sha224",
                                         "sha256",
                                         "md5",
                                         "sha384",
                                         "sha512"};//todo remake*/
namespace windows {


    MainWindow::MainWindow(QWidget *parent)
            : QMainWindow(parent) {

        setupUI();

        connect(importButton, &QPushButton::pressed, this, &MainWindow::onImport);
        connect(exportButton, &QPushButton::pressed, this, &MainWindow::onExport);
        connect(deleteButton, &QPushButton::pressed, this, &MainWindow::onDelete);


        connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettings);


        connect(inputFile, &FileLineEditWithOption::contentChanged, this, &MainWindow::chekConditions);
        connect(outputFile, &FileLineEditWithOption::contentChanged, this, &MainWindow::chekConditions);

        for (const char *hashName: hash_utils::hash_function_name) {
            hashComboBox->addItem(hashName);
        }
        segmentSizeComboBox->addItems(ll);
        chekConditions();

    }

    MainWindow::~MainWindow() {}

    void MainWindow::setupUI() {
        setWindowTitle("File Processing Application");

        centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);


        mainLayout = new QVBoxLayout(centralWidget);

        inputFile = new FileLineEditWithOption(this, QDir().currentPath());

        outputFile = new FileLineEditWithOption(this, QDir().currentPath(), true);


        optionsLayout = new QHBoxLayout();
        QLabel *labelHashFunction = new QLabel("Hash Function:", this);
        QLabel *labelSegmentSize = new QLabel("Segment Size:", this);


        hashComboBox = new QComboBox(this);
        segmentSizeComboBox = new QComboBox(this);

        optionsLayout->addWidget(labelHashFunction);
        optionsLayout->addWidget(hashComboBox);
        optionsLayout->addWidget(labelSegmentSize);
        optionsLayout->addWidget(segmentSizeComboBox);

        buttonLayout = new QHBoxLayout();
        importButton = new QPushButton("Import", this);
        exportButton = new QPushButton("Export", this);
        deleteButton = new QPushButton("Delete", this);

        buttonLayout->addWidget(importButton);
        buttonLayout->addWidget(exportButton);
        buttonLayout->addWidget(deleteButton);




        /*settingsButton = new QPushButton("Settings", this);*/
        /*buttonLayout->addWidget(settingsButton);*/


        auto hbl = new QHBoxLayout();
        settingsAction = new QAction("Settings");
        menuBar()->addAction(settingsAction);




        //todo use tree or list model
        //todo https://doc.qt.io/qt-6/sql-connecting.html
        //https://doc.qt.io/qt-6/qsqlquerymodel.html
        //todo can ve create our own model using pqxx res
        /*QSqlQueryModel aa;*/

        dataTable = new QTableWidget(this);
        dataTable->setColumnCount(3);
        QStringList headers;
        headers << "File" << "Hash" << "Size";
        dataTable->setHorizontalHeaderLabels(headers);

        logTextField = new QTextEdit(this);
        logTextField->setReadOnly(true);

        mainLayout->addWidget(inputFile);
        mainLayout->addWidget(outputFile);
        mainLayout->addLayout(optionsLayout);
        mainLayout->addLayout(buttonLayout);
        mainLayout->addWidget(dataTable);
        mainLayout->addWidget(logTextField);
    }

    void MainWindow::onImport() {
        QString inputPath = inputFile->getContent();
        size_t segmentSize = segmentSizeComboBox->currentText().toUInt();
        if (QFileInfo(inputPath).isDir()) {
            qInfo("process directory");
            //fileService.processDirectory(inputPath.toStdString(), segmentSize);
        } else {
            qInfo("process file");
            //fileService.processFile(inputPath.toStdString(), segmentSize);
        }
    }

    void MainWindow::onExport() {
        // Example of calling loadDirectory or loadFile from FileParsingService
        QString outputPath = outputFile->getContent();
        QString fromPath = inputFile->getContent();
        if (QFileInfo(fromPath).isDir()) {
            qInfo("load directory");
            //fileService.loadDirectory(fromPath.toStdString(), outputPath.toStdString());
        } else {
            qInfo("load file");
            // fileService.loadFile(fromPath.toStdString(), outputPath.toStdString());
        }
    }

    void MainWindow::onDelete() {
        // Example of calling deleteFile or deleteDirectory from FileParsingService
        QString path = inputFile->getContent();
        if (QFileInfo(path).isDir()) {
            qInfo("delete directory");
            //fileService.deleteDirectory(path.toStdString());
        } else {
            qInfo("delete file");
            //fileService.deleteFile(path.toStdString());
        }
    }

    void MainWindow::onSettings() {

        SettingsWindow settingsWindow(this);
        auto stat = settingsWindow.exec();
        qInfo(std::to_string(stat).c_str());
        if (stat == QDialog::Accepted) {
            c_str = std::move(settingsWindow.getConfiguration());
            qInfo("update settings");
        }

        settingsWindow.close();

        qInfo(c_str.c_str());
    }

    void MainWindow::chekConditions() {

        //todo if no connection all button will be disabled(+ add label to show that)

        auto in = inputFile->getContent();
        auto out = outputFile->getContent();

        bool cond = QFileInfo(in).isDir() ==
                    QFileInfo(out).isDir();

        importButton->setEnabled(!in.isEmpty());
        exportButton->setEnabled(!in.isEmpty() && !out.isEmpty()
                                 && cond);
        deleteButton->setEnabled(!in.isEmpty());
    }
}
