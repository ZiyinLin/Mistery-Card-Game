#include "tcpserver.h"
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDebug>
#include <QDir>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QCryptographicHash>
#include <QRegularExpression>
#include "GameRoomWorker.h"
#include <QThread>
#include <QJsonArray>


TcpServer::TcpServer()
{
    tcpServer = new QTcpServer(this);
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newConnection_Slot()));
    initializeDatabase();
}

void TcpServer::StartListen(int port)
{
    if(!tcpServer->listen(QHostAddress::Any,port))
    {
        qDebug()<<"server faile";
    }else{
        qDebug()<<"server start";
    }
}

void TcpServer::CloseServer()
{
    tcpServer->close();
}

void TcpServer::SendData(QString text)
{
    QList <QTcpSocket *> socketList = tcpServer->findChildren<QTcpSocket *>();
    qDebug() << "tcpSocket number:" << socketList.count() << Qt::endl;

    if (socketList.count() == 0) {
        qDebug()<<"Currently there is no client connection, please connect with the client first!";
        return;
    }
    foreach (QTcpSocket *tmpTcpSocket, socketList) {
        tmpTcpSocket->write(text.toUtf8());
    }
}

void TcpServer::newConnection_Slot()
{
    QTcpSocket *temptcpSocket = tcpServer->nextPendingConnection();
    connect(temptcpSocket,SIGNAL(readyRead()),this,SLOT(readyRead_Slot()));
    connect(temptcpSocket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),this,SLOT(stateChanged_Slot(QAbstractSocket::SocketState)));
    connect(temptcpSocket, &QTcpSocket::disconnected, this, [ temptcpSocket, this]() {
        clientDisconnected(temptcpSocket);
        temptcpSocket->deleteLater();
    });

    // 客户端的ip地址
    QString ipaddr = temptcpSocket->peerAddress().toString();
    quint16 port = temptcpSocket->peerPort();
    // 打印客户端连接的端口信息
    qDebug()<<"Server: Client ip address:" + ipaddr;
    qDebug()<<"Server: Client port:" + QString::number(port);
}

void TcpServer::readyRead_Slot()
{
    QTcpSocket *tcpSocket = (QTcpSocket *)sender();
    QByteArray data = tcpSocket->readAll();
    buffer.append(data); // 将新数据添加到缓冲区

    int endIndex;
    // 检查是否收到了完整的 JSON 消息（以换行符为结束标志）
    while ((endIndex = buffer.indexOf('\n')) != -1) { // 查找换行符的位置
        QByteArray jsonData = buffer.mid(0, endIndex).trimmed(); // 提取 JSON 数据
        buffer.remove(0, endIndex + 1); // 移除已处理的数据

        QJsonParseError jsonError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &jsonError);

        if (jsonError.error != QJsonParseError::NoError) {
            qDebug() << "JSON parse error:" << jsonError.errorString();
            continue; // 跳过错误的 JSON 消息
        }

        if (jsonDoc.isObject()) {
            QJsonObject jsonObj = jsonDoc.object();
            QString type = jsonObj.value("type").toString();

            if (type == "login") {
                handleLogin(jsonObj, tcpSocket);
            } else if (type == "register") {
                handleRegistration(jsonObj, tcpSocket);
            } else if (type == "create_room") {
                handleCreateRoom(jsonObj, tcpSocket);
            } else if (type == "join_room") {
                handleJoinRoom(jsonObj, tcpSocket); // 处理加入房间的请求
            } else if (type == "kick_player") {
                handleKickPlayer(jsonObj, tcpSocket); // 处理踢出玩家的请求
            } else {
                // 处理其他类型的请求
                qDebug() << "Received unknown request type:" << type;
            }
        } else {
            qDebug() << "Received JSON is not an object.";
        }
    }
}


