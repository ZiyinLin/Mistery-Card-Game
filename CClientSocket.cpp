#include "CClientSocket.h"
#include <QHostAddress>
#include <QCoreApplication>
#include <QFile>
#include <QString>
#include <QtXml/QDomDocument>
#include <QXmlStreamReader>
#include <QBuffer>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QTimerEvent>
#include <QVariant>

CClientSocket::CClientSocket(QObject *parent,QString strIP, qint16 iPort)
    : QTcpSocket( parent )
{
    strServerIP = strIP;
    iServerPort = iPort;
    iConnSeconds = 5;
    iTimerId = startTimer(iConnSeconds * 1000);
    connect( this, SIGNAL(readyRead()), this, SLOT(onReadyRead()), Qt::QueuedConnection);
    connect( this, SIGNAL(connected()), this, SLOT(onConnected()));
    connect( this, SIGNAL(disconnected()), this, SLOT(onDisConnected()));
}
CClientSocket::~CClientSocket()
{
    if ( iTimerId != 0 )
        killTimer(iTimerId);
}
void CClientSocket::timerEvent( QTimerEvent *event )
{
    if(iTimerId == event->timerId())
        connectToServer(strServerIP, iServerPort);
}

void CClientSocket::connectToServer( QString ip, quint16 port )
{
    if(ip.contains("localhost", Qt::CaseInsensitive))
        ip="127.0.0.1";
    if(state() == QAbstractSocket::ConnectedState)
    {
        if(ip == strServerIP && port == iServerPort)
            return;
        disconnectFromHost();
    }
    if(state() == QAbstractSocket::UnconnectedState)
    {
        strServerIP = ip;
        iServerPort = port;
        QHostAddress addr(ip);
        connectToHost(addr, port);
    }
}

void CClientSocket::close()
{
    disconnectFromHost();
    QTcpSocket::close();
}

void CClientSocket::onReadyRead() {
    // 读取所有可用数据
    QByteArray data = this->readAll();
    qDebug() << "Received raw data:" << data;

    // 将数据按换行符分割
    QList<QByteArray> jsonMessages = data.split('\n');

    // 遍历每个分割得到的字符串
    for (QByteArray messageData : jsonMessages) {
        if (messageData.isEmpty()) continue; // 忽略空行

        // 尝试将字符串转换为JSON文档
        QJsonDocument doc = QJsonDocument::fromJson(messageData);
        if (doc.isNull()) {
            qDebug() << "Failed to create JSON doc from:" << messageData;
            continue;
        }
        if (!doc.isObject()) {
            qDebug() << "JSON is not an object.";
            continue;
        }

        // 获取JSON对象
        QJsonObject jsonObj = doc.object();
        // 处理JSON对象
        processJsonObject(jsonObj);
    }
}

void CClientSocket::processJsonObject(const QJsonObject &jsonObj) {
    // 根据类型处理JSON对象
    QString type = jsonObj.value("type").toString();
    qDebug() << "Received message type:" << type;

    if (type == "login") {
        // 处理登录响应
        QString status = jsonObj.value("status").toString();
        if (status == "success") {
            emit loginSuccess();
        } else {
            QString message = jsonObj.value("message").toString();
            emit loginFailed(message);
        }
    } else if (type == "registration") {
        // 处理注册响应
        QString status = jsonObj.value("status").toString();
        if (status == "success") {
            emit registrationSuccess();
        } else {
            QString message = jsonObj.value("message").toString();
            emit registrationFailed(message);
        }
    } else if (type == "create_room") {
        // 处理创建房间响应
        QString status = jsonObj.value("status").toString();
        if (status == "success") {
            QString roomCode = jsonObj.value("room_code").toString();
            emit roomCreated(roomCode);
        } else {
            QString message = jsonObj.value("message").toString();
            emit roomCreationFailed(message);
        }
    } else if (type == "join_room") {
        // 处理加入房间响应
        QString status = jsonObj.value("status").toString();
        if (status == "success") {
            QString roomCode = jsonObj.value("room_code").toString();
            emit roomJoined(roomCode);
        } else {
            QString message = jsonObj.value("message").toString();
            emit roomJoinFailed(message);
        }
    } else if (type == "room_message") {
        // 处理房间消息
        QString message = jsonObj.value("message").toString();
        qDebug() << "Room message:" << message;
        // 这里可以添加更多的逻辑，比如显示消息到 UI
    } else {
        // 处理其他类型的消息
        qDebug() << "Received unknown message type:" << type;
    }
}


void CClientSocket::onConnected()
{
    qDebug() << "Connected to server";
}

void CClientSocket::onDisConnected()
{
    qDebug() << "Disconnected from server";
    if (iTimerId == 0) {
        iTimerId = startTimer(iConnSeconds * 1000);
    }
}

bool CClientSocket::socketIsConnected()
{
    return (state() == QAbstractSocket::ConnectedState);
}


void CClientSocket::sendDataToServer(QString str)
{
    if (state() == QAbstractSocket::ConnectedState) {
        write(str.toUtf8() + "\n");
    } else {
        qDebug() << "Cannot send data: socket is not connected.";
    }
}

void CClientSocket::sendLoginRequest(const QString& username, const QString& password)
{
    QJsonObject requestObj;
    requestObj.insert("type", "login");
    requestObj.insert("username", username);
    requestObj.insert("password", password);
    QJsonDocument requestDoc(requestObj);
    QByteArray jsonData = requestDoc.toJson(QJsonDocument::Compact);
    write(jsonData + "\n");
}

void CClientSocket::sendStartGameRequest() {
    QJsonObject requestObj;
    requestObj.insert("type", "start_game");
    QJsonDocument requestDoc(requestObj);
    write(requestDoc.toJson(QJsonDocument::Compact) + "\n");
}

void CClientSocket::sendRegisterRequest(const QString& username, const QString& password)
{
    QJsonObject requestObj;
    requestObj.insert("type", "register");
    requestObj.insert("username", username);
    requestObj.insert("password", password);
    QJsonDocument requestDoc(requestObj);
    QByteArray jsonData = requestDoc.toJson(QJsonDocument::Compact);
    write(jsonData + "\n");
}
void CClientSocket::sendCreateRoomRequest(int capacity)
{
    QJsonObject requestObj;
    requestObj.insert("type", "create_room");
    requestObj.insert("capacity", capacity);
    QJsonDocument requestDoc(requestObj);
    write(requestDoc.toJson(QJsonDocument::Compact) + "\n");
}
void CClientSocket::sendJoinRoomRequest(const QString& roomCode)
{
    QJsonObject requestObj;
    requestObj.insert("type", "join_room");
    requestObj.insert("room_code", roomCode);
    QJsonDocument requestDoc(requestObj);
    QByteArray jsonData = requestDoc.toJson(QJsonDocument::Compact);
    write(jsonData + "\n");
}
