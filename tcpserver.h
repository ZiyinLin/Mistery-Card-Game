#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "GameRoom.h"
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtSql/QSqlDatabase>
#include <QRandomGenerator>

class TcpServer : public QTcpServer
{
    Q_OBJECT
public:
    TcpServer();
    void StartListen(int);
    void CloseServer();
    void SendData(QString);
    void SendDataToRoom(const QString& roomCode, const QString& message);
    void clientDisconnected(QTcpSocket* clientSocket);
    void cleanupRooms();
    QString generateUniqueRoomCode();

private:
    QTcpServer *tcpServer;
    QByteArray buffer;
    QSqlDatabase db;
    void initializeDatabase();
    void handleRegistration(const QJsonObject& jsonObj, QTcpSocket* tcpSocket);
    void handleLogin(const QJsonObject& jsonObj, QTcpSocket* tcpSocket);
    void handleCreateRoom(const QJsonObject& jsonObj, QTcpSocket* tcpSocket);
    void handleJoinRoom(const QJsonObject& jsonObj, QTcpSocket* tcpSocket);
    void handleKickPlayer(const QJsonObject& jsonObj, QTcpSocket* tcpSocket);
    QHash<QString, GameRoom*> rooms;
    QHash<QString, QTcpSocket*> userSockets;

private slots:
    void newConnection_Slot();
    void readyRead_Slot();
    void stateChanged_Slot(QAbstractSocket::SocketState socketState);
    void onRoomCreated(GameRoom* newRoom);
    void onRoomCreationError(QTcpSocket* ownerSocket, const QString& errorMessage);
};

#endif // TCPSERVER_H
