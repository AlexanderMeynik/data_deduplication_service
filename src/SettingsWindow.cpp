
#include "SettingsWindow.h"

#include <QLabel>
#include <QFileDialog>
#include <QGridLayout>
#include <QValidator>
#include <QXmlStreamWriter>
#include "dbCommon.h"


namespace windows {


    SettingsWindow::SettingsWindow(QWidget *parent)
            : QDialog(parent) {

        setupUI();

        connect(loadConfigurationPB, &QPushButton::clicked, this, &SettingsWindow::onLoadConfig);
        connect(saveConfigurationPB, &QPushButton::clicked, this, &SettingsWindow::onSaveConfig);
        connect(testConnectionPB, &QPushButton::clicked, this, &SettingsWindow::onTestConnection);
        connect(applyPB, &QPushButton::clicked, this, &SettingsWindow::onApply);

        for (QLineEdit *line: lineEditArray) {
            connect(line, &QLineEdit::textChanged, this, &SettingsWindow::checkConditions);
        }

    }

    void SettingsWindow::setupUI() {

        setWindowTitle("Database Settings");

        lineEditArray = QVector<QLineEdit *>();

        mainLayout = new QVBoxLayout(this);

        imputFormLay = new QFormLayout();


        dbHostLE = new QLineEdit(this);
        dbPortLE = new QLineEdit(this);
        dbUserLE = new QLineEdit(this);
        dbPasswordLE = new QLineEdit(this);
        dbPasswordLE->setEchoMode(QLineEdit::Password);
        dbNameLE = new QLineEdit(this);

        lineEditArray.push_back(dbHostLE);
        lineEditArray.push_back(dbPortLE);
        lineEditArray.push_back(dbUserLE);
        lineEditArray.push_back(dbPasswordLE);
        lineEditArray.push_back(dbHostLE);
        lineEditArray.push_back(dbNameLE);

        imputFormLay->addRow("Database Name:", dbNameLE);
        imputFormLay->addRow("Database User:", dbUserLE);
        imputFormLay->addRow("Database Password:", dbPasswordLE);
        imputFormLay->addRow("Database Host:", dbHostLE);
        imputFormLay->addRow("Database Port:", dbPortLE);


        dbPortLE->setValidator(new QIntValidator(0, 65535, this));
        auto prevFileDir = QString::fromStdString(std::filesystem::absolute(
                std::filesystem::path(db_services::cfileName).parent_path().lexically_normal()
        ).string());
        fileLineEdit = new FileLineEditWithOption(this, prevFileDir);

        auto d1 = new QHBoxLayout();
        auto d2 = new QHBoxLayout();
        auto d3 = new QHBoxLayout();


        loadConfigurationPB = new QPushButton("Load", this);
        applyPB = new QPushButton("Apply", this);
        testConnectionPB = new QPushButton("testConnection", this);
        saveConfigurationPB = new QPushButton("Save", this);

        qLedIndicator = new QLedIndicator(this);
        qLedIndicator->setChecked(false);


        qLedIndicator->setOffColor1(QColor(255, 0, 0, 255));
        qLedIndicator->setOffColor2(QColor(255, 0, 0, 255));

        d1->addWidget(loadConfigurationPB);
        d1->addWidget(saveConfigurationPB);
        d2->addWidget(testConnectionPB);
        d2->addWidget(qLedIndicator);
        d3->addWidget(applyPB);

        mainLayout->addLayout(imputFormLay);
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

        dbUserLE->setText(user);
        dbPasswordLE->setText(password);
        dbNameLE->setText(dbName);
        dbHostLE->setText(hostname);
        dbPortLE->setText(port);


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

        out << dbNameLE->text() << '\n'
            << dbUserLE->text() << '\n'
            << dbPasswordLE->text() << '\n'
            << dbHostLE->text() << '\n'
            << dbPortLE->text();

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
        if (!qLedIndicator->isChecked()) {
            reject();
            return;
        }

        if (!fileLineEdit->getContent().isEmpty()) {
            QFile file(confName);
            file.open(QIODevice::WriteOnly);
            QXmlStreamWriter xmlWriter(&file);
            xmlWriter.setAutoFormatting(true);

            xmlWriter.writeStartDocument();
            xmlWriter.writeStartElement("resources");

            xmlWriter.writeStartElement(parentTag);
            xmlWriter.writeAttribute("path", fileLineEdit->getContent());
            xmlWriter.writeEndElement();


            xmlWriter.writeEndElement();
            xmlWriter.writeEndDocument();
            file.close();

        }
        accept();
    }

    myConnString SettingsWindow::getConfiguration() {
        return {dbUserLE->text().toStdString(),
                dbPasswordLE->text().toStdString(),
                dbHostLE->text().toStdString(),
                dbNameLE->text().toStdString(),
                dbPortLE->text().toUInt()};
    }


    void SettingsWindow::checkConditions() {
        bool allFiledsNotEmpty = std::all_of(lineEditArray.begin(), lineEditArray.end(), [&](QLineEdit *item) {
            return !item->text().isEmpty();
        });

        if (allFiledsNotEmpty) {
            applyPB->setEnabled(true);
            saveConfigurationPB->setEnabled(true);
            testConnectionPB->setEnabled(true);
        } else {
            applyPB->setEnabled(false);
            saveConfigurationPB->setEnabled(false);
            testConnectionPB->setEnabled(false);
        }
    }

} // windows
