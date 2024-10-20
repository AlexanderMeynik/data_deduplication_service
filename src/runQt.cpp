#include <QApplication>
#include <QPushButton>

#include <FileService.h>
#include <QWidget>
#include <QMainWindow>
#include <QVBoxLayout>
#include <memory>
int main(int argc, char **argv)
{
    QApplication app (argc, argv);
    auto ww=std::make_unique<QWidget>();
    ww->setMinimumSize({600,800});

    auto ss=new QVBoxLayout();


//todo a lot of stuff
    auto button=new QPushButton();
    ss->addWidget(button);

    button->setText("Press this thing !");

    auto presscount=0;

    QObject::connect(button,&QPushButton::clicked,[&]()
    {
        std::cout<<++presscount<<'\n';
    });

    ww->setLayout(ss);
    ww->show();

    return app.exec();
}