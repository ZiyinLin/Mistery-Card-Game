#include "GameRoom.h"
#include "../../Server/Game/ServerGame/ServerGame.h"
#include "../../Common/PlayerAction/PlayerAction.h"
#include "../../Client/Game/ClientGame/ClientGame.h"
#include <QJsonDocument>
#include <QDebug>

GameRoom::GameRoom(QObject *parent, int capacity, QString roomCode)
    : QObject(parent), roomCapacity(capacity), roomCode(roomCode.isEmpty() ? "default_code" : roomCode),
      owner(nullptr), m_gameStarted(false), currentPlayerIndex(0), roundNumber(0), turnTimeLimit(30)
{
    // 初始化房间状态
    creationTime = QDateTime::currentDateTime();
    lastActivityTime = creationTime;
    
    // 初始化玩家列表和观察者列表
    players.clear();
    spectators.clear();
    
    // 创建回合计时器
    turnTimer = new QTimer(this);
    connect(turnTimer, &QTimer::timeout, this, &GameRoom::handleTurnTimeout);
    
    // 创建房间状态更新定时器
    statusUpdateTimer = new QTimer(this);
    connect(statusUpdateTimer, &QTimer::timeout, this, &GameRoom::updateRoomStatus);
    startStatusUpdates();  // 创建房间时就开始状态更新
    
    qDebug() << "Created new game room with code:" << roomCode 
             << "and capacity:" << capacity;
    
    // 创建游戏实例
    gameInstance = std::make_unique<ServerGame>(this);
    
    // 当创建ServerGame实例后再连接信号槽
    if (gameInstance) {
        connect(this, &GameRoom::playerActionReceived,
                gameInstance.get(), &ServerGame::onPlayerActionReceived,
                Qt::QueuedConnection);
        
        connect(gameInstance.get(), &ServerGame::gameStateChanged,
                this, &GameRoom::onGameStateUpdated,
                Qt::QueuedConnection);
    }
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
    if (!isFull() && !players.contains(playerSocket)) {
        QString username = playerSocket->property("username").toString();
        players.append(playerSocket);
        
        if (owner == nullptr) {
            owner = playerSocket;
            qDebug() << "Room" << roomCode << ": Set" << username << "as room owner";
        }
        
        // 更新最后活动时间
        updateLastActivity();
        
        // 发送玩家加入通知
        QJsonObject notification;
        notification.insert("type", "player_joined");
        notification.insert("player", username);
        notification.insert("is_owner", playerSocket == owner);
        notification.insert("player_count", players.size());
        
        broadcastToRoom(notification);
        emit playerJoined(username);
        
        qDebug() << "Room" << roomCode << ":" << username << "joined. Total players:" << players.size();
    }
}

bool GameRoom::removePlayer(QTcpSocket* playerSocket)
{
    if (players.contains(playerSocket)) {
        QString username = playerSocket->property("username").toString();
        players.removeAll(playerSocket);
        
        // 如果是房主离开，选择新的房主
        if (playerSocket == owner) {
            owner = players.isEmpty() ? nullptr : players.first();
            if (owner) {
                qDebug() << "Room" << roomCode << ": New owner is" << owner->property("username").toString();
            }
        }
        
        // 更新最后活动时间
        updateLastActivity();
        
        // 发送玩家离开通知
        QJsonObject notification;
        notification.insert("type", "player_left");
        notification.insert("player", username);
        notification.insert("new_owner", owner ? owner->property("username").toString() : "");
        notification.insert("player_count", players.size());
        
        broadcastToRoom(notification);
        emit playerLeft(username);
        
        qDebug() << "Room" << roomCode << ":" << username << "left. Total players:" << players.size();
        
        // 如果游戏已经开始且玩家数不足，结束游戏
        if (m_gameStarted && players.size() < 2) {
            endGame();
        }
        
        return true;
    }
    return false;
}

