#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <memory>
#include "MainWindow.h"
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    auto wind = windows::MainWindow(nullptr);
    wind.show();
    return app.exec();
}