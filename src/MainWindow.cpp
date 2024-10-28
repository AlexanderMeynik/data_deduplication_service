#include "MainWindow.h"

#include <QMenuBar>
#include <QFileInfo>
#include <QSqlQueryModel>
#include <QMessageBox>


std::initializer_list<QString> ll = {"2", "4", "8", "16", "32","64","128","256","512","1024","2048","4096"};//todo remake

namespace windows {


    MainWindow::MainWindow(QWidget *parent)
            : QMainWindow(parent) {
        this->setFixedSize(1024, 620);

        fileService = file_services::FileParsingService();


        setupUI();
        updateStylesheet();


        myViewModel = new MainTableModel(this);
        proxyModel = new MySortFilterProxyModel(this);
        nNullProxyModel = new NotNullFilterProxyModel(this);
        proxyModel->setSourceModel(myViewModel);
        nNullProxyModel->setSourceModel(proxyModel);

        dataTableView->setModel(nNullProxyModel);
        dataTableView->setSelectionMode(QAbstractItemView::SingleSelection);
        dataTableView->setSelectionBehavior(QAbstractItemView::SelectRows);

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
        connect(fileExportLE, &QLineEdit::textChanged, this, &MainWindow::activateButtonsd);
        connect(this, &MainWindow::connectionChanged, this, &MainWindow::activateButtonsd);

        connect(connectPB, &QPushButton::pressed, this, &MainWindow::onloadDatabase);

        connect(dropPB, &QPushButton::pressed, [&] {
            fileService.dbDrop(c_str.getDbname());
            dbConnection = false;
            emit onConnectionChanged(true);
        });

        connect(this, &MainWindow::connectionChanged, this, &MainWindow::onConnectionChanged);


        connect(this, &MainWindow::connectionChanged, [&]() {
            qled->setChecked(dbConnection);
        });

        connect(this, &MainWindow::modelUpdate, this, &MainWindow::updateModel);

        connect(dataseLE, &QLineEdit::textChanged, [&]() {
            connectPB->setEnabled(!dataseLE->text().isEmpty());
        });

        connect(fileExportLE, &QLineEdit::textChanged, [&]
                (const QString &a) {

            if (a.isEmpty()) {
                proxyModel->setFilterRegularExpression(QString());
                return;
            }
            proxyModel->setFilterRegularExpression(a);
            dataTableView->selectionModel()->clearSelection();
        });

        connect(dataTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
                [&](const QItemSelection &selected,
                    const QItemSelection &deselected) {
                    writeLog(QString::number(selected.size()));

                    if (!selected.indexes().isEmpty()) {
                        QModelIndex index = selected.indexes().first();
                        int row = index.row();
                        int columnCount = dataTableView->model()->columnCount();

                        updateLEDS(index);
                    } else {
                        resetLeds();

                    }
                });


        for (const char *hashName: hash_utils::hash_function_name) {
            hashFunctionCoB->addItem(hashName);
        }


        connect(timer, &QTimer::timeout, this, [&]() {
            int value = (progressBar->value() + 5) % 100;
            progressBar->setValue(value);
        });

        connect(statusBar(), &QStatusBar::messageChanged, [&](const QString &text) {
            if (text.isNull()) {
                progressBar->hide();
                lb->hide();
            } else {
                progressBar->setValue(0);
                lb->show();
                progressBar->show();
            }
        });



        segmentSizeCoB->addItems(ll);

        dbConnection = false;

        readConfiguration();



    }

    void MainWindow::updateStylesheet() {
        QFile styleFile("../../resources/styleshhets/widget.qss" );
        styleFile.open( QFile::ReadOnly );
        auto style=QString::fromLatin1( styleFile.readAll());
        setStyleSheet(style);
        styleFile.close();
    }

    MainWindow::~MainWindow() {
    }

