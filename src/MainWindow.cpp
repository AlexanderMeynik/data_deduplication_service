#include "MainWindow.h"

#include <QMenuBar>
#include <QFileInfo>
#include <QSqlQueryModel>


std::initializer_list<QString> ll = {"2", "4", "8", "16", "32"};//todo remake

namespace windows {


    MainWindow::MainWindow(QWidget *parent)
            : QMainWindow(parent) {
        this->setFixedSize(1024, 620);

        fileService = file_services::FileParsingService();


        setupUI();

        mydmod = new MainTableModel(this);
        proxyModel = new MySortFilterProxyModel(this);
        proxyModel->setSourceModel(mydmod);
        dataTable->setModel(proxyModel);

        QCompleter* completer = new QCompleter( this );//todo check
        completer->setModel( proxyModel );
        completer->setCompletionColumn(0);
        completer->setCompletionRole(Qt::DisplayRole);

        fileExportField->setCompleter(completer);

        connect(importButton, &QPushButton::pressed, this, &MainWindow::onImport);
        connect(exportButton, &QPushButton::pressed, this, &MainWindow::onExport);
        connect(deleteButton, &QPushButton::pressed, this, &MainWindow::onDelete);


        connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettings);


        connect(inputFile, &FileLineEditWithOption::contentChanged, this, &MainWindow::activateButtonsd);
        connect(outputFile, &FileLineEditWithOption::contentChanged, this, &MainWindow::activateButtonsd);


        connect(loadDb, &QPushButton::pressed, this, &MainWindow::onloadDatabase);

        connect(this, &MainWindow::connectionChanged,this
        ,&MainWindow::onConnectionChanged);


        connect(dataseName, &QLineEdit::textChanged, [&]() {
            loadDb->setEnabled(!dataseName->text().isEmpty());
        });

        connect(fileExportField,&QLineEdit::textChanged,[&]
                (const QString &a)
        {

            if(dynamic_cast<decltype(this->mydmod)>(proxyModel->sourceModel())->isEmpty())
            {
                return ;
            }
            if(a.isEmpty())
            {
                proxyModel->setFilterRegularExpression(QString());
                return ;
            }
            proxyModel->setFilterRegularExpression(a);

        });

        connect(dataTable->selectionModel(),&QItemSelectionModel::selectionChanged,[&](const QItemSelection &selected,
                const QItemSelection &deselected)
        {
            writeLog(QString::number(selected.size()));

            if (!selected.indexes().isEmpty()) {
                QModelIndex index = selected.indexes().first();
                int row = index.row();
                int columnCount = dataTable->model()->columnCount();

                QString rowData = "Selected row: " + QString::number(row) + " | Data: ";

                for (int col = 0; col < columnCount; ++col) {
                    // Get the data for each column in the selected row
                    QVariant data = index.sibling(row, col).data();
                    rowData += data.toString() + " ";
                }
                updateLEDS(index);

                writeLog(rowData);
            } else
            {
                resetLeds();

            }
        });



        for (const char *hashName: hash_utils::hash_function_name) {
            hashComboBox->addItem(hashName);
        }


        segmentSizeComboBox->addItems(ll);