bool GameRoom::kickPlayer(QTcpSocket* playerSocket)
{
    if (playerSocket != owner && players.contains(playerSocket)) {
        QString username = playerSocket->property("username").toString();
        
        // 发送被踢出通知给被踢玩家
        QJsonObject kickNotification;
        kickNotification.insert("type", "kicked");
        kickNotification.insert("room_code", roomCode);
        QJsonDocument doc(kickNotification);
        playerSocket->write(doc.toJson(QJsonDocument::Compact) + "\n");
        
        // 移除玩家
        removePlayer(playerSocket);
        
        // 发送踢出通知给其他玩家
        QJsonObject notification;
        notification.insert("type", "player_kicked");
        notification.insert("player", username);
        notification.insert("player_count", players.size());
        
        broadcastToRoom(notification);
        emit playerKicked(username);
        
        qDebug() << "Room" << roomCode << ":" << username << "was kicked";
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

QStringList GameRoom::getPlayerNames() const
{
    QStringList names;
    foreach (QTcpSocket* player, players) {
        names << player->property("username").toString();
    }
    return names;
}

void GameRoom::handleTurnTimeout()
{
    // 处理玩家回合超时
    if (!m_gameStarted || players.isEmpty()) return;
    
    QTcpSocket* currentPlayer = players[currentPlayerIndex];
    QString username = currentPlayer->property("username").toString();
    
    // 通知所有玩家当前玩家超时
    QJsonObject notification;
    notification.insert("type", "turn_timeout");
    notification.insert("player", username);
    
    broadcastToRoom(notification);
    
    // 自动进入下一个玩家的回合
    nextTurn();
}

void GameRoom::updateRoomStatus()
{
    QJsonObject status;
    status.insert("type", "room_status");
    status.insert("room_code", roomCode);
    status.insert("capacity", roomCapacity);
    status.insert("game_started", m_gameStarted);
    status.insert("player_count", players.size());

    // 添加玩家列表
    QJsonArray playersArray;
    for (QTcpSocket* player : players) {
        QJsonObject playerObj;
        playerObj.insert("username", player->property("username").toString());
        playerObj.insert("ready", playerReadyStates.value(player, false));
        playerObj.insert("is_owner", (player == owner));
        playersArray.append(playerObj);
    }
    status.insert("players", playersArray);

    // 添加观察者列表
    QJsonArray spectatorsArray;
    for (QTcpSocket* spectator : spectators) {
        spectatorsArray.append(spectator->property("username").toString());
    }
    status.insert("spectators", spectatorsArray);

    // 如果游戏已经开始，添加游戏相关信息
    if (m_gameStarted) {
        status.insert("current_round", roundNumber);
        if (currentPlayerIndex < players.size()) {
            status.insert("current_player", players[currentPlayerIndex]->property("username").toString());
        }
    }

    // 广播给所有玩家和观察者
    QByteArray statusData = QJsonDocument(status).toJson(QJsonDocument::Compact) + "\n";
    for (QTcpSocket* socket : players + spectators) {
        if (socket->state() == QAbstractSocket::ConnectedState) {
            socket->write(statusData);
            socket->flush();
        }
    }
}

void GameRoom::broadcastToRoom(const QJsonObject& message) {
    // 确保在主线程中执行
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, "broadcastToRoom",
                                Qt::BlockingQueuedConnection,
                                Q_ARG(QJsonObject, message));
        return;
    }

    QJsonDocument doc(message);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";
    
    for (QTcpSocket* socket : players) {
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            socket->write(data);
            socket->flush();
        }
    }
    
    for (QTcpSocket* socket : spectators) {
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            socket->write(data);
            socket->flush();
        }
    }
}

void GameRoom::nextTurn()
{
    if (!m_gameStarted || players.isEmpty()) return;
    
    // 停止当前回合计时器
    turnTimer->stop();
    
    // 更新当前玩家索引
    currentPlayerIndex = (currentPlayerIndex + 1) % players.size();
    
    // 如果回到第一个玩家，增加回合数
    if (currentPlayerIndex == 0) {
        roundNumber++;
    }
    
    // 通知所有玩家轮到谁的回合
    QJsonObject notification;
    notification.insert("type", "next_turn");
    notification.insert("player", players[currentPlayerIndex]->property("username").toString());
    notification.insert("round", roundNumber);
    
    broadcastToRoom(notification);
    
    // 启动新的回合计时器
    turnTimer->start(turnTimeLimit * 1000);
}

