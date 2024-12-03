#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QThreadPool>
#include <QTcpSocket>
#include <QJsonObject>
#include "UserManager.h"
#include "RoomManager.h"

class TcpServer : public QTcpServer {
    Q_OBJECT
public:
    TcpServer();
    void StartListen(int port);
    void CloseServer();

private slots:
    void newConnection_Slot();
    void readyRead_Slot();
    void stateChanged_Slot(QAbstractSocket::SocketState socketState);

private:
    QTcpServer *tcpServer;
    UserManager *userManager;
    RoomManager *roomManager;
    QByteArray buffer;
    QThreadPool *threadPool;

    void handleMessage(const QJsonObject& jsonObj, QTcpSocket* tcpSocket);
};

#endif // TCPSERVER_H
