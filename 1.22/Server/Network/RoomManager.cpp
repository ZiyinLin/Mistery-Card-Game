#include "RoomManager.h"
#include "GameRoom.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QDebug>
#include <QMutex>
#include <QMetaObject>
#include <QDateTime>

RoomManager::RoomManager(QObject *parent) : QObject(parent)
{
    heartbeatCheckTimer = new QTimer(this);
    connect(heartbeatCheckTimer, &QTimer::timeout, this, &RoomManager::checkHeartbeats);
    heartbeatCheckTimer->start(10000);  // 每10秒检查一次心跳
}

QString RoomManager::generateUniqueRoomCode()
{
    qDebug() << "\n=== Starting Room Code Generation ===";
    qDebug() << "RoomManager: Generating unique room code...";
    
    // 使用时间戳生成简单的房间码
    qint64 timestamp = QDateTime::currentMSecsSinceEpoch();
    static int counter = 0;
    
    // 取时间戳后4位和计数器后2位，组成6位房间码
    QString roomCode = QString("%1%2")
        .arg(timestamp % 10000, 4, 10, QChar('0'))    // 取时间戳后4位，不足补0
        .arg(++counter % 100, 2, 10, QChar('0'));     // 取计数器后2位，不足补0
    
    qDebug() << "RoomManager: Generated timestamp:" << timestamp;
    qDebug() << "RoomManager: Counter value:" << counter;
    qDebug() << "RoomManager: Final room code:" << roomCode;
    qDebug() << "=== Room Code Generation Complete ===\n";
    
    return roomCode;
}

void RoomManager::handleCreateRoom(const QJsonObject& jsonObj, QTcpSocket* tcpSocket)
{
    qDebug() << "RoomManager: Handling create room request";
    
    // 确保在主线程中执行
    if (QThread::currentThread() != thread()) {
        qDebug() << "RoomManager: Invoking in main thread";
        QMetaObject::invokeMethod(this, "handleCreateRoom",
                                Qt::BlockingQueuedConnection,
                                Q_ARG(QJsonObject, jsonObj),
                                Q_ARG(QTcpSocket*, tcpSocket));
        return;
    }

    QMutexLocker locker(&roomsMutex);
    int capacity = jsonObj.value("capacity").toInt();
    qDebug() << "RoomManager: Creating room with capacity:" << capacity;
    
    QJsonObject response;
    response.insert("type", "create_room");

    try {
        if (capacity < 3 || capacity > 8) {
            qDebug() << "RoomManager: Invalid capacity";
            response.insert("status", "error");
            response.insert("message", "Invalid room capacity. Must be between 3 and 8.");
            emit roomCreationFailed(tcpSocket, response.value("message").toString());
        } else {
            qDebug() << "RoomManager: About to generate room code...";
            QString roomCode = generateUniqueRoomCode();
            qDebug() << "RoomManager: Generated room code:" << roomCode;
            
            GameRoom* newRoom = new GameRoom(this, capacity, roomCode);
            newRoom->setOwner(tcpSocket);
            newRoom->addPlayer(tcpSocket);
            rooms.insert(roomCode, newRoom);

            response.insert("status", "success");
            response.insert("room_code", roomCode);
            
            QJsonObject joinNotification;
            joinNotification.insert("type", "player_joined");
            joinNotification.insert("player", tcpSocket->property("username").toString());
            joinNotification.insert("is_owner", true);
            joinNotification.insert("player_count", 1);
            tcpSocket->write(QJsonDocument(joinNotification).toJson(QJsonDocument::Compact) + "\n");
            
            qDebug() << "RoomManager: Created response object:" << response;
        }

        // 创建响应并发送
        QJsonDocument responseDoc(response);
        QByteArray responseData = responseDoc.toJson(QJsonDocument::Compact) + "\n";
        qDebug() << "RoomManager: Final response data:" << QString(responseData);

        // 确保数据被写入并立即发送
        if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
            qint64 bytesWritten = tcpSocket->write(responseData);
            tcpSocket->waitForBytesWritten(); // 等待数据写入
            bool flushed = tcpSocket->flush();
            qDebug() << "RoomManager: Socket state:" << tcpSocket->state();
            qDebug() << "RoomManager: Wrote" << bytesWritten << "bytes to socket, flush result:" << flushed;
            
            if (bytesWritten == responseData.size()) {
                qDebug() << "RoomManager: Response sent successfully";
                if (response.value("status").toString() == "success") {
                    emit roomCreated(tcpSocket, response.value("room_code").toString());
                }
            } else {
                qDebug() << "RoomManager: Failed to send complete response";
            }
        } else {
            qDebug() << "RoomManager: Socket is not connected! State:" << tcpSocket->state();
        }
    } catch (const std::exception& e) {
        qDebug() << "RoomManager: Exception occurred:" << e.what();
    } catch (...) {
        qDebug() << "RoomManager: Unknown exception occurred";
    }
}