    void MainWindow::setupUI() {
        setWindowTitle("File Processing Application");

        setCentralWidget(new QWidget());

        auto cent = centralWidget();


        settingsAction = new QAction("Settings");
        menuBar()->addAction(settingsAction);

        inputFileLEWO = new FileLineEditWithOption(cent, QDir::currentPath());
        labelHashFunction = new QLabel("Hash Function:");
        labelSegmentSize = new QLabel("Segment Size:");
        hashFunctionCoB = new QComboBox(cent);
        segmentSizeCoB = new QComboBox(cent);
        importPB = new QPushButton("Import");

        importFileArea = new QGroupBox(this);
        importFileArea->setObjectName("importFileArea");
        importFileAreaLay = new QGridLayout();
        importFileArea->setLayout(importFileAreaLay);

        importFileAreaLay->addWidget(new QLabel("Entry to import:"), 0, 0, 1, 1);
        importFileAreaLay->addWidget(inputFileLEWO, 0, 1, 1, 3);
        importFileAreaLay->addWidget(labelHashFunction, 1, 0, 1, 1);
        importFileAreaLay->addWidget(hashFunctionCoB, 1, 1, 1, 1);
        importFileAreaLay->addWidget(labelSegmentSize, 1, 2, 1, 1);
        importFileAreaLay->addWidget(segmentSizeCoB, 1, 3, 1, 1);
        importFileAreaLay->addWidget(importPB, 2, 0, 1, 4);


        fileExportLE = new QLineEdit();
        exportPB = new QPushButton("Export");
        deletePB = new QPushButton("Delete");
        outputFileLEWO = new FileLineEditWithOption(cent, QDir::currentPath(), true);

        exportFileArea = new QGroupBox(this);
        exportFileArea->setObjectName("exportFileArea");
        exportFileAreaLay = new QGridLayout();
        exportFileArea->setLayout(exportFileAreaLay);

        exportFileAreaLay->addWidget(new QLabel("From entry:"), 0, 0, 1, 1);

        exportFileAreaLay->addWidget(fileExportLE, 0, 1, 1, 5);
        exportFileAreaLay->addWidget(new QLabel("To entry:"), 1, 0, 1, 1);
        exportFileAreaLay->addWidget(outputFileLEWO, 1, 1, 1, 5);
        exportFileAreaLay->addWidget(exportPB, 2, 0, 1, 3);
        exportFileAreaLay->addWidget(deletePB, 2, 3, 1, 3);


        segmentSizeLCD=new QLCDNumber();
        fileDataSizeLCD = new QLCDNumber();
        totalSizeLCD = new QLCDNumber();
        fileSegmentLCD = new QLCDNumber();
        totalRepeatedBlocksLCD = new QLCDNumber();
        dataToOriginalPercentageLCD = new QLCDNumber();
        totalRepetitionPercentageLCD = new QLCDNumber();

        importTimeLCD = new QLCDNumber();
        replaceFileCB = new QCheckBox();
        replaceFileCB->setToolTip("If checked will replace file contents");

        includeOptionsArea = new QGroupBox(this);
        includeOptionsArea->setObjectName("includeOptionsArea");
        incudeOptionLay = new QGridLayout();
        includeOptionsArea->setLayout(incudeOptionLay);

        incudeOptionLay->addWidget(new QLabel("Replace files on import"), 0, 0, 1, 1);
        incudeOptionLay->addWidget(replaceFileCB, 0, 1, 1, 1);

        incudeOptionLay->addWidget(new QLabel("Load time (ms)"), 1, 0, 1, 1);
        incudeOptionLay->addWidget(importTimeLCD, 1, 1, 1, 1);
        incudeOptionLay->addWidget(new QLabel("Segment size"), 1, 2, 1, 1);
        incudeOptionLay->addWidget(segmentSizeLCD, 1, 3, 1, 1);

        incudeOptionLay->addWidget(new QLabel("File data size"), 2, 0, 1, 1);
        incudeOptionLay->addWidget(fileDataSizeLCD, 2, 1, 1, 1);
        incudeOptionLay->addWidget(new QLabel("Total file size"), 2, 2, 1, 1);
        incudeOptionLay->addWidget(totalSizeLCD, 2, 3, 1, 1);
        incudeOptionLay->addWidget(dataToOriginalPercentageLCD, 2, 4, 1, 1);

        incudeOptionLay->addWidget(new QLabel("Files segment count"), 3, 0, 1, 1);
        incudeOptionLay->addWidget(fileSegmentLCD, 3, 1, 1, 1);
        incudeOptionLay->addWidget(new QLabel("Unique segment count"), 3, 2, 1, 1);
        incudeOptionLay->addWidget(totalRepeatedBlocksLCD, 3, 3, 1, 1);
        incudeOptionLay->addWidget(totalRepetitionPercentageLCD, 3 , 4, 1, 1);


        deleteFilesCB = new QCheckBox();
        deleteFilesCB->setToolTip("If checked entry will be deleted after export.");
        createMainCB = new QCheckBox();
        createMainCB->setToolTip("If checked new directory will be created if path to out does not exist!");
        createMainCB->setChecked(true);
        errorCountLCD = new QLCDNumber();
        exportTimeLCD = new QLCDNumber();
        deleteTimeLCD = new QLCDNumber();

        exportOptionsArea = new QGroupBox(this);
        exportOptionsArea->setObjectName("exportOptionsArea");
        exportOptionLay = new QGridLayout();
        exportOptionsArea->setLayout(exportOptionLay);

        exportOptionLay->addWidget(new QLabel("Delete entry"), 0, 0, 1, 1);
        exportOptionLay->addWidget(deleteFilesCB, 0, 1, 1, 1);

        exportOptionLay->addWidget(new QLabel("Create root directory"), 1, 0, 1, 1);
        exportOptionLay->addWidget(createMainCB, 1, 1, 1, 1);


        exportOptionLay->addWidget(new QLabel("Delete time (ms)"), 2, 0, 1, 1);
        exportOptionLay->addWidget(deleteTimeLCD, 2, 1, 1, 1);

        exportOptionLay->addWidget(new QLabel("Export time (ms)"), 3, 0, 1, 1);
        exportOptionLay->addWidget(exportTimeLCD, 3, 1, 1, 1);

        exportOptionLay->addWidget(new QLabel("Error count"), 3, 2, 1, 1);
        exportOptionLay->addWidget(errorCountLCD, 3, 3, 1, 1);

        dbUsageCB = new QCheckBox();
        connectPB = new QPushButton("Connect");
        qled = new QLedIndicator(this);

        connectPB->setEnabled(false);
        dropPB = new QPushButton("Drop database");
        dropPB->setEnabled(false);
        dataseLE = new QLineEdit();
        dataseLE = new QLineEdit();
        re = QRegularExpression("^[a-z_][A-Za-z0-9_]{1,62}");
        validator = new QRegularExpressionValidator(re, this);
        dataseLE->setValidator(validator);

        databaseConfigurationArea = new QGroupBox(tr("Database configiration"));
        databaseConfigurationArea->setObjectName("databaseConfigurationArea");
        dbOptionLay = new QGridLayout();
        databaseConfigurationArea->setLayout(dbOptionLay);

        dbOptionLay->addWidget(new QLabel("Database name:"), 0, 0, 1, 1);
        dbOptionLay->addWidget(dataseLE, 0, 1, 1, 1);
        dbOptionLay->addWidget(new QLabel("Create new database"), 0, 2, 1, 1);
        dbOptionLay->addWidget(dbUsageCB, 0, 3, 1, 1);
        dbOptionLay->addWidget(connectPB, 1, 0, 1, 1);
        dbOptionLay->addWidget(dropPB, 1, 2, 1, 1);
        dbOptionLay->addWidget(qled, 1, 3, 1, 1);
      /*  dbOptionLay->addItem(new QSpacerItem(20,20),1, 1, 1, 1);*/


        dataTableView = new DeselectableTreeView(this);
        logTextField = new QTextEdit();
        logTextField->setReadOnly(true);

        mmLayout = new QGridLayout();
        centralWidget()->setLayout(mmLayout);

        importFileAreaLay->setSpacing(10);
        exportFileAreaLay->setSpacing(10);
        incudeOptionLay->setSpacing(10);
        exportOptionLay->setSpacing(10);
        exportFileAreaLay->setSpacing(10);


        mmLayout->addWidget(databaseConfigurationArea, 0, 0, 1, 2);
        mmLayout->addWidget(importFileArea, 1, 0, 1, 1);
        mmLayout->addWidget(dataTableView, 2, 0, 1, 2);
        mmLayout->addWidget(exportFileArea, 3, 0, 2, 1);

        mmLayout->addWidget(includeOptionsArea, 1, 1, 1, 1);
        mmLayout->addWidget(exportOptionsArea, 3, 1, 1, 1);
        mmLayout->addWidget(logTextField, 5, 0, 1, 2);


        progressBar = new QProgressBar(this);
        progressBar->setRange(0, 100);
        progressBar->setTextVisible(false);

        lb = new QLabel("", this);
        QPalette p = palette();
        p.setColor(QPalette::Highlight, Qt::green);
        progressBar->setPalette(p);


        this->statusBar()->addWidget(lb, 50);
        this->statusBar()->addWidget(progressBar, 50);


        timer = new QTimer(this);

        timer->start(50);


        lb->hide();
        progressBar->hide();

        list={fileDataSizeLCD ,  segmentSizeLCD ,  totalSizeLCD ,  fileSegmentLCD ,  totalRepeatedBlocksLCD ,
              dataToOriginalPercentageLCD ,  totalRepetitionPercentageLCD ,  importTimeLCD ,  exportTimeLCD ,
              errorCountLCD,deleteTimeLCD};

        for (auto *elem: list) {
            elem->setSegmentStyle(QLCDNumber::Flat);
        }

        settingsWindow = new SettingsWindow(this);
    }

