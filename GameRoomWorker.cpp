#include "GameRoomWorker.h"
#include "TcpServer.h"

GameRoomWorker::GameRoomWorker(QObject *parent, QTcpSocket* ownerSocket, TcpServer* tcpServer)
    : QObject(parent), ownerSocket(ownerSocket), tcpServer(tcpServer) {}

void GameRoomWorker::createRoom(int capacity) {
    try {
        if (!tcpServer) {
            throw std::runtime_error("TcpServer instance is not set.");
        }
        QString roomCode = tcpServer->generateUniqueRoomCode();
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
