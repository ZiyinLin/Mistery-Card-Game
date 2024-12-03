#include "GameRoomWorker.h"
#include "RoomManager.h"

GameRoomWorker::GameRoomWorker(QObject *parent, QTcpSocket* ownerSocket, RoomManager* roomManager)
    : QObject(parent), ownerSocket(ownerSocket), roomManager(roomManager) {}

void GameRoomWorker::createRoom(int capacity) {
    try {
        if (!roomManager) {
            throw std::runtime_error("RoomManager instance is not set.");
        }
        QString roomCode = roomManager->generateUniqueRoomCode();
        GameRoom* newRoom = new GameRoom(nullptr, capacity, roomCode);
        newRoom->addPlayer(ownerSocket);

        emit roomCreated(newRoom);
    } catch (const std::exception& e) {
        emit errorOccurred(ownerSocket, e.what());
    }
}

GameRoomWorker::~GameRoomWorker() {
    // 执行任何需要的清理工作
}
