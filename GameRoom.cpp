#include "GameRoom.h"

GameRoom::GameRoom(QObject *parent, int capacity, QString roomCode)
    : QObject(parent), roomCapacity(capacity), roomCode(roomCode.isEmpty() ? "default_code" : roomCode)
{
    if (owner == nullptr && !players.isEmpty()) {
        owner = players.first();
    }
    // 其他构造函数的初始化代码
}

int GameRoom::getCapacity() const
{
    return roomCapacity;
}

bool GameRoom::isFull() const
{
    return players.size() >= roomCapacity;
}

void GameRoom::addPlayer(QTcpSocket* playerSocket)
{
    if (!isFull()) {
        players.append(playerSocket);
        if (owner == nullptr) {
            owner = playerSocket;
        }
    }
}

bool GameRoom::removePlayer(QTcpSocket* playerSocket)
{
    if (players.contains(playerSocket)) {
        players.removeAll(playerSocket);
        if (playerSocket == owner) {
            owner = players.isEmpty() ? nullptr : players.first();
        }
        return true;
    }
    return false;
}

bool GameRoom::kickPlayer(QTcpSocket* playerSocket)
{
    if (playerSocket != owner && players.contains(playerSocket)) {
        players.removeAll(playerSocket);
        return true;
    }
    return false;
}

QTcpSocket* GameRoom::getOwner() const
{
    return owner;
}

QList<QTcpSocket*> GameRoom::getPlayers() const
{
    return players;
}

QString GameRoom::getRoomCode() const
{
    return roomCode;
}
QStringList GameRoom::getPlayerNames() const {
    QStringList playerNames;
    foreach (QTcpSocket* player, players) {
        playerNames << player->property("username").toString();
    }
    return playerNames;
}
