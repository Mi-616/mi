#include "widget.h"
#include "ui_widget.h"
#include "tcpserver.h"
#include "tcpclient.h"
#include <QUdpSocket>
#include <QHostInfo>
#include <QMessageBox>
#include <QScrollBar>
#include <QDateTime>
#include <QNetworkInterface>
#include <QProcess>
#include <QtNetwork>
#include <QFileDialog>
#include <QColorDialog>
Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    getWebIP();
    ui->userTableWidget->setFrameShape(QFrame::NoFrame);
    ui->userTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->userTableWidget->horizontalHeader()->setHighlightSections(false);//点击表时不对headerView(表头行)光亮（获取焦点）
    ui->userTableWidget->horizontalHeader()->resizeSection(0,70);
    ui->userTableWidget->horizontalHeader()->resizeSection(1,110);
    ui->userTableWidget->horizontalHeader()->resizeSection(2,110);
    ui->userTableWidget->horizontalHeader()->resizeSection(3,20);
    ui->userTableWidget->verticalHeader()->setDefaultSectionSize(15);
    ui->userTableWidget->verticalHeader()->setVisible(false);
    ui->checkGroupBox->setCheckState(Qt::Checked);
    udpSocket = new QUdpSocket(this);
    port = 61616;
    broadcastAddr="255.255.255.255";
    udpSocket->bind(port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));
    sendMessage(ParticipantLeft,"broadcast");//在发送新加入请求前广播一个离开请求，这样所有其余在线客户端才会清理本机客户端上次异常离开时没有删除的在线状态,因此可以保证本机客户端再次登录能正常获取其余在线客户端的在线状态
    sendMessage(NewParticipant,"broadcast");

    server = new TcpServer(this);
    connect(server, SIGNAL(sendFileName(QString)), this, SLOT(getFileName(QString)));
}

Widget::~Widget()
{
    delete ui;
}

// 使用UDP发送信息
void Widget::sendMessage(MessageType type, QString serverAddress)
{
    QByteArray data;
    QDataStream out(&data, QIODevice::WriteOnly);
    QString localHostName = QHostInfo::localHostName();
    QString address = getIP();
    out << type << getUserName() << localHostName;
    QHostAddress ip;
    if(serverAddress=="broadcast")
        ip.setAddress(broadcastAddr);
    else
    ip.setAddress(QString(serverAddress));
    switch(type)
    {
    case Message :
        if (ui->messageTextEdit->toPlainText() == "") {
            QMessageBox::warning(0,"Warning","Empy Message,Please Say Sth",QMessageBox::Ok);
            return;
        }
        out << address << getMessage();
        ui->messageBrowser->verticalScrollBar()
                ->setValue(ui->messageBrowser->verticalScrollBar()->maximum());     qDebug("sendmsg");
        break;

    case NewParticipant :
        out << address;

        break;

    case ParticipantLeft :        
        break;

    case FileName :{
        int current_row = ui->userTableWidget->currentRow();
        QString clientAddress = ui->userTableWidget->item(current_row, 2)->text();
        out << address << clientAddress << fileName;
        break;
    }
    case Refuse :
        out << serverAddress;
        break;
    }
    udpSocket->writeDatagram(data,data.length(),ip, port);qDebug("sendsocket");
}

// 接收UDP信息
void Widget::processPendingDatagrams()
{
    while(udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size());
        QDataStream in(&datagram, QIODevice::ReadOnly);
        int messageType;
        in >> messageType;
        QString userName,localHostName,ipAddress,message;
        QString time = QDateTime::currentDateTime()
                .toString("yyyy-MM-dd hh:mm:ss");

        switch(messageType)
        {
        case Message:
            in >> userName >> localHostName >> ipAddress >> message;
            ui->messageBrowser->setTextColor(Qt::blue);
            ui->messageBrowser->setCurrentFont(QFont("Times New Roman",12));
            ui->messageBrowser->append("[ " +userName+" ] "+ time);
            ui->messageBrowser->append(message);

            if (ui->userTableWidget->findItems(localHostName, Qt::MatchExactly).empty())
            insertWidgetContact(localHostName);
            break;

        case NewParticipant:
            in >>userName >>localHostName >>ipAddress;
            newParticipant(userName,localHostName,ipAddress);
            break;

        case ParticipantLeft:
            in >>userName >>localHostName;
            participantLeft(userName,localHostName,time);
            break;

        case FileName:{
            in >> userName >> localHostName >> ipAddress;
            QString clientAddress, fileName;
            in >> clientAddress >> fileName;
            hasPendingFile(userName, ipAddress, clientAddress, fileName);
            break;
        }

        case Refuse: in >> userName >> localHostName;
            QString serverAddress;
            in >> serverAddress;
            QString ipAddress = getIP();

            if(ipAddress == serverAddress)
            {
                server->refused();
            }
            break;
        }
    }
}