void RoomManager::handleJoinRoom(const QJsonObject& jsonObj, QTcpSocket* tcpSocket)
{
    qDebug() << "\n=== Starting Room Join Process ===";
    qDebug() << "RoomManager: Attempting to join room with request:" << jsonObj;
    
    // 确保在主线程中执行
    if (QThread::currentThread() != thread()) {
        qDebug() << "RoomManager: Invoking in main thread";
        QMetaObject::invokeMethod(this, "handleJoinRoom",
                                Qt::BlockingQueuedConnection,
                                Q_ARG(QJsonObject, jsonObj),
                                Q_ARG(QTcpSocket*, tcpSocket));
        return;
    }

    QMutexLocker locker(&roomsMutex);
    QString roomCode = jsonObj.value("room_code").toString();
    qDebug() << "RoomManager: Looking for room with code:" << roomCode;

    QJsonObject response;
    response.insert("type", "join_room");

    GameRoom* room = rooms.value(roomCode);
    if (!room) {
        qDebug() << "RoomManager: Room not found!";
        response.insert("status", "error");
        response.insert("message", "Room does not exist");
        emit roomJoinFailed(tcpSocket, "Room does not exist");
    } else if (room->isFull()) {
        qDebug() << "RoomManager: Room is full!";
        response.insert("status", "error");
        response.insert("message", "Room is full");
        emit roomJoinFailed(tcpSocket, "Room is full");
    } else {
        qDebug() << "RoomManager: Adding player to room";
        room->addPlayer(tcpSocket);
        response.insert("status", "success");
        response.insert("room_code", roomCode);

        // 添加当前房间玩家列表
        QJsonArray playersArray;
        foreach (const QString& playerName, room->getPlayerNames()) {
            playersArray.append(playerName);
        }
        response.insert("players", playersArray);
        qDebug() << "RoomManager: Current players in room:" << playersArray;
        
        // 通知房间其他玩家有新玩家加入
        QString username = tcpSocket->property("username").toString();
        QString joinMessage = QString("%1 has joined the room").arg(username);
        qDebug() << "RoomManager: Broadcasting join message:" << joinMessage;
        sendMessageToRoom(roomCode, joinMessage);

        emit roomJoined(tcpSocket, roomCode);

        lastHeartbeats[tcpSocket] = QDateTime::currentDateTime();  // 录初始心跳时间
    }

    // 发送响应
    QJsonDocument responseDoc(response);
    QByteArray responseData = responseDoc.toJson(QJsonDocument::Compact) + "\n";
    qDebug() << "RoomManager: Sending response to client:" << QString(responseData);

    if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
        qint64 bytesWritten = tcpSocket->write(responseData);
        bool flushed = tcpSocket->flush();
        qDebug() << "RoomManager: Socket state:" << tcpSocket->state();
        qDebug() << "RoomManager: Wrote" << bytesWritten << "bytes to socket, flush result:" << flushed;
    } else {
        qDebug() << "RoomManager: Socket is not connected! State:" << tcpSocket->state();
    }
    
    qDebug() << "=== Room Join Process Complete ===\n";
}

