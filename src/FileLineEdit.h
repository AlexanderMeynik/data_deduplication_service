

#ifndef DATA_DEDUPLICATION_SERVICE_FILELINEEDIT_H
#define DATA_DEDUPLICATION_SERVICE_FILELINEEDIT_H

#include <QPushButton>
#include <QWidget>
#include <QLineEdit>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLabel>

#include "dbCommon.h"

/// windows namespace
namespace windows {
    ///Entry type enum
    enum EntryType {
        File,
        Directory
    };

    /**
     * @brief Class for file name selection
     */
    class FileLineEdit : public QWidget {
    Q_OBJECT
    public:
        explicit FileLineEdit(QWidget *parent = nullptr,
                              QString dirPath = QDir::currentPath(),
                              bool saveFile = false,
                              bool ReadOnly = true);

        QString getContent() {
            return lineEdit->text();
        }

        bool isEmpty() {
            return lineEdit->text().isEmpty();
        }

        ~FileLineEdit() override = default;

        void setSaveFile(bool saveFile);

        bool isSaveFile() const;

        EntryType getType() {
            return type;
        }
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

        EntryType type;

        void setUpUi();
    };

    /**
     * @brief Class that allow to choose either files or directories
     */
    class FileLineEditWithOption : public FileLineEdit {
    Q_OBJECT
    public:
        explicit FileLineEditWithOption(QWidget *parent = nullptr,
                                        QString dirPath = QDir::currentPath(),
                                        bool saveFile = false,
                                        bool ReadOnly = true);

        ~FileLineEditWithOption() override = default;

    signals:
        void typeChanged(const EntryType &etr);
    protected slots:
        void onBrowse() override;

    protected:
        QCheckBox *selectModeCheckBox;
    };


} // windows

#endif //DATA_DEDUPLICATION_SERVICE_FILELINEEDIT_H
