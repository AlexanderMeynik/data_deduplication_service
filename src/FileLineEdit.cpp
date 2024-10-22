#include "FileLineEdit.h"
#include "dbCommon.h"

namespace windows {
    FileLineEdit::FileLineEdit(QWidget *parent, QString dirPath) :
            QWidget(parent), dirPath(dirPath) {
        setUpUi();
    }


    void FileLineEdit::setUpUi() {
        QHBoxLayout *qh = new QHBoxLayout(this);


        lineEdit = new QLineEdit(this);
        lineEdit->setText(QString::fromStdString(db_services::cfileName));

        pushButton = new QPushButton(this);
        pushButton->setText("Browse");
        qh->addWidget(lineEdit);
        qh->addWidget(pushButton);

        connect(pushButton, &QPushButton::pressed, this, &FileLineEdit::onBrowse);
        this->setLayout(qh);
    }

    void FileLineEdit::onBrowse() {

        QFileDialog::Options options;

        options |= QFileDialog::DontUseNativeDialog;

        QString selectedFilter;
        QString directory = QFileDialog::getOpenFileName(this,
                                                         tr("Find Files"),
                                                         dirPath,
                                                         QString(),
                                                         &selectedFilter,
                                                         options);
        if (!directory.isEmpty()) {
            lineEdit->setText(directory);
        }
    }
} // windows
