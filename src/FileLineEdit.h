

#ifndef DATA_DEDUPLICATION_SERVICE_FILELINEEDIT_H
#define DATA_DEDUPLICATION_SERVICE_FILELINEEDIT_H

#include <QPushButton>
#include <QWidget>
#include <QLineEdit>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QCheckBox>

#include "dbCommon.h"

namespace windows {
    class FileLineEdit : public QWidget {
    Q_OBJECT

    public:
        explicit FileLineEdit(QWidget *parent = nullptr,
                              QString dirPath = QDir::currentPath(),
                              bool saveFile = false,
                              bool LineAcess = true);

        QString getContent() {
            return lineEdit->text();
        }

        ~FileLineEdit() override = default;

        void setSaveFile(bool saveFile);

        bool isSaveFile() const;

    signals:

        void contentChanged(const QString &str);

    protected slots:

        virtual void onBrowse();

    protected:
        QString dirPath;
        QHBoxLayout *mainLayout;
        QLineEdit *lineEdit;
        QPushButton *pushButton;
        bool saveFile;

        void setUpUi();
    };


    class FileLineEditWithOption : public FileLineEdit {
    Q_OBJECT
    public:

        explicit FileLineEditWithOption(QWidget *parent = nullptr,
                                        QString dirPath = QDir::currentPath(),
                                        bool saveFile = false,
                                        bool LineAcess = true);

        ~FileLineEditWithOption() override = default;


    protected slots:

        void onBrowse() override;

    protected:
        QCheckBox *selectModeCheckBox;
    };


} // windows

#endif //DATA_DEDUPLICATION_SERVICE_FILELINEEDIT_H