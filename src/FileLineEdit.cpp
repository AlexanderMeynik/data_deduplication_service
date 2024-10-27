#include <QHBoxLayout>
#include "FileLineEdit.h"
#include "dbCommon.h"

namespace windows {
    FileLineEdit::FileLineEdit(QWidget *parent,
                               QString dirPath,
                               bool saveFile,
                               bool LineAcess) :
            QWidget(parent),
            dirPath(dirPath),
            saveFile(saveFile) {
        setUpUi();
        lineEdit->setReadOnly(LineAcess);


        connect(pushButton, &QPushButton::pressed, this, &FileLineEdit::onBrowse);
        connect(lineEdit, &QLineEdit::textChanged, [&](const QString &str) {
            emit FileLineEdit::contentChanged(str);
        });
        type=File;

    }


    void FileLineEdit::setUpUi() {
        mainLayout = new QHBoxLayout(this);
        lineEdit = new QLineEdit(this);

        pushButton = new QPushButton(this);
        pushButton->setText("Browse");

        mainLayout->addWidget(lineEdit);
        mainLayout->addWidget(pushButton);

        this->setLayout(mainLayout);
    }


    void FileLineEdit::onBrowse() {
        QFileDialog::Options options;

        options |= QFileDialog::DontUseNativeDialog;
        QString selectedFilter;
        QString result;

        if (saveFile) {
            result = QFileDialog::getSaveFileName(this,
                                                  tr("Save Files"),
                                                  dirPath,
                                                  QString(),
                                                  &selectedFilter,
                                                  options);
        } else {
            result = QFileDialog::getOpenFileName(this,
                                                  tr("Find Files"),
                                                  dirPath,
                                                  QString(),
                                                  &selectedFilter,
                                                  options);
        }

        if (!result.isEmpty()) {
            lineEdit->setText(result);
        }
    }

    void FileLineEdit::setSaveFile(bool saveFile) {
        FileLineEdit::saveFile = saveFile;
    }

    bool FileLineEdit::isSaveFile() const {
        return saveFile;
    }


    FileLineEditWithOption::FileLineEditWithOption(QWidget *parent,
                                                   QString dirPath,
                                                   bool saveFile,
                                                   bool LineAcess) :
            FileLineEdit(parent,
                         std::move(dirPath),
                         saveFile,
                         LineAcess) {

        selectModeCheckBox = new QCheckBox(this);
        selectModeCheckBox->setChecked(false);
        selectModeCheckBox->setToolTip("Directory view mode!");
        mainLayout->addWidget(selectModeCheckBox);

        connect(selectModeCheckBox, &QCheckBox::stateChanged, [&] {
            lineEdit->setText("");
            type=(selectModeCheckBox)?Directory:File;
        });
    }


    void FileLineEditWithOption::onBrowse() {
        QString result;
        if (!selectModeCheckBox->isChecked()) {
            FileLineEdit::onBrowse();
            return;
        } else {

            QFileDialog::Options options;
            options |= QFileDialog::DontUseNativeDialog;
            QString selectedFilter;
            result = QFileDialog::getExistingDirectory(this,
                                                       tr("Find Files"),
                                                       dirPath);
        }


        if (!result.isEmpty()) {
            lineEdit->setText(result);
        }
    }


} // windows