    void MainWindow::onImport() {
        QString inputPath = inputFileLEWO->getContent();

        auto ss = toShortPath(inputPath);
        statusMessage(QString("Import entry %1").arg(ss));
        bool isDir = inputFileLEWO->getType() == Directory;

        /*size_t segmentSize = segmentSizeCoB->currentText().toUInt();*/
        if (isDir) {
            writeLog("process directory");
            //fileService.processDirectory(inputPath.toStdString(), segmentSize);
        } else {
            writeLog("process file");
        }

        emit modelUpdate();
/*        statusMessage(QString("Import entry %1").arg(toShortPath(inputPath)));*/
    }

    void MainWindow::onExport() {
        QString outputPath = outputFileLEWO->getContent();
        QString exportPath = fileExportLE->text();

        statusMessage(QString("Exporting entry %1 to %2").arg(exportPath).arg(outputPath));


        bool main = createMainCB->isChecked();
        bool remove = deleteFilesCB->isChecked();
        bool isDir = inputFileLEWO->getType() == Directory;

        writeLog("entry load");
        //todo process
        if (deleteFilesCB->isChecked()) {
            emit modelUpdate();
        }

        cleadStBar();
    }

    void MainWindow::onDelete() {


        QString path = inputFileLEWO->getContent();
        statusMessage(QString("Deleting entry %1").arg(path));
        bool isDir = inputFileLEWO->getType() == Directory;

        if (isDir) {
            writeLog("delete directory");

            /*fileService.deleteDirectory(path.toStdString());*/
        } else {
            writeLog("delete file");
            /*  fileService.deleteFile(path.toStdString());*/
        }
        emit modelUpdate();
        cleadStBar();
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
        bool old = dbConnection;
        dbConnection = (stat == QDialog::Accepted);
        emit connectionChanged(old);
    }