void RoomManager::handleKickPlayer(const QJsonObject& jsonObj, QTcpSocket* tcpSocket)
{
    QMutexLocker locker(&roomsMutex);
    QString playerToKick = jsonObj.value("player").toString();
    QString currentUsername = tcpSocket->property("username").toString();
    
    // 查找发起踢人请求的玩家所在的房间
    GameRoom* room = nullptr;
    QString roomCode;
    for (auto it = rooms.begin(); it != rooms.end(); ++it) {
        if (it.value()->getPlayers().contains(tcpSocket)) {
            room = it.value();
            roomCode = it.key();
            break;
        }
    }
    
    QJsonObject response;
    response.insert("type", "kick_response");
    
    if (!room) {
        response.insert("status", "error");
        response.insert("message", "You are not in any room");
        tcpSocket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
        return;
    }
    
    // 检查是否是房主
    if (room->getOwner() != tcpSocket) {
        response.insert("status", "error");
        response.insert("message", "Only room owner can kick players");
        tcpSocket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
        return;
    }
    
    // 查找要踢出的玩家
    QTcpSocket* targetSocket = nullptr;
    for (QTcpSocket* player : room->getPlayers()) {
        if (player->property("username").toString() == playerToKick) {
            targetSocket = player;
            break;
        }
    }
    
    if (!targetSocket) {
        response.insert("status", "error");
        response.insert("message", "Player not found in room");
        tcpSocket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
        return;
    }
    
    // 执行踢人操作
    if (room->kickPlayer(targetSocket)) {
        // 发送通知给被踢玩家（只发送一次）
        QJsonObject kickNotification;
        kickNotification.insert("type", "kicked");
        targetSocket->write(QJsonDocument(kickNotification).toJson(QJsonDocument::Compact) + "\n");
        targetSocket->flush();
        
        // 发送成功响应给房主
        response.insert("status", "success");
        response.insert("player", playerToKick);
        tcpSocket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
        
        // 广播消息给房间内其他玩家
        QJsonObject notification;
        notification.insert("type", "player_kicked");
        notification.insert("player", playerToKick);
        notification.insert("room_code", roomCode);
        
        for (QTcpSocket* player : room->getPlayers()) {
            if (player != tcpSocket && player != targetSocket) {
                player->write(QJsonDocument(notification).toJson(QJsonDocument::Compact) + "\n");
            }
        }
        
        // 处理被踢玩家的断开连接
        QTimer::singleShot(500, this, [this, targetSocket]() {
            handlePlayerDisconnect(targetSocket);
        });
    }
}

void RoomManager::handlePlayerDisconnect(QTcpSocket* socket)
{
    // 1. 基础检查
    if (!socket) {
        qDebug() << "Server: [CRITICAL] Null socket in handlePlayerDisconnect";
        return;
    }

    QString username = socket->property("username").toString();
    if (username.isEmpty()) {
        qDebug() << "Server: [WARNING] Socket has no username property";
        lastHeartbeats.remove(socket);
        return;
    }

    qDebug() << "\n=== Starting Disconnect Handling ===";
    qDebug() << "Server: Processing disconnect for client:" << username;

    // 2. 从心跳列表移除
    lastHeartbeats.remove(socket);
    qDebug() << "Server: Removed from heartbeat list";

    // 3. 查找玩家所在的房间
    GameRoom* playerRoom = nullptr;
    QString roomCode;
    {
        QMutexLocker locker(&roomsMutex);
        for (auto it = rooms.begin(); it != rooms.end(); ++it) {
            if (it.value()->getPlayers().contains(socket) || 
                it.value()->getSpectators().contains(socket)) {
                playerRoom = it.value();
                roomCode = it.key();
                break;
            }
        }
    }

    if (playerRoom) {
        qDebug() << "Server: Found player in room" << roomCode;

        // 4. 从房间移除玩家
        bool wasPlayer = playerRoom->getPlayers().contains(socket);
        if (wasPlayer) {
            playerRoom->removePlayer(socket);
            
            // 如果房间中没有真实玩家了，就删除房间（即使还有观察者）
            if (playerRoom->getPlayers().isEmpty()) {
                qDebug() << "Server: Room is empty, removing room" << roomCode;
                
                // 通知所有观察者房间已关闭（只发送一次）
                QJsonObject notification;
                notification.insert("type", "room_closed");
                notification.insert("message", "Room has been closed as all players left");
                
                QByteArray notificationData = QJsonDocument(notification).toJson(QJsonDocument::Compact) + "\n";
                for (QTcpSocket* spectator : playerRoom->getSpectators()) {
                    spectator->write(notificationData);
                    spectator->flush();
                }
                
                // 删除房间
                rooms.remove(roomCode);
                playerRoom->deleteLater();
            } else {
                // 如果还有其他玩家，更新房间状态
                playerRoom->updateRoomStatus();
            }
        } else {
            // 如果是观察者离开
            playerRoom->removeSpectator(socket);
            playerRoom->updateRoomStatus();
        }
    }

    qDebug() << "Server: Disconnect handling completed for client" << username;
}

