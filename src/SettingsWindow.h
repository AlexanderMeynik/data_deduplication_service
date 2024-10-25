
#ifndef DATA_DEDUPLICATION_SERVICE_SETTINGSWINDOW_H
#define DATA_DEDUPLICATION_SERVICE_SETTINGSWINDOW_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QFileSelector>
#include "dbCommon.h"

#include "FileLineEdit.h"
#include "qledindicator.h"


namespace windows {
    using db_services::myConnString;

    class SettingsWindow : public QDialog {
    Q_OBJECT
    public:
        explicit SettingsWindow(QWidget *parent = nullptr);

        ~SettingsWindow() override = default;

        myConnString getConfiguration();


    private slots:

        void onLoadConfig();

        void onSaveConfig();

        void onTestConnection();

        void onApply();

        void checkConditions();

    private:
        QLineEdit *dbHostInput;
        QLineEdit *dbPortInput;
        QLineEdit *dbUserInput;
        QLineEdit *dbPasswordInput;
        QLineEdit *dbNameInput;

        QVector<QLineEdit *> Lines;

        QPushButton *applyButton;
        QPushButton *testConnectionButton;
        QPushButton *loadButton;
        QPushButton *saveButton;


        QFormLayout *formLayout;
        QVBoxLayout *mainLayout;

        FileLineEdit *fileLineEdit;
        QLedIndicator *qLedIndicator;

        void setupUI();
    };//SettingsWindow class


} // windows

#endif //DATA_DEDUPLICATION_SERVICE_SETTINGSWINDOW_H
