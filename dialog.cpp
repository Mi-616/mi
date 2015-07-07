#include "dialog.h"
#include "ui_dialog.h"
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QDebug>
Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_readMsgBtn_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this);
    if(! fileName.isEmpty())
    {
        QFile file(fileName);
        file.open(QFile::ReadOnly | QFile::Text);qDebug("open file");
        QTextStream read(&file);qDebug("read file");
        QString lineStr;
        while(!read.atEnd())
        {
            lineStr = read.readLine();qDebug("get msg");
            ui->textBrowser->append(lineStr);
        }
    }
}
