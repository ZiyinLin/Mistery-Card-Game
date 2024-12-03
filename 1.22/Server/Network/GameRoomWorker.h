#ifndef GAMEROOMWORKER_H
#define GAMEROOMWORKER_H

#include <QObject>
#include <QTcpSocket>
#include "GameRoom.h"
#include "RoomManager.h"

class GameRoomWorker : public QObject {
    Q_OBJECT
public:
    explicit GameRoomWorker(QObject *parent = nullptr, QTcpSocket* ownerSocket = nullptr, RoomManager* roomManager = nullptr);
    ~GameRoomWorker();

signals:
    void roomCreated(GameRoom* newRoom);
    void errorOccurred(QTcpSocket* ownerSocket, const QString& errorMessage);

public slots:
    void createRoom(int capacity);

private:
    QTcpSocket* ownerSocket;
    RoomManager* roomManager;
};

#endif // GAMEROOMWORKER_H
