#include "TcpServer.h"
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDebug>
#include <QHostAddress>
#include "RequestHandler.h"

TcpServer::TcpServer()
{
    tcpServer = new QTcpServer(this);
    roomManager = new RoomManager(this);
    userManager = new UserManager(this, roomManager);
    threadPool = new QThreadPool(this);
    
    // 设置线程池最大线程数
    threadPool->setMaxThreadCount(QThread::idealThreadCount());
    
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newConnection_Slot()));
}

void TcpServer::StartListen(int port)
{
    if(!tcpServer->listen(QHostAddress::Any, port))
    {
        qDebug() << "Server failed to start";
    } else {
        qDebug() << "Server started on port" << port;
    }
}

void TcpServer::CloseServer()
{
    tcpServer->close();
}

void TcpServer::newConnection_Slot()
{
    QTcpSocket *tcpSocket = tcpServer->nextPendingConnection();
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(readyRead_Slot()));
    connect(tcpSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), 
            this, SLOT(stateChanged_Slot(QAbstractSocket::SocketState)));

    QString ipaddr = tcpSocket->peerAddress().toString();
    quint16 port = tcpSocket->peerPort();
    qDebug() << "New client connected from:" << ipaddr << ":" << port;
}

void TcpServer::readyRead_Slot()
{
    QTcpSocket *tcpSocket = qobject_cast<QTcpSocket*>(sender());
    if (!tcpSocket) return;

    QByteArray data = tcpSocket->readAll();
    buffer.append(data);

    int endIndex;
    while ((endIndex = buffer.indexOf('\n')) != -1) {
        QByteArray jsonData = buffer.mid(0, endIndex).trimmed();
        buffer.remove(0, endIndex + 1);

        QJsonParseError jsonError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &jsonError);

        if (jsonError.error != QJsonParseError::NoError) {
            qDebug() << "JSON parse error:" << jsonError.errorString();
            continue;
        }

        if (jsonDoc.isObject()) {
            handleMessage(jsonDoc.object(), tcpSocket);
        }
    }
}

void TcpServer::handleMessage(const QJsonObject& jsonObj, QTcpSocket* tcpSocket)
{
    RequestHandler* handler = new RequestHandler(jsonObj, tcpSocket, userManager, roomManager);
    handler->setAutoDelete(true);
    
    // 确保在主线程中创建连接
    QMetaObject::invokeMethod(handler, [this, handler]() {
        threadPool->start(handler);
    }, Qt::QueuedConnection);
}

void TcpServer::stateChanged_Slot(QAbstractSocket::SocketState socketState)
{
    QTcpSocket *tcpSocket = qobject_cast<QTcpSocket*>(sender());
    if (!tcpSocket) return;

    switch(socketState) {
        case QAbstractSocket::UnconnectedState: {
            qDebug() << "Client disconnected";
            
            // 保存需要的信息
            QString username = tcpSocket->property("username").toString();
            
            // 先从用户管理器移除
            userManager->removeUserSocket(tcpSocket);
            
            // 如果用户已登录，从房间移除
            if (!username.isEmpty()) {
                roomManager->handlePlayerDisconnect(tcpSocket);
            }
            
            // 延迟删除socket，给其他操作完成的时间
            QTimer::singleShot(1000, [tcpSocket]() {
                if (tcpSocket) {
                    tcpSocket->deleteLater();
                }
            });
            
            break;
        }
        default:
            break;
    }
}