        dbConnection = false;
        readConfiguration();
        activateButtonsd();

    }

    MainWindow::~MainWindow() {
    }

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


        fileExportField = new QLineEdit();


        hbl->addWidget(exportButton);
        hbl->addWidget(deleteButton);

        //todo add database status(total blocks files size) files
        //todo for import and export add fileds for segment count etc.
        //todo add connection qled



        dataTable = new DeselectableTreeView(this);

        /*dataTable->resizeColumnToContents(0);*/

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


        exportOptionsArea = new QGroupBox(tr("FileExportOptions"));

        deleteFiles = new QCheckBox();
        deleteFiles->setToolTip("If checked will delete file/directory after export.");
        createMain = new QCheckBox();
        createMain->setToolTip("If checked  new directory will be created if out path does not exist!");

        errorCount = new QLCDNumber();
        /*errorCount->setDigitCount(10);*/
        exportTime = new QLCDNumber();


        auto exportOptionLay = new QGridLayout();
        exportOptionLay->addWidget(new QLabel("Delete file/directory"), 0, 0, 1, 1);
        exportOptionLay->addWidget(deleteFiles, 0, 1, 1, 1);
        exportOptionLay->addWidget(new QLabel("Create root directory"), 0, 2, 1, 1);
        exportOptionLay->addWidget(createMain, 0, 3, 1, 1);


        exportOptionLay->addWidget(new QLabel("Export time (ms)"),
                                   1, 0, 1, 1);
        exportOptionLay->addWidget(exportTime,
                                   1, 1, 1, 1);
        exportOptionLay->addWidget(new QLabel("Error count"),
                                   1, 2, 1, 1);
        exportOptionLay->addWidget(errorCount,
                                   1, 3, 1, 1);

        createMain->setChecked(true);

        exportOptionsArea->setLayout(exportOptionLay);

        databaseConfiguration = new QGroupBox(tr("Database configiration"));
        auto dbOptionLay = new QGridLayout();

        createNewDbCB = new QCheckBox();
        loadDb = new QPushButton("Connect");
        loadDb->setEnabled(false);
        dataseName = new QLineEdit();

        dataseName = new QLineEdit();
        QRegularExpression re("^[a-z_][A-Za-z0-9_]{1,62}");
        QRegularExpressionValidator *validator = new QRegularExpressionValidator(re, this);
        dataseName->setValidator(validator);

        dbOptionLay->addWidget(new QLabel("Database name:"), 0, 0, 1, 1);
        dbOptionLay->addWidget(dataseName, 0, 1, 1, 1);

        dbOptionLay->addWidget(new QLabel("Create new database"), 1, 0, 1, 1);
        dbOptionLay->addWidget(createNewDbCB, 1, 1, 1, 1);
        dbOptionLay->addWidget(loadDb, 1, 2, 1, 1);


        databaseConfiguration->setLayout(dbOptionLay);


        VV->addWidget(includeOptionsArea);
        VV->addWidget(databaseConfiguration);
        VV->addWidget(logTextField);
        VV->addWidget(exportOptionsArea);


        mmm->addLayout(VV);

        centralWidget()->setLayout(mmm);

        settingsWindow = new SettingsWindow(this);
    }

    void MainWindow::onImport() {
        this->updateModel(0);
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
        mydmod->reset();
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
        auto stat = settingsWindow->exec();

        if (stat == QDialog::Accepted) {
            c_str = std::move(settingsWindow->getConfiguration());
            writeLog("update settings");
            writeLog(c_str.c_str());
        }
        else
        {
            writeLog("Rejected connection", ERROR);
        }
        settingsWindow->close();
        bool old =dbConnection;
        dbConnection = (stat == QDialog::Accepted);
        emit connectionChanged(old);
    }

    void MainWindow::activateButtonsd() {


        auto in = inputFile->getContent();
        auto out = outputFile->getContent();

        bool cond = QFileInfo(in).isDir() ==
                    QFileInfo(out).isDir();

        /*importButton->setEnabled(dbConnection&&!in.isEmpty());
        exportButton->setEnabled(dbConnection&&!in.isEmpty() && !out.isEmpty()
                                 && cond);
        deleteButton->setEnabled(dbConnection&&!in.isEmpty());*/

        importButton->setEnabled(dbConnection);
        exportButton->setEnabled(dbConnection);
        deleteButton->setEnabled(dbConnection);
    }


    void MainWindow::onloadDatabase() {
        this->c_str.setDbname(dataseName->text().toStdString());
        bool old =dbConnection;
        dbConnection = checkConnString(c_str);
        emit connectionChanged(old);


        if (createNewDbCB->isChecked()) {
            if (dbConnection) {
                writeLog("database aleady exists");
                return;
            }
            //todo create database
        } else {
            //todo dbload
        }

    }






    void MainWindow::readConfiguration() {
        QFile file(confName);
        file.open(QIODevice::ReadOnly);
        if (!file.isOpen()) {
            writeLog("No file found", ERROR);
            //todo set connection pop up(or open settings)
            return;
        }
        QXmlStreamReader xmlReader;
        xmlReader.setDevice(&file);
        xmlReader.readNext();
        while (!xmlReader.atEnd()) {
            if (xmlReader.isStartElement()) {
                std::string path;
                if (xmlReader.name() == parentTag) {
                    for (auto &attr: xmlReader.attributes()) {
                        if (attr.name().toString() == "path") {
                            path = attr.value().toString().toStdString();
                            this->c_str=db_services::loadConfiguration(path);
                            dataseName->setText(QString::fromStdString(c_str.getDbname()));


                            bool old =dbConnection;
                            dbConnection = checkConnString(c_str);
                            emit connectionChanged(old);
                            break;
                        }
                    }
                }
            }
            xmlReader.readNext();
        }
        file.close();
    }

    void MainWindow::onConnectionChanged(bool old) {
        if(!dbConnection)
        {
            if(dataTable->model())
            {
                fileService.disconnect();
                mydmod->reset();
                /*dataTable->setModel(nullptr);*/
            }
        }
        else
        {
            auto dbName = dataseName->text().toStdString();

            auto r1=fileService.dbLoad(dbName);
            bool result=mydmod->performConnection(c_str);

            if(!result||r1!=0)
            {
                return;
            }


            mydmod->getData();
            dataTable->sizeHint();
            dataTable->setSortingEnabled(true);
            dataTable->sortByColumn(-1,Qt::SortOrder::DescendingOrder);
            /*dataTable->resizeColumnToContents(0);*/
            dataTable->setWordWrap(true);
            /*dataTable->resizeColumnsToContents();*/
        }

        activateButtonsd();

    }

    void MainWindow::resizeEvent(QResizeEvent *event) {
        QWidget::resizeEvent(event);

        dataTable->sizeHint();
        writeLog("resized");
    }

    void MainWindow::updateModel(size_t index) {

        mydmod->getData();
    }

    void MainWindow::updateLEDS(QModelIndex &idx) {
        //todo when connection/model resets leds will also reset
        int row=idx.row();
        this->setStyleSheet("QLCDNumber { background-color: white; color: black; }");
        totalSize->setDigitCount(8);
        totalSize->display(idx.sibling(row, 2).data().toInt());

        totalRepeatedpercentage->setDigitCount(5);
        totalRepeatedpercentage->setSegmentStyle(QLCDNumber::Filled);
        totalRepeatedpercentage->display(smartCeil(idx.sibling(row, 3).data().toDouble(),2));

    }

    void MainWindow::resetLeds() {
        totalSize->display(0);
        totalRepeatedpercentage->display(0);
        this->setStyleSheet("");
    }
}