// 处理新用户加入
void Widget::newParticipant(QString userName, QString localHostName, QString ipAddress)
{
    bool isEmpty = ui->userTableWidget->findItems(localHostName, Qt::MatchExactly).isEmpty();
    if (isEmpty) {
        QTableWidgetItem *user = new QTableWidgetItem(userName);
        QTableWidgetItem *host = new QTableWidgetItem(localHostName);
        QTableWidgetItem *ip = new QTableWidgetItem(ipAddress);
        QTableWidgetItem *check= new QTableWidgetItem();
            check->setCheckState(Qt::Unchecked);

        ui->userTableWidget->insertRow(0); //插入新行
        ui->userTableWidget->setItem(0,0,user);
        ui->userTableWidget->setItem(0,1,host);
        ui->userTableWidget->setItem(0,2,ip);
        ui->userTableWidget->setItem(0,3,check);
        ui->messageBrowser->setTextColor(Qt::gray);
        ui->messageBrowser->setCurrentFont(QFont("Times New Roman",10));
        ui->messageBrowser->append(tr("%1 在线").arg(userName));
        ui->userNumLabel->setText(tr("在线用户:%1").arg(ui->userTableWidget->rowCount()));
        sendMessage(NewParticipant,"broadcast");//将本机的状态以新加入的类型发送给新加入主机以便新加入主机更新自己的用户在线状态
    }
}

// 处理用户离开
void Widget::participantLeft(QString userName, QString localHostName, QString time)
{
    bool isEmpty = ui->userTableWidget->findItems(localHostName, Qt::MatchExactly).isEmpty();
    if (isEmpty)
        {return;}
    else{
    int rowNum = ui->userTableWidget->findItems(localHostName, Qt::MatchExactly).first()->row();
    ui->userTableWidget->removeRow(rowNum);
    ui->messageBrowser->setTextColor(Qt::gray);
    ui->messageBrowser->setCurrentFont(QFont("Times New Roman", 10));
    ui->messageBrowser->append(tr("%1 leaving at %2").arg(userName).arg(time));
    ui->userNumLabel->setText(tr("在线用户:%1").arg(ui->userTableWidget->rowCount()));}
}

// 获取ip地址
QString Widget::getIP()
{
    /*QList<QHostAddress> list = QNetworkInterface::allAddresses();
    foreach (QHostAddress address, list) {
        if(address.protocol() == QAbstractSocket::IPv4Protocol)
            return address.toString();
    }*/
    /*{   QString localhostName="www.baidu.com";
        QHostInfo info=QHostInfo::fromName(localhostName);
        foreach(QHostAddress address,info.addresses())
        {
          if(address.protocol()==QAbstractSocket::IPv4Protocol)
            qDebug()<<address.toString(); //Êä³öIPV4µÄµØÖ·;
        }
    }*/
    return QNetworkInterface().allAddresses().at(2).toString();
    return 0;
}

// 获取用户名
QString Widget::getUserName()
{
    QStringList envVariables;
    envVariables << "USERNAME.*" << "USER.*" << "USERDOMAIN.*"
                 << "HOSTNAME.*" << "DOMAINNAME.*";
    QStringList environment = QProcess::systemEnvironment();
    foreach (QString string, envVariables) {
        int index = environment.indexOf(QRegExp(string));
        if (index != -1) {
            QStringList stringList = environment.at(index).split('=');
            if (stringList.size() == 2) {
                return stringList.at(1);
                break;
            }
        }
    }
    return "unknown";
}

