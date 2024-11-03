#include <QApplication>
#include "MainWindow.h"

/**
 * Qt entrypoint
 * @param argc
 * @param argv
 */
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    google::InitGoogleLogging(argv[0]);
    google::SetVLOGLevel("*", 3);
    auto wind = windows::MainWindow(nullptr);
    wind.show();
    return app.exec();
}