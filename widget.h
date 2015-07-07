#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "dialog.h"
class QUdpSocket;

class TcpServer;
namespace Ui {
class Widget;
}

enum MessageType{Message, NewParticipant, ParticipantLeft, FileName, Refuse};


class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

protected:
    void newParticipant(QString userName,
                        QString localHostName, QString ipAddress);
    void participantLeft(QString userName,
                         QString localHostName, QString time);
    void sendMessage(MessageType type, QString serverAddress="");

    void insertWidgetContact(QString localHostName);
    void hasPendingFile(QString userName, QString serverAddress,
                        QString clientAddress, QString fileName);
    void saveFile(QString fileName);

    QString getIP();
    QString getUserName();
    QString getMessage();
    void getWebIP();

private:
    Ui::Widget *ui;
    QUdpSocket *udpSocket;
    int port;
    QString broadcastAddr;
    QString fileName;
    TcpServer *server;
    Dialog *readChatBtn;
    QColor color;
private slots:
    void processPendingDatagrams();
    void on_sendButton_clicked();
    void on_closeButton_clicked();
    void on_sendFileBtn_clicked();
    void getFileName(QString);
    void on_checkGroupBox_clicked();
    void on_addContactBtn_clicked();
    void on_removeContactBtn_clicked();
    void on_readChatBtn_clicked();
    void on_pushButton_clicked();
    void on_fontComboBox_currentFontChanged(const QFont &f);
    void on_sizeComboBox_currentIndexChanged(QString size);
    void on_boldBtn_clicked(bool checked);
    void on_italicBtn_clicked(bool checked);
    void on_underlineBtn_clicked(bool checked);
    void on_colorlBtn_clicked();
    void on_saveToolBtn_clicked();
};

#endif // WIDGET_H