bool GameRoom::startGame()
{
    if (players.size() < 2 || m_gameStarted) {
        return false;
    }

    m_gameStarted = true;
    currentPlayerIndex = 0;
    roundNumber = 1;

    // 通知所有玩家游戏开始
    QJsonObject notification;
    notification.insert("type", "game_started");
    notification.insert("first_player", players[currentPlayerIndex]->property("username").toString());
    notification.insert("round", roundNumber);
    
    // 添加玩家列表
    QJsonArray playerList;
    foreach (QTcpSocket* player, players) {
        playerList.append(player->property("username").toString());
    }
    notification.insert("players", playerList);
    
    broadcastToRoom(notification);
    
    // 启动回合计时器
    turnTimer->start(turnTimeLimit * 1000);
    
    emit gameStartedSignal();
    return true;
}

void GameRoom::endGame()
{
    if (!m_gameStarted) return;

    m_gameStarted = false;
    turnTimer->stop();
    
    // 通知所有玩家游戏结束
    QJsonObject notification;
    notification.insert("type", "game_ended");
    notification.insert("total_rounds", roundNumber);
    
    broadcastToRoom(notification);
    
    emit gameEndedSignal();
}

void GameRoom::pauseGame()
{
    if (!m_gameStarted) return;
    
    turnTimer->stop();
    
    // 通知所有玩家游戏暂停
    QJsonObject notification;
    notification.insert("type", "game_paused");
    notification.insert("current_player", players[currentPlayerIndex]->property("username").toString());
    notification.insert("round", roundNumber);
    
    broadcastToRoom(notification);
}

void GameRoom::resumeGame()
{
    if (!m_gameStarted) return;
    
    // 重新启动回合计时器
    turnTimer->start(turnTimeLimit * 1000);
    
    // 通知所有玩家游戏继续
    QJsonObject notification;
    notification.insert("type", "game_resumed");
    notification.insert("current_player", players[currentPlayerIndex]->property("username").toString());
    notification.insert("round", roundNumber);
    
    broadcastToRoom(notification);
}

// 添加一些辅助函数的实现
bool GameRoom::isPlayerTurn(QTcpSocket* player) const
{
    return player == getCurrentPlayer();
}

void GameRoom::updateLastActivity()
{
    lastActivityTime = QDateTime::currentDateTime();
}

void GameRoom::handlePlayerReady(QTcpSocket* playerSocket)
{
    // 确保在主线程中执行
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, "handlePlayerReady",
                                Qt::BlockingQueuedConnection,
                                Q_ARG(QTcpSocket*, playerSocket));
        return;
    }
    
    if (!players.contains(playerSocket)) return;
    
    // 切换玩家的准备状态
    bool currentState = playerReadyStates.value(playerSocket, false);
    playerReadyStates[playerSocket] = !currentState;
    
    qDebug() << "Player" << playerSocket->property("username").toString() 
             << "ready state changed to:" << (!currentState);
    
    // 广播玩家准备状态变化
    QJsonObject notification;
    notification.insert("type", "player_ready_state");
    notification.insert("player", playerSocket->property("username").toString());
    notification.insert("ready", !currentState);
    
    // 使用主线程安全的广播方法
    QMetaObject::invokeMethod(this, "broadcastToRoom",
                            Qt::QueuedConnection,
                            Q_ARG(QJsonObject, notification));
    
    // 检查是否所有玩家都准备好了
    QMetaObject::invokeMethod(this, "checkGameStart",
                            Qt::QueuedConnection);
}

void GameRoom::saveGameState()
{
    QJsonObject gameState;
    gameState.insert("room_code", roomCode);
    gameState.insert("round_number", roundNumber);
    gameState.insert("current_player", currentPlayerIndex);
    
    QJsonArray playerStates;
    foreach (QTcpSocket* player, players) {
        QJsonObject playerState;
        playerState.insert("username", player->property("username").toString());
        playerState.insert("score", player->property("score").toInt());
        playerStates.append(playerState);
    }
    gameState.insert("players", playerStates);
    
    // 保存到文件
    QString filename = QString("game_state_%1.json").arg(roomCode);
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(gameState).toJson());
        file.close();  // 确保文件被关闭
    }
}