// 获取要发送的消息
QString Widget::getMessage()
{
    QString msg = ui->messageTextEdit->toHtml();
    //QString time = QDateTime::currentDateTime()
      //      .toString("yyyy-MM-dd hh:mm:ss");

    ui->messageTextEdit->setFocus();
    //ui->messageBrowser->setTextColor(Qt::red);
   // ui->messageBrowser->setCurrentFont(QFont("Times New Roman",12));
    //ui->messageBrowser->append("[ " +getUserName()+" ] "+ time);
    //ui->messageBrowser->append(msg);
    return msg;
}

// 发送消息
void Widget::on_sendButton_clicked()
{
    if(ui->userTableWidget->rowCount()==0)
    {
        QMessageBox::warning(0, "选择用户",
                       "请向用户列表中添加联系人！", QMessageBox::Ok);
        return;
    }
    if(ui->checkGroupBox->checkState()==Qt::Checked)
    {       
        for(int i=0;i<ui->userTableWidget->rowCount();i++)
        {
            if(!(ui->userTableWidget->item(i,0)->text()=="Contact"))
            sendMessage(Message,ui->userTableWidget->item(i,2)->text());
        }
    }else
    {
        int hostNum = ui->userTableWidget->findItems(getIP(), Qt::MatchExactly).first()->row();
        ui->userTableWidget->item(hostNum,3)->setCheckState(Qt::Checked);
        for(int i=0;i<ui->userTableWidget->rowCount();i++)
        {
            if(ui->userTableWidget->item(i,3)->checkState()==Qt::Checked)
                sendMessage(Message,ui->userTableWidget->item(i,2)->text());
        }
    }
    ui->messageTextEdit->clear();
}

//切换局域网群聊与联系人私聊
void Widget::on_checkGroupBox_clicked()
{
    if(ui->checkGroupBox->checkState()==Qt::Checked)
    {
        sendMessage(NewParticipant,"broadcast");
        saveFile("/home/lovemi/Desktop/privateChatHistory");
    }else if(ui->checkGroupBox->checkState()==Qt::Unchecked)
    {
        sendMessage(ParticipantLeft,"broadcast");
        saveFile("/home/lovemi/Desktop/groupChatHistory");
    }
    ui->messageBrowser->clear();
}

