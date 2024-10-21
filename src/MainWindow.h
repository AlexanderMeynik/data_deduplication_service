
#ifndef DATA_DEDUPLICATION_SERVICE_MAINWINDOW_H
#define DATA_DEDUPLICATION_SERVICE_MAINWINDOW_H

#include <QMainWindow>

namespace windows {
    QT_BEGIN_NAMESPACE
    namespace Ui { class MainWindow; }
    QT_END_NAMESPACE


    class MainWindow : public QMainWindow {
    Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = nullptr);

        ~MainWindow() override;

    private:
        Ui::MainWindow *ui;
    };
}



#endif //DATA_DEDUPLICATION_SERVICE_MAINWINDOW_H
