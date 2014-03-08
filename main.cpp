#include <QApplication>
#include "amcpp.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("amcpp");
    amcpp w;
    w.show();

    return a.exec();
}
