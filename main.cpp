#include <QApplication>
#include "widget.h"
#include <QTextCodec>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QTextCodec *codec= QTextCodec::codecForName("gbk");
    QTextCodec::setCodecForLocale(codec);

    Widget w;
    w.show();

    return a.exec();
}
