#include <QFileInfo>
#include "MainWindow.h"
#include "SettingsWindow.h"

namespace windows {


    MainWindow::MainWindow(QWidget *parent)
            : QMainWindow(parent) {

        // Call the function to setup the UI
        setupUI();

        // Connect buttons to their respective slots
        connect(importButton, &QPushButton::pressed, this, &MainWindow::onImport);
        connect(exportButton, &QPushButton::pressed, this, &MainWindow::onExport);
        connect(deleteButton, &QPushButton::pressed, this, &MainWindow::onDelete);
        connect(settingsButton, &QPushButton::pressed, this, &MainWindow::onSettings);

        // Set up combobox with hash functions from hash_utils
        for (const char *hashName: hash_utils::hash_function_name) {
            hashComboBox->addItem(hashName);
        }
        // Setup segment sizes in combobox
        segmentSizeComboBox->addItems({"2", "4", "8"});

    }

    MainWindow::~MainWindow() {}

    void MainWindow::setupUI() {
        setWindowTitle("File Processing Application");

        centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);

        mainLayout = new QVBoxLayout(centralWidget);

        fileChooserLayout = new QHBoxLayout();
        QLabel *labelInputFile = new QLabel("Input File/Directory:", this);
        inputPathChooser = new QLineEdit(this);
        QPushButton *browseInputButton = new QPushButton("Browse...", this);
        fileChooserLayout->addWidget(labelInputFile);
        fileChooserLayout->addWidget(inputPathChooser);
        fileChooserLayout->addWidget(browseInputButton);

        outputChooserLayout = new QHBoxLayout();
        QLabel *labelOutputFile = new QLabel("Output Directory:", this);
        outputPathChooser = new QLineEdit(this);
        QPushButton *browseOutputButton = new QPushButton("Browse...", this);
        outputChooserLayout->addWidget(labelOutputFile);
        outputChooserLayout->addWidget(outputPathChooser);
        outputChooserLayout->addWidget(browseOutputButton);

        optionsLayout = new QHBoxLayout();
        QLabel *labelHashFunction = new QLabel("Hash Function:", this);
        hashComboBox = new QComboBox(this);
        QLabel *labelSegmentSize = new QLabel("Segment Size:", this);
        segmentSizeComboBox = new QComboBox(this);
        optionsLayout->addWidget(labelHashFunction);
        optionsLayout->addWidget(hashComboBox);
        optionsLayout->addWidget(labelSegmentSize);
        optionsLayout->addWidget(segmentSizeComboBox);

        buttonLayout = new QHBoxLayout();
        importButton = new QPushButton("Import", this);
        exportButton = new QPushButton("Export", this);
        deleteButton = new QPushButton("Delete", this);
        settingsButton = new QPushButton("Settings", this);
        buttonLayout->addWidget(importButton);
        buttonLayout->addWidget(exportButton);
        buttonLayout->addWidget(deleteButton);
        buttonLayout->addWidget(settingsButton);

        dataTable = new QTableWidget(this);
        dataTable->setColumnCount(3);
        QStringList headers;
        headers << "File" << "Hash" << "Size";
        dataTable->setHorizontalHeaderLabels(headers);

        logTextField = new QTextEdit(this);
        logTextField->setReadOnly(true);

        mainLayout->addLayout(fileChooserLayout);
        mainLayout->addLayout(outputChooserLayout);
        mainLayout->addLayout(optionsLayout);
        mainLayout->addLayout(buttonLayout);
        mainLayout->addWidget(dataTable);
        mainLayout->addWidget(logTextField);
    }

    void MainWindow::onImport() {
        QString inputPath = inputPathChooser->text();
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
        QString outputPath = outputPathChooser->text();
        QString fromPath = inputPathChooser->text();
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
        QString path = inputPathChooser->text();
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
}
