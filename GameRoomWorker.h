#ifndef GAMEROOMWORKER_H
#define GAMEROOMWORKER_H

#include <QObject>
#include <QTcpSocket>
#include "GameRoom.h"
#include "TcpServer.h"

class GameRoomWorker : public QObject {
    Q_OBJECT
public:
    explicit GameRoomWorker(QObject *parent = nullptr, QTcpSocket* ownerSocket = nullptr, TcpServer* tcpServer = nullptr);
    ~GameRoomWorker();

signals:
    void roomCreated(GameRoom* newRoom);
    void errorOccurred(QTcpSocket* ownerSocket, const QString& errorMessage);

public slots:
    void createRoom(int capacity);

private:
    QTcpSocket* ownerSocket;
    TcpServer* tcpServer;
};

#endif // GAMEROOMWORKER_H