void GameRoom::addSpectator(QTcpSocket* spectatorSocket)
{
    // 确保在主线程中执行
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, "addSpectator",
                                Qt::BlockingQueuedConnection,
                                Q_ARG(QTcpSocket*, spectatorSocket));
        return;
    }

    if (spectatorSocket && !spectators.contains(spectatorSocket)) {
        spectators.append(spectatorSocket);
        
        // 通知所有玩家和观战者有新的观战者加入
        QJsonObject notification;
        notification.insert("type", "spectator_joined");
        notification.insert("spectator", spectatorSocket->property("username").toString());
        broadcastToRoom(notification);
        
        qDebug() << "Room" << roomCode << ": New spectator" 
                 << spectatorSocket->property("username").toString();
    }
}

void GameRoom::setGameState(RoomState newState)
{
    QJsonObject notification;
    notification.insert("type", "game_state_changed");
    notification.insert("state", static_cast<int>(newState));
    
    switch(newState) {
    case RoomState::WAITING:
        notification.insert("message", "Waiting for players...");
        break;
    case RoomState::STARTING:
        notification.insert("message", "Game is starting...");
        break;
    case RoomState::PLAYING:
        notification.insert("message", "Game in progress");
        break;
    case RoomState::PAUSED:
        notification.insert("message", "Game is paused");
        break;
    case RoomState::ENDED:
        notification.insert("message", "Game has ended");
        break;
    }
    
    broadcastToRoom(notification);
    emit gameStateChanged(newState);
}

void GameRoom::saveGameProgress()
{
    QJsonObject gameState;
    gameState.insert("room_code", roomCode);
    gameState.insert("current_round", roundNumber);
    gameState.insert("current_player", currentPlayerIndex);
    
    QJsonArray playerStates;
    foreach(QTcpSocket* player, players) {
        QJsonObject playerState;
        playerState.insert("username", player->property("username").toString());
        playerState.insert("ready", playerReadyStates.value(player, false));
        playerStates.append(playerState);
    }
    gameState.insert("players", playerStates);
    
    QString filename = QString("game_progress_%1.json").arg(roomCode);
    QFile file(filename);
    if(file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(gameState).toJson());
        file.close();  // 确保文件被关闭
    }
}

bool GameRoom::loadGameProgress(const QString& filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();  // 确保文件被关闭
    
    if(doc.isNull() || !doc.isObject()) {
        return false;
    }
    
    QJsonObject gameState = doc.object();
    roundNumber = gameState["current_round"].toInt();
    currentPlayerIndex = gameState["current_player"].toInt();
    
    // 恢复玩家状态
    QJsonArray playerStates = gameState["players"].toArray();
    for(const QJsonValue& value : playerStates) {
        QJsonObject playerState = value.toObject();
        QString username = playerState["username"].toString();
        foreach(QTcpSocket* player, players) {
            if(player->property("username").toString() == username) {
                playerReadyStates[player] = playerState["ready"].toBool();
                break;
            }
        }
    }
    
    return true;
}

bool GameRoom::isPlayerReady(QTcpSocket* player) const
{
    return playerReadyStates.value(player, false);
}

void GameRoom::setPlayerReady(QTcpSocket* player, bool ready) {
    // 确保在主线程中执行
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, "setPlayerReady",
                                Qt::BlockingQueuedConnection,
                                Q_ARG(QTcpSocket*, player),
                                Q_ARG(bool, ready));
        return;
    }

    if (!players.contains(player)) return;
    
    playerReadyStates[player] = ready;
    QString username = player->property("username").toString();
    
    QJsonObject notification;
    notification.insert("type", "player_ready_state");
    notification.insert("player", username);
    notification.insert("ready", ready);
    broadcastToRoom(notification);
    
    checkGameStart();
}

bool GameRoom::areAllPlayersReady() const
{
    qDebug() << "Checking player ready states:";  // 添加日志
    if (players.size() < 1) {
        qDebug() << "Not enough players:" << players.size();  // 添加日志
        return false;
    }
    
    for (QTcpSocket* player : players) {
        bool ready = playerReadyStates.value(player, false);
        qDebug() << "Player" << player->property("username").toString() 
                 << "ready state:" << ready;  // 添加日志
        if (!ready) {
            return false;
        }
    }
    return true;
}

