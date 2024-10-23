#include "MainWindow.h"

#include <QMenuBar>
#include <QFileInfo>
#include <QSqlQueryModel>


#include "SettingsWindow.h"

std::initializer_list<QString> ll = {"2", "4", "8", "16", "32"};//todo remake

namespace windows {


    MainWindow::MainWindow(QWidget *parent)
            : QMainWindow(parent) {
        this->setFixedSize(1024, 620);
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

        setCentralWidget(new QWidget());
        auto cent = centralWidget();


        auto mmm = new QHBoxLayout();
        mainLayout = new QVBoxLayout();

        inputFile = new FileLineEditWithOption(cent, QDir::currentPath());
        outputFile = new FileLineEditWithOption(cent, QDir::currentPath(), true);


        optionsLayout = new QHBoxLayout();
        labelHashFunction = new QLabel("Hash Function:");
        labelSegmentSize = new QLabel("Segment Size:");


        hashComboBox = new QComboBox(cent);
        segmentSizeComboBox = new QComboBox(cent);

        optionsLayout->addWidget(labelHashFunction);
        optionsLayout->addWidget(hashComboBox);
        optionsLayout->addWidget(labelSegmentSize);
        optionsLayout->addWidget(segmentSizeComboBox);

        buttonLayout = new QHBoxLayout();
        importButton = new QPushButton("Import");


        buttonLayout->addWidget(importButton);


        settingsAction = new QAction("Settings");
        menuBar()->addAction(settingsAction);


        exportButton = new QPushButton("Export");
        deleteButton = new QPushButton("Delete");

        hbl = new QHBoxLayout();
        fileExportField = new QComboBox();
        hbl->addWidget(exportButton);
        hbl->addWidget(deleteButton);

        //todo add database status(total blocks files size) files
        //todo for import and export add fileds for segment count etc.
        //todo add connection qled


        //todo use tree or list model
        //todo https://doc.qt.io/qt-6/sql-connecting.html
        //https://doc.qt.io/qt-6/qsqlquerymodel.html
        //todo can ve create our own model using pqxx res
        /*QSqlQueryModel aa;*/

        dataTable = new QTableWidget();
        dataTable->setColumnCount(3);
        QStringList headers;
        headers << "File" << "Hash" << "Size";
        dataTable->setHorizontalHeaderLabels(headers);

        logTextField = new QTextEdit();
        logTextField->setReadOnly(true);

        mainLayout->addWidget(inputFile);
        mainLayout->addLayout(optionsLayout);
        mainLayout->addLayout(buttonLayout);
        mainLayout->addWidget(dataTable);
        mainLayout->addWidget(fileExportField);
        mainLayout->addWidget(outputFile);
        mainLayout->addLayout(hbl);
        mmm->addLayout(mainLayout);

        auto VV = new QVBoxLayout();


        includeOptionsArea = new QGroupBox(tr("FileImportOptions"));
        replaceCB = new QCheckBox();
        replaceCB->setToolTip("If checked will replace file contents");
        auto incudeOptionLay = new QGridLayout();

        numberBlocks = new QLCDNumber();
        totalSize = new QLCDNumber();
        numbErSegments = new QLCDNumber();
        totalRepeatedBlocks = new QLCDNumber();
        totalRepeatedpercentage = new QLCDNumber();
        importTime = new QLCDNumber();

        incudeOptionLay->addWidget(new QLabel("Replace files on import"), 0, 0, 1, 2);
        incudeOptionLay->addWidget(replaceCB, 0, 2, 1, 1);


        incudeOptionLay->addWidget(new QLabel("Load time (ms)"),
                                   0, 3, 1, 1);
        incudeOptionLay->addWidget(importTime,
                                   0, 4, 1, 2);


        incudeOptionLay->addWidget(new QLabel("Total file blocks"), 1, 0, 1, 1);
        incudeOptionLay->addWidget(numberBlocks, 1, 1, 1, 1);
        incudeOptionLay->addWidget(new QLabel("Total file size"), 1, 2, 1, 1);
        incudeOptionLay->addWidget(totalSize, 1, 3, 1, 3);


        incudeOptionLay->addWidget(new QLabel("Files segment count"), 2, 0, 1, 1);
        incudeOptionLay->addWidget(numbErSegments, 2, 1, 1, 1);

        incudeOptionLay->addWidget(new QLabel("Duplicated blocks count"), 2, 2, 1, 1);
        incudeOptionLay->addWidget(totalRepeatedBlocks, 2, 3, 1, 1);

        incudeOptionLay->addWidget(totalRepeatedpercentage, 2, 4, 1, 1);
        incudeOptionLay->addWidget(new QLabel("%"), 2, 5, 1, 1);

        includeOptionsArea->setLayout(incudeOptionLay);


        auto exportOptionsArea = new QGroupBox(tr("FileExportOptions"));
        deleteFiles = new QCheckBox();
        deleteFiles->setToolTip("If checked will delete file/directory after export.");
        createMain = new QCheckBox();
        createMain->setToolTip("If checked  new directory will be created if out path does not exist!");
        auto exportOptionLay = new QGridLayout();
        exportOptionLay->addWidget(deleteFiles);
        exportOptionLay->addWidget(createMain);
        exportOptionsArea->setLayout(exportOptionLay);


        VV->addWidget(includeOptionsArea);
        VV->addWidget(logTextField);
        VV->addWidget(exportOptionsArea);


        mmm->addLayout(VV);

        /*mainLayout->addWidget(logTextField);*/
        centralWidget()->setLayout(mmm);
    }

    void MainWindow::onImport() {
        QString inputPath = inputFile->getContent();
        size_t segmentSize = segmentSizeComboBox->currentText().toUInt();
        if (QFileInfo(inputPath).isDir()) {
            writeLog("process directory");
            //fileService.processDirectory(inputPath.toStdString(), segmentSize);
        } else {
            writeLog("process file");
            //fileService.processFile(inputPath.toStdString(), segmentSize);
        }
    }

    void MainWindow::onExport() {
        // Example of calling loadDirectory or loadFile from FileParsingService
        QString outputPath = outputFile->getContent();
        QString fromPath = inputFile->getContent();
        if (QFileInfo(fromPath).isDir()) {
            writeLog("load directory");
            //fileService.loadDirectory(fromPath.toStdString(), outputPath.toStdString());
        } else {
            writeLog("load file");
            // fileService.loadFile(fromPath.toStdString(), outputPath.toStdString());
        }
    }

    void MainWindow::onDelete() {
        // Example of calling deleteFile or deleteDirectory from FileParsingService
        QString path = inputFile->getContent();
        if (QFileInfo(path).isDir()) {
            writeLog("delete directory");
            //fileService.deleteDirectory(path.toStdString());
        } else {
            writeLog("delete file");
            //fileService.deleteFile(path.toStdString());
        }
    }

    void MainWindow::onSettings() {

        SettingsWindow settingsWindow(this);
        auto stat = settingsWindow.exec();
        writeLog(std::to_string(stat).c_str());
        if (stat == QDialog::Accepted) {
            c_str = std::move(settingsWindow.getConfiguration());
            writeLog("update settings");
        }

        settingsWindow.close();

        writeLog(c_str.c_str());
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

    void MainWindow::writeLog(QString qss, LogLevel lg) {
        logTextField->setTextColor(colourLookUp[lg]);
        logTextField->append(QString(logLevelLookUp[lg]).arg(qss));
    }
}