void TcpServer::stateChanged_Slot(QAbstractSocket::SocketState socketState)
{
    QTcpSocket *tcpSocket = (QTcpSocket *)sender();
    switch(socketState){
    case QAbstractSocket::UnconnectedState:
        qDebug()<<"connection end";
        tcpSocket->deleteLater();
        break;
    default:
        break;
    }
}
void TcpServer::initializeDatabase()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("users.db");

    if (!db.open()) {
        qDebug() << "Error: Unable to open database!";
        return;
    }

    QSqlQuery query;
    if (!query.exec("CREATE TABLE IF NOT EXISTS users ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "username TEXT UNIQUE, "
                    "password TEXT)")) {
        qDebug() << "Failed to create table:" << query.lastError();
    }
}
void TcpServer::handleRegistration(const QJsonObject& jsonObj, QTcpSocket* tcpSocket)
{
    QString username = jsonObj.value("username").toString().trimmed();
    QString password = jsonObj.value("password").toString().trimmed();

    // 正则表达式确保用户名和密码符合特定格式
    QRegularExpression usernameRegex("^[a-zA-Z0-9_]{3,15}$");
    QRegularExpression passwordRegex("^[a-zA-Z0-9@#$%^&+=]{6,}$");

    QString errMsg;
    if (!usernameRegex.match(username).hasMatch()) {
        errMsg = "Username must be 3 to 15 characters long and can only contain letters, numbers, and underscores.\n";
    } else if (!passwordRegex.match(password).hasMatch()) {
        errMsg = "Password must be at least 6 characters long and can contain letters, numbers, and special characters @#$%^&+=.\n";
    }

    if (!errMsg.isEmpty()) {
        QJsonObject response;
        response.insert("type", "registration");
        response.insert("status", "error");
        response.insert("message", errMsg);
        QJsonDocument responseDoc(response);
        tcpSocket->write(responseDoc.toJson(QJsonDocument::Compact) + "\n");
        return;
    }

    // 密码哈希处理
    QByteArray hashedPassword = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);

    // 使用参数化查询防止SQL注入
    QSqlQuery query;
    query.prepare("INSERT INTO users (username, password) VALUES (:username, :password)");
    query.bindValue(":username", username);
    query.bindValue(":password", hashedPassword.toHex());

    // 执行查询并处理结果
    QJsonObject response;
    if (!query.exec()) {
        qDebug() << "Registration failed:" << query.lastError();
        response.insert("type", "registration");
        response.insert("status", "error");
        response.insert("message", "Registration failed: Username already exists or database error");
    } else {
        response.insert("type", "registration");
        response.insert("status", "success");
        response.insert("message", "Registration successful");
    }
    QJsonDocument responseDoc(response);
    tcpSocket->write(responseDoc.toJson(QJsonDocument::Compact) + "\n");
}


void TcpServer::handleLogin(const QJsonObject& jsonObj, QTcpSocket* tcpSocket)
{
    QString username = jsonObj.value("username").toString().trimmed();
    QString password = jsonObj.value("password").toString().trimmed();

    // 正则表达式确保用户名和密码符合特定格式
    QRegularExpression usernameRegex("^[a-zA-Z0-9_]{3,15}$");
    QRegularExpression passwordRegex("^[a-zA-Z0-9@#$%^&+=]{6,}$");

    QString errMsg;
    if (!usernameRegex.match(username).hasMatch()) {
        errMsg = "Username must be 3 to 15 characters long and can only contain letters, numbers, and underscores.\n";
    } else if (!passwordRegex.match(password).hasMatch()) {
        errMsg = "Password must be at least 6 characters long and can contain letters, numbers, and special characters @#$%^&+=.\n";
    }

    if (!errMsg.isEmpty()) {
        tcpSocket->write(errMsg.toUtf8());
        return;
    }

    // 密码哈希处理
    QByteArray hashedPassword = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    QSqlQuery query;
    query.prepare("SELECT password FROM users WHERE username = :username");
    query.bindValue(":username", username);
    QJsonObject response;
    if (!query.exec()) {
        qDebug() << "Login failed:" << query.lastError();
        response.insert("type", "login");
        response.insert("status", "error");
        response.insert("message", "Login failed: Database error");
    } else if (query.next()) {
        QString storedPassword = query.value(0).toString();
        if (storedPassword == QString(hashedPassword.toHex())) {
            // 检查用户是否已经登录
            QTcpSocket* existingSocket = userSockets.value(username, nullptr);
            if (existingSocket && existingSocket != tcpSocket) {
                // 已经有一个连接使用这个用户名，断开它
                existingSocket->disconnectFromHost();
            }
            // 更新映射
            userSockets[username] = tcpSocket;

            // 设置用户属性
            tcpSocket->setProperty("username", QVariant(username));

            // 发送登录成功的响应
            response.insert("type", "login");
            response.insert("status", "success");
            response.insert("message", "Login successful");
            response.insert("username", username);
        } else {
            // 密码不正确
            response.insert("type", "login");
            response.insert("status", "error");
            response.insert("message", "Login failed: Incorrect password");
        }
    } else {
        // 用户名不存在
        response.insert("type", "login");
        response.insert("status", "error");
        response.insert("message", "Login failed: Username does not exist");
    }
    QJsonDocument responseDoc(response);
    tcpSocket->write(responseDoc.toJson(QJsonDocument::Compact) + "\n");
}