void GameRoom::checkGameStart()
{
    // 确保在主线程中执行
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, "checkGameStart",
                                Qt::BlockingQueuedConnection);
        return;
    }
    
    // 如果所有玩家都准备好了，并且人数大于等于3，就可以开始游戏
    if (areAllPlayersReady() && players.size() >= 1) {
        QJsonObject notification;
        notification.insert("type", "game_can_start");
        notification.insert("message", "All players are ready and minimum player count (3) reached. Game can start.");
        
        // 直接发送消息给房主，不使用 invokeMethod
        if (owner && owner->state() == QAbstractSocket::ConnectedState) {
            QByteArray data = QJsonDocument(notification).toJson(QJsonDocument::Compact) + "\n";
            owner->write(data);
            owner->flush();
            qDebug() << "Sent game start notification to room owner";
        }
    }
}

void GameRoom::updateGameStats()
{
    stats.totalGamesPlayed++;
    stats.totalRoundsPlayed += roundNumber;
    
    QDateTime gameDuration = QDateTime::currentDateTime();
    if (stats.longestGameDuration.isNull() || 
        creationTime.secsTo(gameDuration) > creationTime.secsTo(stats.longestGameDuration)) {
        stats.longestGameDuration = gameDuration;
    }
    if (stats.shortestGameDuration.isNull() || 
        creationTime.secsTo(gameDuration) < creationTime.secsTo(stats.shortestGameDuration)) {
        stats.shortestGameDuration = gameDuration;
    }
}

QJsonObject GameRoom::getGameStats() const
{
    QJsonObject statsObj;
    statsObj.insert("total_games", stats.totalGamesPlayed);
    statsObj.insert("total_rounds", stats.totalRoundsPlayed);
    
    if (!stats.longestGameDuration.isNull()) {
        statsObj.insert("longest_game", stats.longestGameDuration.toString(Qt::ISODate));
    }
    if (!stats.shortestGameDuration.isNull()) {
        statsObj.insert("shortest_game", stats.shortestGameDuration.toString(Qt::ISODate));
    }
    
    QJsonObject playerWins;
    for (auto it = stats.playerWins.begin(); it != stats.playerWins.end(); ++it) {
        playerWins.insert(it.key(), it.value());
    }
    statsObj.insert("player_wins", playerWins);
    
    return statsObj;
}

