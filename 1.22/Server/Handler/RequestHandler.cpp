#include "RequestHandler.h"
#include <QMetaObject>
#include <QThread>
#include <QCoreApplication>
#include <QDebug>
#include <QJsonDocument>

RequestHandler::RequestHandler(const QJsonObject& req, QTcpSocket* sock, 
                             UserManager* um, RoomManager* rm)
    : request(req), socket(sock), userManager(um), roomManager(rm)
{
    // 确保对象在主线程中
    moveToThread(QCoreApplication::instance()->thread());
    
    // 使用直接连接以确保信号立即处理
    connect(this, &RequestHandler::processLoginRequest, 
            userManager, &UserManager::handleLogin, Qt::DirectConnection);
    connect(this, &RequestHandler::processRegistrationRequest, 
            userManager, &UserManager::handleRegistration, Qt::DirectConnection);
    connect(this, &RequestHandler::processCreateRoomRequest, 
            roomManager, &RoomManager::handleCreateRoom, Qt::DirectConnection);
    connect(this, &RequestHandler::processJoinRoomRequest, 
            roomManager, &RoomManager::handleJoinRoom, Qt::DirectConnection);
    connect(this, &RequestHandler::processKickPlayerRequest, 
            roomManager, &RoomManager::handleKickPlayer, Qt::DirectConnection);
    connect(this, &RequestHandler::processTransferOwnershipRequest, 
            roomManager, &RoomManager::handleTransferOwnership, Qt::DirectConnection);
    connect(this, &RequestHandler::processSpectateRequest,
            roomManager, &RoomManager::handleSpectateRequest, Qt::DirectConnection);
            
    connect(this, &RequestHandler::finished, &eventLoop, &QEventLoop::quit, Qt::DirectConnection);
}

void RequestHandler::run()
{
    QString type = request.value("type").toString();
    qDebug() << "Processing request of type:" << type;

    if (type == "login") {
        emit processLoginRequest(request, socket);
    }
    else if (type == "register") {
        emit processRegistrationRequest(request, socket);
    }
    else if (type == "create_room") {
        qDebug() << "Emitting create room request";
        emit processCreateRoomRequest(request, socket);
    }
    else if (type == "join_room") {
        emit processJoinRoomRequest(request, socket);
    }
    else if (type == "kick_player") {
        emit processKickPlayerRequest(request, socket);
    }
    else if (type == "leave_room") {
        // 先发送响应
        QJsonObject response;
        response.insert("type", "leave_response");
        response.insert("status", "success");
        socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
        socket->flush(); // 确保响应被发送
        
        // 等待一小段时间确保响应被处理
        QThread::msleep(100);
        
        // 然后再处理离开房间
        QString username = socket->property("username").toString();
        roomManager->handlePlayerDisconnect(socket);
    }
    else if (type == "heartbeat") {
        roomManager->handleHeartbeat(socket);
    }
    else if (type == "transfer_ownership") {
        emit processTransferOwnershipRequest(request, socket);
    }
    else if (type == "spectate") {
        emit processSpectateRequest(request, socket);
    }
    else if (type == "player_ready") {
        // 使用 findPlayerRoom 方法来查找玩家所在的房间
        GameRoom* room = roomManager->findPlayerRoom(socket);
        if (room) {
            room->handlePlayerReady(socket);
        }
    }
    else if (type == "start_game") {
        qDebug() << "Processing start game request";
        GameRoom* room = roomManager->findPlayerRoom(socket);
        if (room) {
            qDebug() << "Found room for player";
            if (room->getOwner() == socket) {
                qDebug() << "Player is room owner, starting game...";
                room->startGameLogic();
            } else {
                qDebug() << "Player is not room owner, sending error message";
                QJsonObject response;
                response.insert("type", "start_game_failed");
                response.insert("message", "Only room owner can start the game");
                socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
                socket->flush();
            }
        } else {
            qDebug() << "Room not found for player";
        }
    }
    else if (type == "player_action") {
        GameRoom* room = roomManager->findPlayerRoom(socket);
        if (room) {
            QString username = socket->property("username").toString();
            qDebug() << "\n=== Received Play Card Request ===";
            qDebug() << "From player:" << username;
            qDebug() << "Action data:" << QJsonDocument(request).toJson(QJsonDocument::Compact);
            
            room->handlePlayerAction(request, socket);
        }
    }
    else if (type == "discard_cards") {
        GameRoom* room = roomManager->findPlayerRoom(socket);
        if (room && room->getGameInstance()) {
            int cardId = request.value("card_id").toInt();
            int count = request.value("count").toInt();
            
            // 获取玩家
            QString username = socket->property("username").toString();
            Player* player = room->getGameInstance()->getPlayer(username);
            
            if (player) {
                // 处理弃牌请求
                QMetaObject::invokeMethod(room->getGameInstance(), "processPlayerDiscard", 
                    Qt::BlockingQueuedConnection,
                    Q_ARG(Player*, player),
                    Q_ARG(int, cardId),
                    Q_ARG(int, count));
            }
        }
    }
    else if (type == "chat_message") {
        GameRoom* room = roomManager->findPlayerRoom(socket);
        if (room) {
            // 获取消息内容和发送者
            QString content = request.value("content").toString();
            QString sender = request.value("sender").toString();
            
            // 创建广播消息
            QJsonObject broadcastMsg;
            broadcastMsg.insert("type", "chat_message");
            broadcastMsg.insert("content", content);
            broadcastMsg.insert("sender", sender);
            
            // 广播给房间内所有玩家
            room->broadcastMessage(QJsonDocument(broadcastMsg).toJson(QJsonDocument::Compact) + "\n");
        }
    }
    
    emit finished();
    qDebug() << "Request processing completed for type:" << type;
} 