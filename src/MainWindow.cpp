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

        myViewModel = new MainTableModel(this);
        proxyModel = new MySortFilterProxyModel(this);
        nNullProxyModel=new NotNullFilterProxyModel(this);
        proxyModel->setSourceModel(myViewModel);
        nNullProxyModel->setSourceModel(proxyModel);
        treeView->setModel(nNullProxyModel);

        QCompleter *completer = new QCompleter(this);
        completer->setModel(proxyModel);
        completer->setCompletionColumn(0);
        completer->setCompletionRole(Qt::DisplayRole);

        fileExportLE->setCompleter(completer);

        connect(importPB, &QPushButton::pressed, this, &MainWindow::onImport);
        connect(exportPB, &QPushButton::pressed, this, &MainWindow::onExport);
        connect(deletePB, &QPushButton::pressed, this, &MainWindow::onDelete);


        connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettings);


        connect(inputFileLEWO, &FileLineEditWithOption::contentChanged, this, &MainWindow::activateButtonsd);
        connect(outputFileLEWO, &FileLineEditWithOption::contentChanged, this, &MainWindow::activateButtonsd);


        connect(loadPB, &QPushButton::pressed, this, &MainWindow::onloadDatabase);

        connect(dropPB,&QPushButton::pressed,[&]
        {
            fileService.dbDrop(c_str.getDbname());
            dbConnection= false;
            emit onConnectionChanged(true);
        });

        connect(this, &MainWindow::connectionChanged, this, &MainWindow::onConnectionChanged);


        connect(dataseLE, &QLineEdit::textChanged, [&]() {
            loadPB->setEnabled(!dataseLE->text().isEmpty());
        });

        connect(fileExportLE, &QLineEdit::textChanged, [&]
                (const QString &a) {

            if (a.isEmpty()) {
                proxyModel->setFilterRegularExpression(QString());
                return;
            }
            proxyModel->setFilterRegularExpression(a);

        });

        connect(treeView->selectionModel(), &QItemSelectionModel::selectionChanged, [&](const QItemSelection &selected,
                                                                                        const QItemSelection &deselected) {
            writeLog(QString::number(selected.size()));

            if (!selected.indexes().isEmpty()) {
                QModelIndex index = selected.indexes().first();
                int row = index.row();
                int columnCount = treeView->model()->columnCount();

                QString rowData = "Selected row: " + QString::number(row) + " | Data: ";

                for (int col = 0; col < columnCount; ++col) {
                    QVariant data = index.sibling(row, col).data();
                    rowData += data.toString() + " ";
                }
                updateLEDS(index);

                writeLog(rowData);
            } else {
                resetLeds();

            }
        });


        for (const char *hashName: hash_utils::hash_function_name) {
            hashFunctionCoB->addItem(hashName);
        }


        segmentSizeCoB->addItems(ll);

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

        mmLayout=new QGridLayout();

        mainLayout = new QVBoxLayout();



        settingsAction = new QAction("Settings");
        menuBar()->addAction(settingsAction);


        //todo add database status(total blocks files size) files
        //todo for import and export add fileds for segment count etc.
        //todo add connection qled


        inputFileLEWO = new FileLineEditWithOption(cent, QDir::currentPath());
        labelHashFunction = new QLabel("Hash Function:");
        labelSegmentSize = new QLabel("Segment Size:");
        hashFunctionCoB = new QComboBox(cent);
        segmentSizeCoB = new QComboBox(cent);
        importPB = new QPushButton("Import");

        importFileArea=new QGroupBox(this);
        importFileAreaLay=new QGridLayout();
        importFileArea->setLayout(importFileAreaLay);

        importFileAreaLay->addWidget(inputFileLEWO,0,0,1,4);
        importFileAreaLay->addWidget(labelHashFunction,1,0,1,1);
        importFileAreaLay->addWidget(hashFunctionCoB,1,1,1,1);
        importFileAreaLay->addWidget(labelSegmentSize,1,2,1,1);
        importFileAreaLay->addWidget(segmentSizeCoB,1,3,1,1);
        importFileAreaLay->addWidget(importPB,2,0,1,4);


        fileExportLE = new QLineEdit();
        exportPB = new QPushButton("Export");
        deletePB = new QPushButton("Delete");
        outputFileLEWO = new FileLineEditWithOption(cent, QDir::currentPath(), true);

        exportFileArea=new QGroupBox(this);
        exportFileAreaLay=new QGridLayout();
        exportFileArea->setLayout(exportFileAreaLay);

        exportFileAreaLay->addWidget(outputFileLEWO,0,0,1,2);
        exportFileAreaLay->addWidget(fileExportLE,1,0,1,2);
        exportFileAreaLay->addWidget(exportPB,2,0,1,1);
        exportFileAreaLay->addWidget(deletePB,2,1,1,1);


        treeView = new DeselectableTreeView(this);

        mainLayout->addWidget(importFileArea);
        mainLayout->addWidget(treeView);
        mainLayout->addWidget(exportFileArea);





        logTextField = new QTextEdit();
        logTextField->setReadOnly(true);







        mmLayout->addLayout(mainLayout,0,0,1,1);


        auto VV = new QVBoxLayout();


        includeOptionsArea = new QGroupBox(tr("FileImportOptions"));
        replaceFileCB = new QCheckBox();
        replaceFileCB->setToolTip("If checked will replace file contents");
        auto incudeOptionLay = new QGridLayout();

        numberBlocksLCD = new QLCDNumber();
        totalSizeLCD = new QLCDNumber();
        fileSegmentLCD = new QLCDNumber();
        totalRepeatedBlocksLCD = new QLCDNumber();
        totalRepetitionPercentageLCD = new QLCDNumber();
        importTimeLCD = new QLCDNumber();

        incudeOptionLay->addWidget(new QLabel("Replace files on import"), 0, 0, 1, 2);
        incudeOptionLay->addWidget(replaceFileCB, 0, 2, 1, 1);


        incudeOptionLay->addWidget(new QLabel("Load time (ms)"),
                                   0, 3, 1, 1);
        incudeOptionLay->addWidget(importTimeLCD,
                                   0, 4, 1, 2);


        incudeOptionLay->addWidget(new QLabel("Total file blocks"), 1, 0, 1, 1);
        incudeOptionLay->addWidget(numberBlocksLCD, 1, 1, 1, 1);
        incudeOptionLay->addWidget(new QLabel("Total file size"), 1, 2, 1, 1);
        incudeOptionLay->addWidget(totalSizeLCD, 1, 3, 1, 3);


        incudeOptionLay->addWidget(new QLabel("Files segment count"), 2, 0, 1, 1);
        incudeOptionLay->addWidget(fileSegmentLCD, 2, 1, 1, 1);

        incudeOptionLay->addWidget(new QLabel("Duplicated blocks count"), 2, 2, 1, 1);
        incudeOptionLay->addWidget(totalRepeatedBlocksLCD, 2, 3, 1, 1);

        incudeOptionLay->addWidget(totalRepetitionPercentageLCD, 2, 4, 1, 1);
        incudeOptionLay->addWidget(new QLabel("%"), 2, 5, 1, 1);

        includeOptionsArea->setLayout(incudeOptionLay);


        exportOptionsArea = new QGroupBox(tr("FileExportOptions"));

        deleteFilesCB = new QCheckBox();
        deleteFilesCB->setToolTip("If checked will delete file/directory after export.");
        createMainCB = new QCheckBox();
        createMainCB->setToolTip("If checked  new directory will be created if out path does not exist!");

        errorCountLCD = new QLCDNumber();
        exportTimeLCD = new QLCDNumber();


        auto exportOptionLay = new QGridLayout();
        exportOptionLay->addWidget(new QLabel("Delete file/directory"), 0, 0, 1, 1);
        exportOptionLay->addWidget(deleteFilesCB, 0, 1, 1, 1);
        exportOptionLay->addWidget(new QLabel("Create root directory"), 0, 2, 1, 1);
        exportOptionLay->addWidget(createMainCB, 0, 3, 1, 1);


        exportOptionLay->addWidget(new QLabel("Export time (ms)"),
                                   1, 0, 1, 1);
        exportOptionLay->addWidget(exportTimeLCD,
                                   1, 1, 1, 1);
        exportOptionLay->addWidget(new QLabel("Error count"),
                                   1, 2, 1, 1);
        exportOptionLay->addWidget(errorCountLCD,
                                   1, 3, 1, 1);

        createMainCB->setChecked(true);

        exportOptionsArea->setLayout(exportOptionLay);

        databaseConfigurationArea = new QGroupBox(tr("Database configiration"));
        auto dbOptionLay = new QGridLayout();

        dbUsageCB = new QCheckBox();
        loadPB = new QPushButton("Connect");
        loadPB->setEnabled(false);
        dropPB=new QPushButton("Drop database");
        dropPB->setEnabled(false);

        dataseLE = new QLineEdit();

        dataseLE = new QLineEdit();
        QRegularExpression re("^[a-z_][A-Za-z0-9_]{1,62}");
        QRegularExpressionValidator *validator = new QRegularExpressionValidator(re, this);
        dataseLE->setValidator(validator);

        dbOptionLay->addWidget(new QLabel("Database name:"), 0, 0, 1, 1);
        dbOptionLay->addWidget(dataseLE, 0, 1, 1, 1);

        dbOptionLay->addWidget(new QLabel("Create new database"), 1, 0, 1, 1);
        dbOptionLay->addWidget(dbUsageCB, 1, 1, 1, 1);
        dbOptionLay->addWidget(loadPB, 1, 2, 1, 1);
        dbOptionLay->addWidget(dropPB,1,3,1,1);


        databaseConfigurationArea->setLayout(dbOptionLay);


        VV->addWidget(includeOptionsArea);
        VV->addWidget(databaseConfigurationArea);
        VV->addWidget(logTextField);
        VV->addWidget(exportOptionsArea);


        mmLayout->addLayout(VV,0,1,1,1);

        centralWidget()->setLayout(mmLayout);

        settingsWindow = new SettingsWindow(this);
    }

    void MainWindow::onImport() {
        this->updateModel(0);
        QString inputPath = inputFileLEWO->getContent();
        size_t segmentSize = segmentSizeCoB->currentText().toUInt();
        if (QFileInfo(inputPath).isDir()) {
            writeLog("process directory");
            //fileService.processDirectory(inputPath.toStdString(), segmentSize);
        } else {
            writeLog("process file");
            //fileService.processFile(inputPath.toStdString(), segmentSize);
        }
    }

    void MainWindow::onExport() {
        QString outputPath = outputFileLEWO->getContent();
        QString fromPath = inputFileLEWO->getContent();
        if (QFileInfo(fromPath).isDir()) {
            writeLog("load directory");
            //fileService.loadDirectory(fromPath.toStdString(), outputPath.toStdString());
        } else {
            writeLog("load file");
            // fileService.loadFile(fromPath.toStdString(), outputPath.toStdString());
        }
    }

    void MainWindow::onDelete() {
        myViewModel->reset();
        QString path = inputFileLEWO->getContent();
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
        } else {
            writeLog("Rejected connection", ERROR);
        }
        settingsWindow->close();
        bool old = dbConnection;
        dbConnection = (stat == QDialog::Accepted);
        emit connectionChanged(old);
    }

    void MainWindow::activateButtonsd() {


        auto in = inputFileLEWO->getContent();
        auto out = outputFileLEWO->getContent();

        bool cond = QFileInfo(in).isDir() ==
                    QFileInfo(out).isDir();

        /*importButton->setEnabled(dbConnection&&!in.isEmpty());
        exportButton->setEnabled(dbConnection&&!in.isEmpty() && !out.isEmpty()
                                 && cond);
        deleteButton->setEnabled(dbConnection&&!in.isEmpty());*/

        importPB->setEnabled(dbConnection);
        exportPB->setEnabled(dbConnection);
        deletePB->setEnabled(dbConnection);
    }


    void MainWindow::onloadDatabase() {
        this->c_str.setDbname(dataseLE->text().toStdString());
        bool old = dbConnection;
        dbConnection = checkConnString(c_str);
        emit connectionChanged(old);


        if (dbUsageCB->isChecked()) {
            if (dbConnection) {
                writeLog("database aleady exists");
                return;
            }
            fileService.dbLoad<file_services::create>(std::string_view{c_str.getDbname()});
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
                            this->c_str = db_services::loadConfiguration(path);
                            dataseLE->setText(QString::fromStdString(c_str.getDbname()));


                            bool old = dbConnection;
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
        if (!dbConnection) {
            if (treeView->model()) {
                fileService.disconnect();
                myViewModel->reset();
            }
        } else {
            auto dbName = dataseLE->text().toStdString();

            auto r1 = fileService.dbLoad(dbName);
            bool result = myViewModel->performConnection(c_str);

            if (!result || r1 != 0) {
                return;
            }


            myViewModel->getData();
            treeView->sizeHint();
            treeView->setSortingEnabled(true);
            treeView->sortByColumn(-1, Qt::SortOrder::DescendingOrder);
            /*dataTable->resizeColumnToContents(0);*/
            treeView->setWordWrap(true);
            /*dataTable->resizeColumnsToContents();*/
        }

        activateButtonsd();

    }

    void MainWindow::resizeEvent(QResizeEvent *event) {
        QWidget::resizeEvent(event);

        treeView->sizeHint();
        writeLog("resized");
    }

    void MainWindow::updateModel(size_t index) {

        myViewModel->getData();
    }

    void MainWindow::updateLEDS(QModelIndex &idx) {
        //todo when connection/model resets leds will also reset
        int row = idx.row();
        this->setStyleSheet("QLCDNumber { background-color: white; color: black; }");
        totalSizeLCD->setDigitCount(8);
        totalSizeLCD->display(idx.sibling(row, 2).data().toInt());

        totalRepetitionPercentageLCD->setDigitCount(5);
        totalRepetitionPercentageLCD->setSegmentStyle(QLCDNumber::Filled);
        totalRepetitionPercentageLCD->display(smartCeil(idx.sibling(row, 3).data().toDouble(), 2));

    }

    void MainWindow::resetLeds() {
        totalSizeLCD->display(0);
        totalRepetitionPercentageLCD->display(0);
        this->setStyleSheet("");
    }
}