bool GameRoom::saveGameState(const QString& filename)
{
    QJsonObject state;
    state.insert("room_code", roomCode);
    state.insert("game_started", m_gameStarted);
    state.insert("current_round", roundNumber);
    state.insert("current_player_index", currentPlayerIndex);
    
    QJsonArray playerArray;
    foreach(QTcpSocket* player, players) {
        QJsonObject playerObj;
        playerObj.insert("username", player->property("username").toString());
        playerObj.insert("ready", playerReadyStates.value(player, false));
        playerArray.append(playerObj);
    }
    state.insert("players", playerArray);
    
    QFile file(filename);
    if(!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    file.write(QJsonDocument(state).toJson());
    return true;
}

bool GameRoom::loadGameState(const QString& filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if(doc.isNull() || !doc.isObject()) {
        return false;
    }
    
    QJsonObject state = doc.object();
    m_gameStarted = state["game_started"].toBool();
    roundNumber = state["current_round"].toInt();
    currentPlayerIndex = state["current_player_index"].toInt();
    
    return true;
}

void GameRoom::startStatusUpdates()
{
    if (!statusUpdateTimer->isActive()) {
        statusUpdateTimer->start(STATUS_UPDATE_INTERVAL);
    }
}

void GameRoom::stopStatusUpdates()
{
    if (statusUpdateTimer->isActive()) {
        statusUpdateTimer->stop();
    }
}

GameRoom::~GameRoom()
{
    stopStatusUpdates();
    delete statusUpdateTimer;
}

void GameRoom::startGameLogic() {
    // 确保在主线程中执行
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, "startGameLogic",
                                Qt::BlockingQueuedConnection);
        return;
    }

    qDebug() << "\n=== Starting Game Logic ===";
    
    // 检查开始条件
    if (!areAllPlayersReady() || players.size() < 1) {
        QString reason = !areAllPlayersReady() ? "Not all players are ready" : 
                        "Need at least 3 players";
        qDebug() << "Game start failed:" << reason;
        
        QJsonObject response;
        response.insert("type", "start_game_failed");
        response.insert("message", players.size() < 1 ?
            "Need at least 3 players to start" : 
            "All players must be ready to start");
        
        if (owner) {
            owner->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
        }
        return;
    }
    
    // 设置游戏状态
    m_gameStarted = true;
    setGameState(RoomState::PLAYING);
    
    // 创建游戏实例（在主线程中）
    gameInstance = std::make_unique<ServerGame>();
    gameInstance->moveToThread(QCoreApplication::instance()->thread());
    // 连接信号
    connect(gameInstance.get(), &ServerGame::gameReturningToRoom,
            this, &GameRoom::updateRoomStatus);
    
    // 初始化玩家
    for (auto* socket : players) {
        QString username = socket->property("username").toString();
        Player* player = new Player(username, username);
        gameInstance->addPlayer(player);
        gameInstance->addPlayerSocket(socket);
    }

    // 广播游戏开始消息
    QJsonObject notification;
    notification.insert("type", "game_started");
    notification.insert("message", "Game has started!");
    
    QJsonArray playerArray;
    for (auto* socket : players) {
        playerArray.append(socket->property("username").toString());
    }
    notification.insert("players", playerArray);
    
    broadcastToRoom(notification);
    
    // 启动游戏逻辑
    QMetaObject::invokeMethod(gameInstance.get(), "startGameLogic",
                            Qt::QueuedConnection);
}

void GameRoom::handlePlayerAction(const QJsonObject& actionJson, QTcpSocket* socket) {
    if (gameInstance && socket) {
        QJsonDocument doc(actionJson);
        QByteArray data = doc.toJson(QJsonDocument::Compact);
        
        qDebug() << "\n=== GameRoom Handling Player Action ===";
        qDebug() << "From player:" << socket->property("username").toString();
        qDebug() << "Action data:" << QString(data);
        
        // 调用 ServerGame 的处理函数
        gameInstance->receivePlayerAction(data, socket);
        
        qDebug() << "Action forwarded to ServerGame";
    } else {
        qDebug() << "Error: Cannot handle player action - gameInstance or socket is null";
    }
}

void GameRoom::onGameStateUpdated(const QJsonObject& json) {
    // 广播游戏状态更新给所有玩家
    QJsonObject notification;
    notification.insert("type", "game_state_update");
    notification.insert("state", json);
    
    broadcastToRoom(notification);
}

// 添加这些函数的实现
void GameRoom::setOwner(QTcpSocket* ownerSocket) {
    // 确保在主线程中执行
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, "setOwner",
                                Qt::BlockingQueuedConnection,
                                Q_ARG(QTcpSocket*, ownerSocket));
        return;
    }

    if (ownerSocket && players.contains(ownerSocket)) {
        owner = ownerSocket;
        
        // 通知所有玩家房主变更
        QJsonObject notification;
        notification.insert("type", "owner_changed");
        notification.insert("new_owner", owner->property("username").toString());
        broadcastToRoom(notification);
        
        qDebug() << "Room" << roomCode << ": New owner set to" 
                 << owner->property("username").toString();
    }
}

QList<QTcpSocket*> GameRoom::getSpectators() const {
    return spectators;
}

void GameRoom::removeSpectator(QTcpSocket* socket) {
    // 确保在主线程中执行
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, "removeSpectator",
                                Qt::BlockingQueuedConnection,
                                Q_ARG(QTcpSocket*, socket));
        return;
    }

    if (spectators.removeOne(socket)) {
        // 通知所有玩家和观战者有观战者离开
        QJsonObject notification;
        notification.insert("type", "spectator_left");
        notification.insert("spectator", socket->property("username").toString());
        broadcastToRoom(notification);
        
        qDebug() << "Room" << roomCode << ": Spectator" 
                 << socket->property("username").toString() << "left";
    }
}