void RoomManager::sendMessageToRoom(const QString& roomCode, const QString& message)
{
    qDebug() << "RoomManager: Sending message to room" << roomCode << ":" << message;
    GameRoom* room = rooms.value(roomCode);
    if (!room) {
        qDebug() << "RoomManager: Room not found for message broadcast!";
        return;
    }

    QJsonObject messageObj;
    messageObj.insert("type", "room_message");
    messageObj.insert("message", message);
    QJsonDocument messageDoc(messageObj);
    QByteArray messageData = messageDoc.toJson(QJsonDocument::Compact) + "\n";

    foreach (QTcpSocket* player, room->getPlayers()) {
        if (player->state() == QAbstractSocket::ConnectedState) {
            qDebug() << "RoomManager: Sending message to player:" << player->property("username").toString();
            player->write(messageData);
            player->flush();
        }
    }
}

void RoomManager::cleanupRooms()
{
    QMutableHashIterator<QString, GameRoom*> i(rooms);
    while (i.hasNext()) {
        i.next();
        if (i.value()->getPlayers().isEmpty()) {
            delete i.value();
            i.remove();
        }
    }
}

void RoomManager::handleHeartbeat(QTcpSocket* socket)
{
    if (!socket) {
        qDebug() << "Server: [WARNING] Received heartbeat from null socket";
        return;
    }

    QString username = socket->property("username").toString();
    if (!username.isEmpty()) {
        QMutexLocker locker(&roomsMutex);
        if (socket->state() == QAbstractSocket::ConnectedState) {
            lastHeartbeats[socket] = QDateTime::currentDateTime();
            qDebug() << "Server: Received heartbeat from" << username;
        } else {
            lastHeartbeats.remove(socket);
            qDebug() << "Server: Removed disconnected socket from heartbeat list for" << username;
        }
    }
}

void RoomManager::checkHeartbeats()
{
    try {
        QDateTime currentTime = QDateTime::currentDateTime();
        
        // 创建一个需要断开连接的socket列表
        QList<QTcpSocket*> socketsToDisconnect;
        
        {
            QMutexLocker locker(&roomsMutex);
            
            qDebug() << "\n=== Starting Heartbeat Check ===";
            qDebug() << "Server: Thread ID:" << QThread::currentThreadId();
            qDebug() << "Server: Number of monitored clients:" << lastHeartbeats.size();
            qDebug() << "Server: Current time:" << currentTime.toString();
            
            // 使用迭代器安全地遍历心跳列表
            QMutableHashIterator<QTcpSocket*, QDateTime> it(lastHeartbeats);
            while (it.hasNext()) {
                it.next();
                QTcpSocket* socket = it.key();
                
                // 检查socket是否有效
                if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
                    qDebug() << "Server: Removing invalid socket from heartbeat list";
                    it.remove();
                    continue;
                }
                
                QString username = socket->property("username").toString();
                if (username.isEmpty()) {
                    qDebug() << "Server: Removing socket without username from heartbeat list";
                    it.remove();
                    continue;
                }
                
                int secondsSinceLastHeartbeat = it.value().secsTo(currentTime);
                qDebug() << "Server: Client" << username 
                         << "last heartbeat:" << secondsSinceLastHeartbeat << "seconds ago"
                         << "at" << it.value().toString();
                
                // 如果超过15秒没有心跳，将socket加入待断开列表
                if (secondsSinceLastHeartbeat > 15) {
                    qDebug() << "Server: Client" << username << "heartbeat timeout";
                    socketsToDisconnect.append(socket);
                    it.remove();
                }
            }
        }
        
        // 在锁外处理需要断开的连接
        foreach(QTcpSocket* socket, socketsToDisconnect) {
            if (socket) {
                QString username = socket->property("username").toString();
                qDebug() << "Server: Disconnecting client" << username << "due to heartbeat timeout";
                
                // 先处理断开连接的逻辑
                handlePlayerDisconnect(socket);
                
                // 然后关闭socket
                socket->disconnectFromHost();
                
                // 使用定时器延删除socket
                QTimer::singleShot(1000, this, [socket]() {
                    if (socket) {
                        socket->deleteLater();
                    }
                });
            }
        }
        
        qDebug() << "=== Heartbeat Check Complete ===\n";
        
    } catch (const std::exception& e) {
        qDebug() << "Server: [CRITICAL] Exception in checkHeartbeats:" << e.what();
    } catch (...) {
        qDebug() << "Server: [CRITICAL] Unknown exception in checkHeartbeats";
    }
}

