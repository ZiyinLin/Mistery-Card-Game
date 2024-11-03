#ifndef GAMEROOM_H
#define GAMEROOM_H

#include <QObject>
#include <QList>
#include <QTcpSocket>

class GameRoom : public QObject
{
    Q_OBJECT
public:
    explicit GameRoom(QObject *parent = nullptr, int capacity = 3, QString roomCode = "");
    int getCapacity() const;
    bool isFull() const;
    void addPlayer(QTcpSocket* playerSocket);
    bool removePlayer(QTcpSocket* playerSocket);
    QTcpSocket* getOwner() const;
    bool kickPlayer(QTcpSocket* playerSocket);
    QList<QTcpSocket*> getPlayers() const;
    QString getRoomCode() const;
    QStringList getPlayerNames() const;

private:
    int roomCapacity;
    QList<QTcpSocket*> players;
    QTcpSocket* owner = nullptr;
    QString roomCode;
};

#endif // GAMEROOM_H
