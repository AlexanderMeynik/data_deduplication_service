
#include "SettingsWindow.h"

#include <QLabel>
#include <QFileDialog>
#include <QGridLayout>

#include <QValidator>
#include "dbCommon.h"


namespace windows {


    SettingsWindow::SettingsWindow(QWidget *parent)
            : QDialog(parent) {

        setupUI();

        connect(loadButton, &QPushButton::clicked, this, &SettingsWindow::onLoadConfig);
        connect(saveButton, &QPushButton::clicked, this, &SettingsWindow::onSaveConfig);
        connect(testConnectionButton, &QPushButton::clicked, this, &SettingsWindow::onTestConnection);
        connect(applyButton, &QPushButton::clicked, this, &SettingsWindow::onApply);

        for (QLineEdit *line: Lines) {
            connect(line, &QLineEdit::textChanged, this, &SettingsWindow::checkConditions);
        }

    }

    void SettingsWindow::setupUI() {

        setWindowTitle("Database Settings");

        Lines = QVector<QLineEdit *>();

        mainLayout = new QVBoxLayout(this);

        formLayout = new QFormLayout();


        dbHostInput = new QLineEdit(this);
        dbPortInput = new QLineEdit(this);
        dbUserInput = new QLineEdit(this);
        dbPasswordInput = new QLineEdit(this);
        dbPasswordInput->setEchoMode(QLineEdit::Password);
        dbNameInput = new QLineEdit(this);

        Lines.push_back(dbHostInput);
        Lines.push_back(dbPortInput);
        Lines.push_back(dbUserInput);
        Lines.push_back(dbPasswordInput);
        Lines.push_back(dbHostInput);
        Lines.push_back(dbNameInput);

        formLayout->addRow("Database Name:", dbNameInput);
        formLayout->addRow("Database User:", dbUserInput);
        formLayout->addRow("Database Password:", dbPasswordInput);
        formLayout->addRow("Database Host:", dbHostInput);
        formLayout->addRow("Database Port:", dbPortInput);


        dbPortInput->setValidator(new QIntValidator(0, 65535, this));
        auto prevFileDir = QString::fromStdString(std::filesystem::absolute(
                std::filesystem::path(db_services::cfileName).parent_path().lexically_normal()
        ).string());
        fileLineEdit = new FileLineEditWithOption(this, prevFileDir);

        auto d1 = new QHBoxLayout();
        auto d2 = new QHBoxLayout();
        auto d3 = new QHBoxLayout();


        loadButton = new QPushButton("Load", this);
        applyButton = new QPushButton("Apply", this);
        testConnectionButton = new QPushButton("testConnection", this);
        saveButton = new QPushButton("Save", this);

        qLedIndicator = new QLedIndicator(this);
        qLedIndicator->setChecked(false);


        qLedIndicator->setOffColor1(QColor(255, 0, 0, 255));
        qLedIndicator->setOffColor2(QColor(255, 0, 0, 255));

        d1->addWidget(loadButton);
        d1->addWidget(saveButton);
        d2->addWidget(testConnectionButton);
        d2->addWidget(qLedIndicator);
        d3->addWidget(applyButton);

        mainLayout->addLayout(formLayout);
        mainLayout->addWidget(fileLineEdit);

        mainLayout->addLayout(d1);
        mainLayout->addLayout(d2);
        mainLayout->addLayout(d3);

        setLayout(mainLayout);
        checkConditions();
    }


    void SettingsWindow::onLoadConfig() {
        QFile file;
        file.setFileName(fileLineEdit->getContent());
        file.open(QIODevice::ReadOnly);

        if (!file.isOpen()) {
            qWarning("Unable to open file");
            return;
        }
        QTextStream in(&file);

        QString user, password, dbName, hostname, port;
        in >> dbName >> user >> password >> hostname >> port;
        if (in.status() != QTextStream::Ok) {
            qInfo("Error occured during read");
            return;
        }

        dbUserInput->setText(user);
        dbPasswordInput->setText(password);
        dbNameInput->setText(dbName);
        dbHostInput->setText(hostname);
        dbPortInput->setText(port);


    }

    void SettingsWindow::onSaveConfig() {
        auto prevFileDir = QString::fromStdString(std::filesystem::absolute(
                std::filesystem::path(db_services::cfileName).parent_path().lexically_normal()
        ).string());

        QString fileName = QFileDialog::getSaveFileName(this,
                                                        tr("Save Conf"),
                                                        prevFileDir);
        if (fileName.isEmpty()) {
            //todo print something
            qInfo("nothing");
            return;
        }

        QFile file;
        file.setFileName(fileName);
        file.open(QIODevice::WriteOnly);

        if (!file.isOpen()) {
            qWarning("Unable to open file");
            return;
        }
        QTextStream out(&file);

        out << dbNameInput->text() << '\n'
            << dbUserInput->text() << '\n'
            << dbPasswordInput->text() << '\n'
            << dbHostInput->text() << '\n'
            << dbPortInput->text();

        if (out.status() != QTextStream::Ok) {
            qInfo("Error occurred during write");
            return;
        }

        file.close();


    }

    void SettingsWindow::onTestConnection() {
        bool check = checkConnString(this->getConfiguration());

        qLedIndicator->setChecked(check);
    }


    void SettingsWindow::onApply() {
        onTestConnection();
        if(!qLedIndicator->isChecked())
        {
            reject();
            return;
        }
        accept();
    }

    myConnString SettingsWindow::getConfiguration() {
        return {dbUserInput->text().toStdString(),
                dbPasswordInput->text().toStdString(),
                dbHostInput->text().toStdString(),
                dbNameInput->text().toStdString(),
                dbPortInput->text().toUInt()};
    }


    void SettingsWindow::checkConditions() {
        bool allFiledsNotEmpty = std::all_of(Lines.begin(), Lines.end(), [&](QLineEdit *item) {
            return !item->text().isEmpty();
        });

        if (allFiledsNotEmpty) {
            applyButton->setEnabled(true);
            saveButton->setEnabled(true);
            testConnectionButton->setEnabled(true);
        } else {
            applyButton->setEnabled(false);
            saveButton->setEnabled(false);
            testConnectionButton->setEnabled(false);
        }
    }

} // windows