void RoomManager::handleTransferOwnership(const QJsonObject& jsonObj, QTcpSocket* tcpSocket)
{
    QMutexLocker locker(&roomsMutex);
    QString newOwnerName = jsonObj.value("new_owner").toString();
    QString currentUsername = tcpSocket->property("username").toString();
    
    // 查找发起转让请求的玩家所在的房间
    GameRoom* room = nullptr;
    QString roomCode;
    for (auto it = rooms.begin(); it != rooms.end(); ++it) {
        if (it.value()->getPlayers().contains(tcpSocket)) {
            room = it.value();
            roomCode = it.key();
            break;
        }
    }
    
    QJsonObject response;
    response.insert("type", "transfer_response");
    
    if (!room) {
        response.insert("status", "error");
        response.insert("message", "You are not in any room");
        tcpSocket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
        return;
    }
    
    // 检查是否是房主
    if (room->getOwner() != tcpSocket) {
        response.insert("status", "error");
        response.insert("message", "Only room owner can transfer ownership");
        tcpSocket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
        return;
    }
    
    // 查找新房主
    QTcpSocket* newOwnerSocket = nullptr;
    for (QTcpSocket* player : room->getPlayers()) {
        if (player->property("username").toString() == newOwnerName) {
            newOwnerSocket = player;
            break;
        }
    }
    
    if (!newOwnerSocket) {
        response.insert("status", "error");
        response.insert("message", "Player not found in room");
        tcpSocket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
        return;
    }
    
    // 执行转让操作
    room->setOwner(newOwnerSocket);
    
    // 发送成功响应给原房主
    response.insert("status", "success");
    response.insert("new_owner", newOwnerName);
    tcpSocket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
    
    // 广播消息给房间内所有玩家
    QJsonObject notification;
    notification.insert("type", "ownership_transferred");
    notification.insert("previous_owner", currentUsername);
    notification.insert("new_owner", newOwnerName);
    
    for (QTcpSocket* player : room->getPlayers()) {
        player->write(QJsonDocument(notification).toJson(QJsonDocument::Compact) + "\n");
    }
}

void RoomManager::handleSpectateRequest(const QJsonObject& jsonObj, QTcpSocket* tcpSocket)
{
    QMutexLocker locker(&roomsMutex);
    QString roomCode = jsonObj.value("room_code").toString();
    QString username = tcpSocket->property("username").toString();
    
    QJsonObject response;
    response.insert("type", "spectate_response");

    GameRoom* room = rooms.value(roomCode);
    if (!room) {
        response.insert("status", "error");
        response.insert("message", "Room does not exist");
        tcpSocket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
        tcpSocket->flush();
        emit spectateFailed(tcpSocket, "Room does not exist");
        return;  // 直接返回，不继续执行
    }

    // 添加为观察者而不是玩家
    room->addSpectator(tcpSocket);
    response.insert("status", "success");
    response.insert("room_code", roomCode);
    
    // 发送响应
    tcpSocket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
    tcpSocket->flush();
    
    // 通知房间其他人有新的观察者加入
    QJsonObject notification;
    notification.insert("type", "spectator_joined");
    notification.insert("spectator", username);
    
    // 广播给房间内的玩家
    for (QTcpSocket* player : room->getPlayers()) {
        player->write(QJsonDocument(notification).toJson(QJsonDocument::Compact) + "\n");
    }
    
    // 立即更新并广播房间状态
    room->updateRoomStatus();
    
    emit spectateSuccess(tcpSocket, roomCode);
}

GameRoom* RoomManager::findPlayerRoom(QTcpSocket* playerSocket)
{
    QMutexLocker locker(&roomsMutex);
    for (auto it = rooms.begin(); it != rooms.end(); ++it) {
        if (it.value()->getPlayers().contains(playerSocket)) {
            return it.value();
        }
    }
    return nullptr;
} 