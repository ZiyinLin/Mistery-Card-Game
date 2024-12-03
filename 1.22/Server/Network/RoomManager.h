#ifndef ROOMMANAGER_H
#define ROOMMANAGER_H

#include <QObject>
#include <QHash>
#include <QMutex>
#include <QTcpSocket>
#include <QJsonObject>
#include <QThread>
#include <QDateTime>
#include <QTimer>

// 前向声明
class GameRoom;

class RoomManager : public QObject {
    Q_OBJECT
public:
    explicit RoomManager(QObject *parent = nullptr);
    QString generateUniqueRoomCode();
    void sendMessageToRoom(const QString& roomCode, const QString& message);
    GameRoom* findPlayerRoom(QTcpSocket* playerSocket);

public slots:
    void handleCreateRoom(const QJsonObject& jsonObj, QTcpSocket* tcpSocket);
    void handleJoinRoom(const QJsonObject& jsonObj, QTcpSocket* tcpSocket);
    void handleKickPlayer(const QJsonObject& jsonObj, QTcpSocket* tcpSocket);
    void handlePlayerDisconnect(QTcpSocket* socket);
    void checkHeartbeats();
    void handleHeartbeat(QTcpSocket* socket);
    void handleTransferOwnership(const QJsonObject& jsonObj, QTcpSocket* tcpSocket);
    void handleSpectateRequest(const QJsonObject& jsonObj, QTcpSocket* tcpSocket);

signals:
    void roomCreated(QTcpSocket* socket, const QString& roomCode);
    void roomCreationFailed(QTcpSocket* socket, const QString& message);
    void roomJoined(QTcpSocket* socket, const QString& roomCode);
    void roomJoinFailed(QTcpSocket* socket, const QString& message);
    void playerDisconnected(const QString& username);
    void spectateSuccess(QTcpSocket* socket, const QString& roomCode);
    void spectateFailed(QTcpSocket* socket, const QString& message);

private:
    QHash<QString, GameRoom*> rooms;
    QMutex roomsMutex;
    QHash<QTcpSocket*, QDateTime> lastHeartbeats;
    QTimer* heartbeatCheckTimer;
    void cleanupRooms();
};

#endif // ROOMMANAGER_H 