    void MainWindow::activateButtonsd() {


        auto in = inputFileLEWO->getContent();
        auto out = outputFileLEWO->getContent();
        auto toFile = fileExportLE->text();

        auto b1 = isDirName(toFile);
        auto b2 = outputFileLEWO->getType() == Directory;
        bool both = b1 == b2;
        bool isValid = proxyModel->rowCount() && !toFile.isEmpty();


        importPB->setEnabled(dbConnection && !inputFileLEWO->isEmpty());
        exportPB->setEnabled(dbConnection && !outputFileLEWO->isEmpty() && isValid && both);
        deletePB->setEnabled(dbConnection && isValid);
    }


    void MainWindow::onloadDatabase() {
        statusMessage("Connecting to database");
        this->c_str.setDbname(dataseLE->text().toStdString());
        bool old = dbConnection;
        dbConnection = checkConnString(c_str);
        emit connectionChanged(old);


        if (dbUsageCB->isChecked()) {
            if (dbConnection) {
                writeLog("database aleady exists");
                return;
            }
            fileService.dbLoad<file_services::create>(c_str.getDbname());


        } else {
            fileService.dbLoad(c_str.getDbname());

        }
        emit connectionChanged(dbConnection);
        cleadStBar();
    }


    void MainWindow::readConfiguration() {
        QFile file(confName);
        file.open(QIODevice::ReadOnly);
        if (!file.isOpen()) {
            writeLog("No file found", ERROR);
            QMessageBox msgBox;
            msgBox.setWindowTitle("Warning");
            msgBox.setText("Database configuration not found!");
            msgBox.setText("Database configuration is empty would you like to create one using settings?");
            msgBox.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
            auto button = msgBox.exec();
            switch (button) {
                case QMessageBox::Cancel:
                    abort();
                    break;
                case QMessageBox::Ok:
                    onSettings();
                    break;
            }
            return;
        }
        QXmlStreamReader xmlReader;
        xmlReader.setDevice(&file);
        xmlReader.readNext();
        bool old;
        while (!xmlReader.atEnd()) {
            if (xmlReader.isStartElement()) {
                std::string path;
                if (xmlReader.name() == parentTag) {
                    for (auto &attr: xmlReader.attributes()) {
                        if (attr.name().toString() == "path") {
                            path = attr.value().toString().toStdString();
                            this->c_str = db_services::loadConfiguration(path);
                            dataseLE->setText(QString::fromStdString(c_str.getDbname()));


                            old = dbConnection;
                            dbConnection = checkConnString(c_str);
                            break;
                        }
                    }
                }
            }
            xmlReader.readNext();
        }

        emit connectionChanged(old);
        file.close();
    }

