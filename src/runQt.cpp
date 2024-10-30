#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <memory>
#include "MainWindow.h"

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    google::InitGoogleLogging(argv[0]);
    google::SetVLOGLevel("*", 3);
    auto wind = windows::MainWindow(nullptr);
    wind.show();
    return app.exec();
}