//获取webIP
void Widget::getWebIP()
{
        QNetworkAccessManager *manager = new QNetworkAccessManager();
        QNetworkReply *reply = manager->get(QNetworkRequest(QUrl("http://1111.ip138.com/ic.asp")));
        QByteArray responseData;
        QEventLoop eventLoop;
        QObject::connect(manager, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
        eventLoop.exec();
        responseData = reply->readAll();
        QString ipMsg;
        ui->webIPLabel->setText(ipMsg.fromLocal8Bit(responseData));
}

//手动添加联系人,adding object on row(row_count)
void Widget::insertWidgetContact(QString localHostName)
{
    QTableWidgetItem *userType = new QTableWidgetItem("Contact");
    QTableWidgetItem *host = new QTableWidgetItem(localHostName);
    QTableWidgetItem *ip = new QTableWidgetItem("输入联系人IP地址");
    QTableWidgetItem *check= new QTableWidgetItem();
        check->setCheckState(Qt::Unchecked);
    int row_count = ui->userTableWidget->rowCount(); //获取表单行数
    ui->userTableWidget->insertRow(row_count);
    ui->userTableWidget->setItem(row_count,0,userType);
    ui->userTableWidget->setItem(row_count,1,host);
    ui->userTableWidget->setItem(row_count,2,ip);
    ui->userTableWidget->setItem(row_count,3,check);
}

//关闭窗口
void Widget::on_closeButton_clicked()
{
    sendMessage(ParticipantLeft,"broadcast");
    QWidget::close();
}

//传输文件Button
void Widget::on_sendFileBtn_clicked()
{
    if(ui->userTableWidget->selectedItems().isEmpty())
    {
        QMessageBox::warning(0, "选择用户",
                       "请先从用户列表选择要传送的用户！", QMessageBox::Ok);
        return;
    }
    server->show();
    server->initServer();
}

//获取要发送的文件名
void Widget::getFileName(QString name)
{
    fileName = name;
    int row = ui->userTableWidget->selectedItems().first()->row();
    QString clientIP = ui->userTableWidget->item(row,2)->text();
    sendMessage(FileName,clientIP);
}

//是否接收文件
void Widget::hasPendingFile(QString userName, QString serverAddress,
                            QString clientAddress, QString fileName)
{
    QString ipAddress = getIP();
    if(ipAddress == clientAddress)
    {
        int btn = QMessageBox::information(this,tr("接受文件"),
                                           tr("来自%1(%2)的文件：%3,是否接收？")
                                           .arg(userName).arg(serverAddress).arg(fileName),
                                           QMessageBox::Yes,QMessageBox::No);
        if (btn == QMessageBox::Yes) {
            QString name = QFileDialog::getSaveFileName(0,"保存文件",fileName);
            if(!name.isEmpty())
            {
                TcpClient *client = new TcpClient(this);
                client->setFileName(name);
                client->setHostAddress(QHostAddress(serverAddress));
                client->show();
            }
        } else {
            sendMessage(Refuse, serverAddress);
        }
    }
}


//添加联系人按键
void Widget::on_addContactBtn_clicked()
{
    insertWidgetContact("输入联系人主机名");
}

//移除联系人
void Widget::on_removeContactBtn_clicked()
{
    if(ui->userTableWidget->selectedItems().isEmpty())
    {
        QMessageBox::warning(0, "选择用户",
                       "请先从用户列表选择要移除的用户！", QMessageBox::Ok);
        return;
    }
    int row=ui->userTableWidget->selectedItems().first()->row();
    ui->userTableWidget->removeRow(row);
}

//保存聊天记录
void Widget::saveFile(QString fileName)
{
    QFile file(fileName);
    if(!file.open(QFile::WriteOnly | QFile::Text | QIODevice::Append))
    {
        QMessageBox::warning(this,QString("保存聊天记录"),
                             QString("无法保存聊天记录 %1: \n %2").arg(fileName).arg(file.errorString()));
    }
    QTextStream out(&file);
    out<<ui->messageBrowser->toHtml();
    out<<"\n";
}

//open the browser to view the chatting history
void Widget::on_readChatBtn_clicked()
{
    readChatBtn = new Dialog(this);
    readChatBtn->show();
}

//set the broadcast addr for group chatting
void Widget::on_pushButton_clicked()
{
    QString addr = ui->lineEdit->text();
    broadcastAddr = addr;
    sendMessage(ParticipantLeft,"broadcast");
    sendMessage(NewParticipant,"broadcast");
}

//change font type
void Widget::on_fontComboBox_currentFontChanged(const QFont &f)
{
    ui->messageTextEdit->setCurrentFont(f);
    ui->messageTextEdit->setFocus();
}

//change font size
void Widget::on_sizeComboBox_currentIndexChanged(QString size)
{
    ui->messageTextEdit->setFontPointSize(size.toDouble());
    ui->messageTextEdit->setFocus();
}

//text bold
void Widget::on_boldBtn_clicked(bool checked)
{
    if(checked)
        ui->messageTextEdit->setFontWeight(QFont::Bold);
    else
        ui->messageTextEdit->setFontWeight(QFont::Normal);
    ui->messageTextEdit->setFocus();
}

//text italic
void Widget::on_italicBtn_clicked(bool checked)
{
    ui->messageTextEdit->setFontItalic(checked);
    ui->messageTextEdit->setFocus();
}

//underline the text
void Widget::on_underlineBtn_clicked(bool checked)
{
    ui->messageTextEdit->setFontUnderline(checked);
    ui->messageTextEdit->setFocus();
}

//change font color
void Widget::on_colorlBtn_clicked()
{
    color = QColorDialog::getColor(color,this);
    if(color.isValid()){
        ui->messageTextEdit->setTextColor(color);
        ui->messageTextEdit->setFocus();
    }
}

void Widget::on_saveToolBtn_clicked()
{
    if(ui->checkGroupBox->checkState()==Qt::Checked)
        saveFile(QString("/home/%1/Desktop/groupChatHistory").arg(getUserName()));
    else
        saveFile(QString("/home/%1/Desktop/privateChatHistory").arg(getUserName()));
    ui->messageBrowser->clear();
}