    void MainWindow::onConnectionChanged(bool old) {
        statusMessage("Connection change");
        if (!dbConnection) {
            reset:dataTableView->selectionModel()->clearSelection();
            if (dataTableView->model()) {
                fileService.disconnect();
                myViewModel->reset();
            }
        } else {
            auto dbName = dataseLE->text().toStdString();

            bool conn = myViewModel->performConnection(c_str);

            if (!conn) {
                goto reset;
            }
            myViewModel->getData();
            dataTableView->sizeHint();
            dataTableView->setSortingEnabled(true);
            dataTableView->sortByColumn(-1, Qt::SortOrder::DescendingOrder);
            dataTableView->resizeColumnsToContents();
        }


        cleadStBar();

    }

    void MainWindow::resizeEvent(QResizeEvent *event) {
        QWidget::resizeEvent(event);
        dataTableView->sizeHint();
        writeLog("resized");
    }

    void MainWindow::updateModel() {
        statusMessage("Updating model");
        myViewModel->getData();
        cleadStBar();
    }

    void MainWindow::updateLEDS(QModelIndex &idx) {
        updateStylesheet();
        //todo when connection/model resets leds will also reset
        int row = idx.row();


        fileDataSizeLCD->setDigitCount(8);
        totalSizeLCD->setDigitCount(8);
        totalRepetitionPercentageLCD->setDigitCount(5);
        dataToOriginalPercentageLCD->setDigitCount(5);
        this->fileSegmentLCD->setDigitCount(8);
        this->totalRepeatedBlocksLCD->setDigitCount(8);
        deleteTimeLCD->setDigitCount(5);

        segmentSizeLCD->display(idx.sibling(row, 1).data().toInt());
        fileDataSizeLCD->display(idx.sibling(row, 2).data().toInt());
        totalSizeLCD->display(idx.sibling(row, 3).data().toInt());
        dataToOriginalPercentageLCD->display(smartCeil(idx.sibling(row, 4).data().toDouble(), 2));
        fileSegmentLCD->display(idx.sibling(row, 5).data().toInt());
        totalRepeatedBlocksLCD->display(idx.sibling(row, 6).data().toInt());
        totalRepetitionPercentageLCD->display(smartCeil(idx.sibling(row, 7).data().toDouble(), 2));
        importTimeLCD->display(smartCeil(idx.sibling(row, 8).data().toDouble(), 2));
    }

    void MainWindow::resetLeds() {

        for (auto * l:list) {
            l->display(0);
        }
    }

    QString MainWindow::toShortPath(const QString &qString) {
        return QString::fromStdString(
                std::filesystem::path(qString.toStdString()).lexically_relative(fs::current_path()).string());
    }
}