QString TcpServer::generateUniqueRoomCode() {
    QString roomCode;
    do {
        roomCode = QString::number(QRandomGenerator::global()->bounded(100000, 999999));
    } while (rooms.contains(roomCode));
    return roomCode;
}

void TcpServer::handleCreateRoom(const QJsonObject& jsonObj, QTcpSocket* tcpSocket) {
    int capacity = jsonObj.value("capacity").toInt();
    if (capacity < 3 || capacity > 8) {
        tcpSocket->write("Invalid room capacity. Must be between 3 and 8.\n");
        return;
    }

    // 创建工作线程和工作对象
    QThread* thread = new QThread(this);
    GameRoomWorker* worker = new GameRoomWorker(nullptr, tcpSocket, this);
    worker->moveToThread(thread);


    // 连接信号和槽
    connect(thread, &QThread::started, [worker, capacity]() {
        worker->createRoom(capacity);
    });
    connect(worker, &GameRoomWorker::roomCreated, this, &TcpServer::onRoomCreated);
    connect(worker, &GameRoomWorker::errorOccurred, this, [this](QTcpSocket* ownerSocket, const QString& errorMessage) {
        this->onRoomCreationError(ownerSocket, errorMessage);
    });

    // 清理
    connect(worker, &GameRoomWorker::roomCreated, worker, &GameRoomWorker::deleteLater);
    connect(worker, &GameRoomWorker::errorOccurred, worker, &GameRoomWorker::deleteLater);
    connect(worker, &GameRoomWorker::roomCreated, thread, &QThread::quit);
    connect(worker, &GameRoomWorker::errorOccurred, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    // 启动线程
    thread->start();
}



void TcpServer::handleKickPlayer(const QJsonObject& jsonObj, QTcpSocket* tcpSocket)
{
    QString roomCode = jsonObj.value("room_code").toString();
    QString playerToKick = jsonObj.value("player").toString();

    GameRoom* room = rooms.value(roomCode, nullptr);
    if (room && room->getOwner() == tcpSocket) {
        // 查找要踢的玩家
        foreach (QTcpSocket* player, room->getPlayers()) {
            // 使用之前保存的用户名属性来获取玩家的用户名
            if (player->property("username").toString() == playerToKick) {
                if (room->kickPlayer(player)) {
                    // 通知被踢的玩家
                    QJsonObject kickedNotification;
                    kickedNotification.insert("type", "kicked");
                    kickedNotification.insert("reason", "You have been kicked out of the room.");
                    QJsonDocument doc(kickedNotification);
                    player->write(doc.toJson(QJsonDocument::Compact) + "\n");

                    // 通知房间内的所有玩家
                    SendDataToRoom(roomCode, QString("%1 has been kicked out of the room.").arg(playerToKick));
                    player->disconnectFromHost(); // 断开连接
                    break;
                }
            }
        }
    } else {
        // 发送错误消息给请求踢人的玩家
        QJsonObject errorResponse;
        errorResponse.insert("type", "kick_error");
        errorResponse.insert("reason", "You are not the owner of this room or room does not exist.");
        QJsonDocument doc(errorResponse);
        tcpSocket->write(doc.toJson(QJsonDocument::Compact) + "\n");
    }
}

void TcpServer::SendDataToRoom(const QString& roomCode, const QString& message)
{
    GameRoom* room = rooms.value(roomCode, nullptr);
    if (room == nullptr) {
        // 房间不存在
        qDebug() << "Attempted to send message to a non-existent room:" << roomCode;
        return;
    }

    QList<QTcpSocket*> players = room->getPlayers();
    foreach (QTcpSocket* player, players) {
        if (player->state() == QAbstractSocket::ConnectedState) {
            QJsonObject messageObj;
            messageObj.insert("type", "room_message");
            messageObj.insert("message", message);
            QJsonDocument messageDoc(messageObj);
            player->write(messageDoc.toJson(QJsonDocument::Compact).append('\n'));
        } else {
            qDebug() << "Player socket not connected, unable to send message.";
        }
    }
}

void TcpServer::handleJoinRoom(const QJsonObject& jsonObj, QTcpSocket* tcpSocket)
{
    QString roomCode = jsonObj.value("room_code").toString();
    QJsonObject response;

    // 检查房间码是否存在
    if (!rooms.contains(roomCode)) {
        response.insert("type", "join_room");
        response.insert("status", "error");
        response.insert("message", "Room does not exist.");
        QJsonDocument responseDoc(response);
        tcpSocket->write(responseDoc.toJson(QJsonDocument::Compact) + "\n");
        return;
    }

    GameRoom* room = rooms.value(roomCode);
    if (room->isFull()) {
        response.insert("type", "join_room");
        response.insert("status", "error");
        response.insert("message", "Room is full.");
        QJsonDocument responseDoc(response);
        tcpSocket->write(responseDoc.toJson(QJsonDocument::Compact) + "\n");
        return;
    }
    room->addPlayer(tcpSocket);

    // 通知玩家成功加入房间
    response.insert("type", "join_room");
    response.insert("status", "success");
    response.insert("message", "Joined room successfully.");
    response.insert("room_code", roomCode);

    // 发送房间的当前状态给玩家，包括房间内的其他玩家列表
    QStringList playerNames = room->getPlayerNames();
    QJsonArray playersArray;
    foreach (const QString& playerName, playerNames) {
        playersArray.append(playerName);
    }
    response.insert("players", playersArray);

    QJsonDocument responseDoc(response);
    tcpSocket->write(responseDoc.toJson(QJsonDocument::Compact) + "\n");

    // 通知房间内的其他玩家有新玩家加入
    QString joinMessage = QString("%1 has joined the room.").arg(tcpSocket->property("username").toString());
    SendDataToRoom(roomCode, joinMessage);
}

void TcpServer::clientDisconnected(QTcpSocket* clientSocket) {
    // 从所有房间中查找并移除该客户端
    foreach (GameRoom* room, rooms) {
        if (room->removePlayer(clientSocket)) {
            // 如果找到并移除了玩家，通知房间内的其他玩家
            QString username = clientSocket->property("username").toString();
            SendDataToRoom(room->getRoomCode(), QString("%1 has left the room.").arg(username));
            break;
        }
    }
}
void TcpServer::cleanupRooms() {
    QMutableHashIterator<QString, GameRoom*> i(rooms);
    while (i.hasNext()) {
        i.next();
        if (i.value()->getPlayers().isEmpty()) {
            delete i.value();
            i.remove();
        }
    }
}
void TcpServer::onRoomCreated(GameRoom* newRoom) {
    // 将新创建的房间添加到房间列表中
    QString roomCode = newRoom->getRoomCode();
    rooms.insert(roomCode, newRoom);

    // 发送房间创建成功的消息给房间的创建者
    QTcpSocket* ownerSocket = newRoom->getOwner();
    if (ownerSocket) {
        QJsonObject response;
        response.insert("type", "create_room");
        response.insert("status", "success");
        response.insert("room_code", roomCode);
        QJsonDocument responseDoc(response);
        ownerSocket->write(responseDoc.toJson(QJsonDocument::Compact) + "\n");
    }
}

void TcpServer::onRoomCreationError(QTcpSocket* ownerSocket, const QString& errorMessage) {
    if (ownerSocket) {
        QJsonObject response;
        response.insert("type", "create_room");
        response.insert("status", "error");
        response.insert("message", errorMessage);
        QJsonDocument responseDoc(response);
        ownerSocket->write(responseDoc.toJson(QJsonDocument::Compact) + "\n");
    }
}
