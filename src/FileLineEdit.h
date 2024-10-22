

#ifndef DATA_DEDUPLICATION_SERVICE_FILELINEEDIT_H
#define DATA_DEDUPLICATION_SERVICE_FILELINEEDIT_H

#include <QPushButton>
#include <QWidget>
#include <QLineEdit>
#include <QFileDialog>
#include <QHBoxLayout>

namespace windows {


    class FileLineEdit : public QWidget {
    Q_OBJECT

    public:
        explicit FileLineEdit(QWidget *parent = nullptr, QString dirPath = QDir::currentPath());

        QString getContent() {
            return lineEdit->text();
        }

        ~FileLineEdit() override = default;

    private slots:

        void onBrowse();

    private:
        QLineEdit *lineEdit;
        QPushButton *pushButton;

        void setUpUi();

        QString dirPath;
    };
} // windows

#endif //DATA_DEDUPLICATION_SERVICE_FILELINEEDIT_H
