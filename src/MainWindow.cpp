#include "MainWindow.h"

#include <QMenuBar>
#include <QFileInfo>
#include <QSqlQueryModel>
#include <QMessageBox>
#include <QElapsedTimer>


std::initializer_list <QString> ll = {"2", "4", "8", "16", "32", "64", "128", "256", "512", "1024", "2048",
                                      "4096"};

namespace windows {


    MainWindow::MainWindow(QWidget *parent)
            : QMainWindow(parent) {
        this->setFixedSize(1280, 720);
        /*this->fu*/

        fileService = file_services::FileService();


        setupUI();
        updateStylesheet();


        myViewModel = new DeduplicationCharacteristicsModel(this);
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
            writeLog(QString("Drop database %1").arg(dataseNameLE->text()));
            fileService.dbDrop(dataseNameLE->text().toStdString());
            dbConnection = false;
            emit onConnectionChanged(true);
        });

        connect(this, &MainWindow::connectionChanged, this, &MainWindow::onConnectionChanged);


        connect(this, &MainWindow::modelUpdate, this, &MainWindow::updateModel);

        connect(dataseNameLE, &QLineEdit::textChanged, [&]() {
            connectPB->setEnabled(!dataseNameLE->text().isEmpty());

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

                    if (!selected.indexes().isEmpty()) {
                        QModelIndex index = selected.indexes().first();
                        int row = index.row();
                        int columnCount = dataTableView->model()->columnCount();

                        updateLEDS(index);
                    } else {
                        resetLeds(0);

                    }
                });

        connect(this, &MainWindow::modelUpdate, this, &MainWindow::calculateCoefficient);

        for (const char *hashName: hash_utils::hash_function_name) {
            hashFunctionCoB->addItem(hashName);
        }

        segmentSizeCoB->addItems(ll);
        segmentSizeCoB->setCurrentIndex(5);

        dbConnection = false;

        readConfiguration();


    }

    void MainWindow::updateStylesheet() {
        QFile styleFile("../../resources/styleshhets/widget.qss");
        styleFile.open(QFile::ReadOnly);
        auto style = QString::fromLatin1(styleFile.readAll());
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

        inputFileLEWO = new FileLineEditWithOption(this, QDir::currentPath());
        labelHashFunction = new QLabel("Hash Function:", this);
        labelSegmentSize = new QLabel("Segment Size:", this);
        hashFunctionCoB = new QComboBox(this);
        segmentSizeCoB = new QComboBox(this);
        importPB = new QPushButton("Import", this);

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
        fileExportLE->setObjectName("fileExportLE");
        exportPB = new QPushButton("Export");
        deletePB = new QPushButton("Delete");
        outputFileLEWO = new FileLineEditWithOption(this, QDir::currentPath(), true);

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


        segmentSizeLCD = new QLCDNumber(this);
        fileDataSizeLCD = new QLCDNumber(this);
        totalSizeLCD = new QLCDNumber(this);
        fileSegmentLCD = new QLCDNumber(this);
        totalRepeatedBlocksLCD = new QLCDNumber(this);
        dataToOriginalPercentageLCD = new QLCDNumber(this);
        totalRepetitionPercentageLCD = new QLCDNumber(this);
        uniquePercentage = new QLCDNumber(this);


        fileDataSizeLCD->setDigitCount(8);
        totalSizeLCD->setDigitCount(8);
        fileSegmentLCD->setDigitCount(8);
        totalRepeatedBlocksLCD->setDigitCount(8);
        dataToOriginalPercentageLCD->setDigitCount(5);
        totalRepetitionPercentageLCD->setDigitCount(5);
        uniquePercentage->setDigitCount(5);

        importTimeLCD = new QLCDNumber(this);
        replaceFileCB = new QCheckBox();
        replaceFileCB->setToolTip("If checked will replace file contents");

        includeOptionsArea = new QGroupBox(this);
        includeOptionsArea->setObjectName("includeOptionsArea");
        incudeOptionLay = new QGridLayout();
        includeOptionsArea->setLayout(incudeOptionLay);

        incudeOptionLay->addWidget(new QLabel("Replace files", this), 0, 0, 1, 1);
        incudeOptionLay->addWidget(replaceFileCB, 0, 1, 1, 1);

        incudeOptionLay->addWidget(new QLabel("Unique segments percentage", this), 0, 2, 1, 2);
        incudeOptionLay->addWidget(uniquePercentage, 0, 4, 1, 1);
        incudeOptionLay->addWidget(new QLabel("%", this), 0, 5, 1, 1);

        incudeOptionLay->addWidget(new QLabel("Load time (ms)", this), 1, 0, 1, 1);
        incudeOptionLay->addWidget(importTimeLCD, 1, 1, 1, 1);
        incudeOptionLay->addWidget(new QLabel("Segment size", this), 1, 2, 1, 1);
        incudeOptionLay->addWidget(segmentSizeLCD, 1, 3, 1, 1);

        incudeOptionLay->addWidget(new QLabel("File data size", this), 2, 0, 1, 1);
        incudeOptionLay->addWidget(fileDataSizeLCD, 2, 1, 1, 1);
        incudeOptionLay->addWidget(new QLabel("Total file size", this), 2, 2, 1, 1);
        incudeOptionLay->addWidget(totalSizeLCD, 2, 3, 1, 1);
        incudeOptionLay->addWidget(dataToOriginalPercentageLCD, 2, 4, 1, 1);
        incudeOptionLay->addWidget(new QLabel("%", this), 2, 5, 1, 1);

        incudeOptionLay->addWidget(new QLabel("Files segment count", this), 3, 0, 1, 1);
        incudeOptionLay->addWidget(fileSegmentLCD, 3, 1, 1, 1);
        incudeOptionLay->addWidget(new QLabel("Unique segment count", this), 3, 2, 1, 1);
        incudeOptionLay->addWidget(totalRepeatedBlocksLCD, 3, 3, 1, 1);
        incudeOptionLay->addWidget(totalRepetitionPercentageLCD, 3, 4, 1, 1);
        incudeOptionLay->addWidget(new QLabel("%", this), 3, 5, 1, 1);


        deleteFilesCB = new QCheckBox();
        deleteFilesCB->setToolTip("If checked entry will be deleted after export.");
        createMainCB = new QCheckBox();
        createMainCB->setToolTip("If checked new directory will be created if path to out does not exist!");
        createMainCB->setChecked(true);

        compareCB = new QCheckBox();
        compareCB->setToolTip("If checked exported entry will be compared with import one");


        errorCountLCD = new QLCDNumber(this);
        exportTimeLCD = new QLCDNumber(this);
        deleteTimeLCD = new QLCDNumber(this);
        totalBlocksLCD = new QLCDNumber(this);
        checkTimeLCD = new QLCDNumber(this);

        errorCountLCD->setDigitCount(8);
        deleteTimeLCD->setDigitCount(5);
        totalBlocksLCD->setDigitCount(8);

        exportOptionsArea = new QGroupBox(this);
        exportOptionsArea->setObjectName("exportOptionsArea");
        exportOptionLay = new QGridLayout();
        exportOptionsArea->setLayout(exportOptionLay);

        exportOptionLay->addWidget(new QLabel("Delete entry", this), 0, 0, 1, 1);
        exportOptionLay->addWidget(deleteFilesCB, 0, 1, 1, 1);


        exportOptionLay->addWidget(new QLabel("Create root directory", this), 1, 0, 1, 1);
        exportOptionLay->addWidget(createMainCB, 1, 1, 1, 1);

        exportOptionLay->addWidget(new QLabel("Compare output", this), 2, 0, 1, 1);
        exportOptionLay->addWidget(compareCB, 2, 1, 1, 1);
        exportOptionLay->addWidget(new QLabel("Compare time (ms)", this), 2, 2, 1, 1);
        exportOptionLay->addWidget(checkTimeLCD, 2, 3, 1, 1);

        exportOptionLay->addWidget(new QLabel("Delete time (ms)", this), 3, 0, 1, 1);
        exportOptionLay->addWidget(deleteTimeLCD, 3, 1, 1, 1);

        exportOptionLay->addWidget(new QLabel("Total checked blocks", this), 3, 2, 1, 1);
        exportOptionLay->addWidget(totalBlocksLCD, 3, 3, 1, 1);

        exportOptionLay->addWidget(new QLabel("Export time (ms)", this), 4, 0, 1, 1);
        exportOptionLay->addWidget(exportTimeLCD, 4, 1, 1, 1);

        exportOptionLay->addWidget(new QLabel("Error count", this), 4, 2, 1, 1);
        exportOptionLay->addWidget(errorCountLCD, 4, 3, 1, 1);


        dbUsageCB = new QCheckBox();
        connectPB = new QPushButton("Connect");
        qled = new QLedIndicator(this);

        connectPB->setEnabled(false);
        dropPB = new QPushButton("Drop database");

        dataseNameLE = new QLineEdit();
        dataseNameLE = new QLineEdit();
        re = QRegularExpression("^[a-z_][A-Za-z0-9_]{1,62}");
        validator = new QRegularExpressionValidator(re, this);
        dataseNameLE->setValidator(validator);

        databaseConfigurationArea = new QGroupBox(tr("Database configiration"));
        databaseConfigurationArea->setObjectName("databaseConfigurationArea");
        dbOptionLay = new QGridLayout();
        databaseConfigurationArea->setLayout(dbOptionLay);

        dbOptionLay->addWidget(new QLabel("Database name:", this), 0, 0, 1, 1);
        dbOptionLay->addWidget(dataseNameLE, 0, 1, 1, 1);
        dbOptionLay->addWidget(new QLabel("Create new database", this), 0, 2, 1, 1);
        dbOptionLay->addWidget(dbUsageCB, 0, 3, 1, 1);
        dbOptionLay->addWidget(connectPB, 1, 0, 1, 1);
        dbOptionLay->addWidget(dropPB, 1, 2, 1, 1);
        dbOptionLay->addWidget(qled, 1, 3, 1, 1);


        dataTableView = new DeselectableTableView(this);
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
        mmLayout->addWidget(exportFileArea, 3, 0, 1, 1);

        mmLayout->addWidget(includeOptionsArea, 1, 1, 1, 1);
        mmLayout->addWidget(exportOptionsArea, 3, 1, 1, 1);
        mmLayout->addWidget(logTextField, 5, 0, 1, 2);


        timer = new QTimer(this);

        timer->start(50);


        list = {fileDataSizeLCD, segmentSizeLCD, totalSizeLCD, fileSegmentLCD, totalRepeatedBlocksLCD,
                dataToOriginalPercentageLCD, totalRepetitionPercentageLCD, importTimeLCD, exportTimeLCD,
                errorCountLCD, deleteTimeLCD, totalBlocksLCD, checkTimeLCD, uniquePercentage};

        for (auto *elem: list) {
            elem->setSegmentStyle(QLCDNumber::Flat);
        }

        settingsWindow = new SettingsWindow(this);
    }

    void MainWindow::onImport() {
        QString inputPath = inputFileLEWO->getContent();

        writeLog(QString("Import entry %1").arg(toShortPath(inputPath)));
        bool isDir = inputFileLEWO->getType() == Directory;
        bool replace = replaceFileCB->isChecked();
        unsigned segmentSize = segmentSizeCoB->currentText().toInt();
        hash_utils::hash_function  tt= (hashFunctionCoB->currentIndex()<hashFunctionCoB->count())?
                static_cast<hash_function>(hashFunctionCoB->currentIndex()):SHA_256;



         int res = 0;

        if (replace) {
            if (isDir) {
                res = fileService.processDirectory<file_services::ReplaceWithNew>(inputPath.toStdString(), segmentSize,tt);
            } else {
                res = fileService.processFile<file_services::ReplaceWithNew>(inputPath.toStdString(), segmentSize,tt);
            }
        } else {
            if (isDir) {
                res = fileService.processDirectory(inputPath.toStdString(), segmentSize,tt);
            } else {
                res = fileService.processFile(inputPath.toStdString(), segmentSize,tt);
            }
        }
        if (res < 0) {
            if (res == AlreadyExists) {
                writeLog(QString("%1 already exists")
                                 .arg(isDir ? "Directory" : "File"),WARNING);
            } else {
                writeLog("Error occurred during entry import");
            }
        }

        emit modelUpdate();
    }

    void MainWindow::onExport() {
        QString outputPath = outputFileLEWO->getContent();
        QString exportPath = fileExportLE->text();

        writeLog(QString("Exporting entry %1 to %2").arg(toShortPath(exportPath)).arg(toShortPath(outputPath)));


        bool main = createMainCB->isChecked();
        bool remove = deleteFilesCB->isChecked();
        bool isDir = outputFileLEWO->getType() == Directory;
        int flag = 2 * main + remove;
        int res = 0;
        QElapsedTimer timer1;
        timer1.start();
        if (isDir) {
            res = (fileService.*dirs[flag])(exportPath.toStdString(), outputPath.toStdString());
        } else {
            res = (fileService.*files[flag])(exportPath.toStdString(), outputPath.toStdString(),
                                             paramType::EmptyParameterValue);
        }
        exportTimeLCD->display(static_cast<int>(timer1.elapsed()));

        if (res != 0) {
            writeLog("Error occured during entry export", ERROR);
        }

        if (remove) {
            emit modelUpdate();
        }
        if (compareCB->checkState()) {
            QElapsedTimer timer1;
            timer1.start();
            this->compareExport(exportPath, outputPath, isDir);
            checkTimeLCD->display(static_cast<int>(timer1.elapsed()));
        } else {
            checkTimeLCD->display(0);
        }

    }

    void MainWindow::onDelete() {


        QString path = fileExportLE->text();
        writeLog(QString("Deleting entry %1").arg(toShortPath(path)));
        bool isDir = isDirName(fileExportLE->text());

        QElapsedTimer timer1;
        timer1.start();
        int res;
        if (isDir) {
            res = this->fileService.deleteDirectory(path.toStdString());
        } else {
            res = this->fileService.deleteFile(path.toStdString());
        }

        if (res != 0) {
            writeLog("Error occurred during entry delete", ERROR);
        }

        deleteTimeLCD->display(static_cast<int>(timer1.elapsed()));
        gClk.reset();
        emit modelUpdate();
    }

    void MainWindow::onSettings() {
        auto stat = settingsWindow->exec();

        if (stat == QDialog::Accepted) {
            c_str = std::move(settingsWindow->getConfiguration());
            this->dataseNameLE->setText(QString::fromStdString(c_str.getDbname()));
            writeLog(QString("Settings updated  new connection string \"%1\"").arg(c_str.c_str()));
        } else {
            writeLog("Rejected connection", ERROR);
        }
        bool old = dbConnection;
        dbConnection = (checkConnString(c_str));
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
        this->c_str.setDbname(dataseNameLE->text().toStdString());
        bool old = dbConnection;


        if (dbUsageCB->isChecked()) {
            dbConnection = checkConnString(c_str);
            if (dbConnection) {
                writeLog("Database already exists", WARNING);
                return;
            }
            writeLog(QString("Creating database %1!").arg(dataseNameLE->text()));
            fileService.dbLoad<file_services::create>(c_str.getDbname());

        } else {
            writeLog(QString("Connecting to database %1!").arg(dataseNameLE->text()));
            fileService.dbLoad(c_str.getDbname());
        }

        dbConnection = checkConnString(c_str);
        emit connectionChanged(old);

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
                            dataseNameLE->setText(QString::fromStdString(c_str.getDbname()));


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
        dropPB->setEnabled(dbConnection);
        dataTableView->selectionModel()->clearSelection();
        dataTableView->resizeColumnToContents(0);
        dataTableView->resizeColumnsToContents();
        dataTableView->resizeRowsToContents();
        if (!dbConnection) {
            reset:
            dataTableView->selectionModel()->clearSelection();
            if (dataTableView->model()) {
                fileService.disconnect();
                myViewModel->Reset();
            }
            uniquePercentage->display(0);
            writeLog("Unable to connect", WARNING);
        } else {
            auto dbName = dataseNameLE->text().toStdString();

            bool conn = myViewModel->performConnection(c_str);

            auto res = fileService.dbLoad(c_str);

            if (!conn || res != 0) {
                goto reset;
            }

            emit modelUpdate();


            writeLog("Connection was established", RESULT);
        }
        qled->setChecked(dbConnection);
    }

    void MainWindow::resizeEvent(QResizeEvent *event) {
        QWidget::resizeEvent(event);
        dataTableView->sizeHint();
    }

    void MainWindow::updateModel() {
        myViewModel->getData();
        dataTableView->sizeHint();
        dataTableView->setSortingEnabled(true);
        dataTableView->sortByColumn(-1, Qt::SortOrder::DescendingOrder);
        dataTableView->resizeColumnToContents(0);
        dataTableView->resizeColumnsToContents();
        dataTableView->resizeRowsToContents();

    }

    void MainWindow::updateLEDS(QModelIndex &idx) {
        updateStylesheet();
        int row = idx.row();


        segmentSizeLCD->display(idx.sibling(row, 1).data().toInt());
        fileDataSizeLCD->display(idx.sibling(row, 2).data().toInt());
        totalSizeLCD->display(idx.sibling(row, 3).data().toInt());
        dataToOriginalPercentageLCD->display(smartCeil(idx.sibling(row, 4).data().toDouble(), 2));
        fileSegmentLCD->display(idx.sibling(row, 5).data().toInt());
        totalRepeatedBlocksLCD->display(idx.sibling(row, 6).data().toInt());
        totalRepetitionPercentageLCD->display(smartCeil(idx.sibling(row, 7).data().toDouble(), 2));
        importTimeLCD->display(smartCeil(idx.sibling(row, 8).data().toDouble(), 2));
    }

    void MainWindow::resetLeds(int i) {


        if (i == 0) {
            segmentSizeLCD->display(0);
            fileDataSizeLCD->display(0);
            totalSizeLCD->display(0);
            dataToOriginalPercentageLCD->display(0);
            fileSegmentLCD->display(0);
            totalRepeatedBlocksLCD->display(0);
            totalRepetitionPercentageLCD->display(0);
            importTimeLCD->display(0);
        } else {
            deleteTimeLCD->display(0);
            totalBlocksLCD->display(0);
            errorCountLCD->display(0);
            exportTimeLCD->display(0);
            checkTimeLCD->display(0);
        }
    }

    QString MainWindow::toShortPath(const QString &qString) {
        return QString::fromStdString(
                std::filesystem::path(qString.toStdString()).lexically_relative(
                        std::filesystem::current_path()).string());
    }

    void MainWindow::compareExport(const QString &exportee, const QString &output, bool isDirectory) {
        writeLog(QString("Compare %1 %2 %3")
                         .arg(isDirectory ? "directories" : "files")
                         .arg(exportee).arg(output));
        size_t segmentSize = segmentSizeCoB->currentText().toUInt();
        std::array<size_t, 4> res;//todo total blocks files
        if (isDirectory) {
            res = file_services::compareDirectories(exportee.toStdString(), output.toStdString(), segmentSize);

        } else {
            res = file_services::compareFiles(exportee.toStdString(), output.toStdString(), segmentSize);
        }
        errorCountLCD->display((int) (res[0]));
        totalBlocksLCD->display((int) (res[2]));
        if (res[0] != 0) {
            writeLog(QString("%1 errors was found")
                             .arg(res[0]), ERROR);
        }
    }
}
