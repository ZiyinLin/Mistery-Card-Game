#include "InputHandler.h"
#include <QRegularExpression>
#include <QDebug>
#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>

const QString RED_TEXT_START = "\033[31m";
const QString RED_TEXT_END = "\033[0m";

InputHandler::InputHandler(CClientSocket* socket) : m_socket(socket)
{
    // 连接信号槽
    connect(m_socket, &CClientSocket::loginSuccess, this, &InputHandler::onLoginSuccess);
    connect(m_socket, &CClientSocket::registrationSuccess, this, &InputHandler::onRegistrationSuccess);
    connect(m_socket, &CClientSocket::loginFailed, this, &InputHandler::onLoginFailed);
    connect(m_socket, &CClientSocket::registrationFailed, this, &InputHandler::onRegistrationFailed);
    connect(m_socket, &CClientSocket::forceLogout, this, [this](const QString& message) {
        qDebug() << RED_TEXT_START << "Forced logout:" << message << RED_TEXT_END;
        currentState = State::INIT;
        qDebug() << "\nDo you want to register or login? (register/login):";
    });

    // 添加新的连接处理踢人相关的响应
    connect(m_socket, &CClientSocket::playerKicked, this, [](const QString& player) {
        qDebug() << player << "has been kicked from the room";
    });

    connect(m_socket, &CClientSocket::kickFailed, this, [](const QString& message) {
        qDebug() << "Failed to kick player:" << message;
    });

    connect(m_socket, &CClientSocket::roomLeft, this, [this]() {
        returnToMainMenu();
    });

    connect(m_socket, &CClientSocket::spectateFailed, this, [this](const QString& message) {
        qDebug() << "Failed to spectate room:" << message;
        returnToMainMenu();
    });

    connect(m_socket, &CClientSocket::roomJoinFailed, this, [this](const QString& message) {
        qDebug() << "Failed to join room:" << message;
        returnToMainMenu();
    });

    connect(m_socket, &CClientSocket::ownershipTransferred, this, [](const QString& newOwner) {
        qDebug() << "Room ownership transferred to" << newOwner;
    });

    connect(m_socket, &CClientSocket::transferFailed, this, [](const QString& message) {
        qDebug() << "Failed to transfer ownership:" << message;
    });

    // 添加游戏状态变化的连接
    connect(m_socket, &CClientSocket::gameStateChanged, this, &InputHandler::onGameStateChanged);
}

void InputHandler::run()
{
    // 首先输出初始提示
    qDebug() << "Do you want to register or login? (register/login):";

    QTextStream qtin(stdin);
    QString input;

    while (!isInterruptionRequested()) {
        // 等待输入
        input = qtin.readLine();
        if (!input.isEmpty()) {
            // 在主线程中处理输入
            QMetaObject::invokeMethod(this, "processUserInput",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString, input));
        }

        // 给事件循环一些时间处理其他事件
        QThread::msleep(100);
    }
}

void InputHandler::processUserInput(const QString& input)
{
    static QString username, password, action;

    switch (currentState) {
    case State::INIT:
        if (input.toLower() == "register") {
            action = input.toLower();
            qDebug() << "Registration requirements:";
            qDebug() << "- Username must be 3-15 characters and contain only letters, numbers, and underscores";
            qDebug() << "- Password must be at least 6 characters and can contain letters, numbers, and special characters (@#$%^&+=)";
            qDebug() << "\nPlease enter username:";
            currentState = State::WAITING_USERNAME;
        } else if (input.toLower() == "login") {
            action = input.toLower();
            qDebug() << "Please enter username:";
            currentState = State::WAITING_USERNAME;
        } else {
            qDebug() << "Do you want to register or login? (register/login):";
        }
        break;

    case State::WAITING_USERNAME:
        username = input.trimmed();
        if (action == "register") {
            // 检查用户名格式
            QRegularExpression usernameRegex("^[a-zA-Z0-9_]{3,15}$");
            if (!usernameRegex.match(username).hasMatch()) {
                qDebug() << "Invalid username format!";
                qDebug() << "Username must be 3-15 characters and contain only letters, numbers, and underscores";
                qDebug() << "Please enter username again:";
                return;
            }
        }
        qDebug() << "Please enter password:";
        currentState = State::WAITING_PASSWORD;
        break;

    case State::WAITING_PASSWORD:
        password = input.trimmed();
        if (action == "register") {
            // 检查密码格式
            QRegularExpression passwordRegex("^[a-zA-Z0-9@#$%^&+=]{6,}$");
            if (!passwordRegex.match(password).hasMatch()) {
                qDebug() << "Invalid password format!";
                qDebug() << "Password must be at least 6 characters and can contain letters, numbers, and special characters (@#$%^&+=)";
                qDebug() << "Please enter password again:";
                return;
            }
        }
        if (action == "register") {
            m_socket->sendRegisterRequest(username, password);
        } else {
            m_socket->sendLoginRequest(username, password);
        }
        break;

    case State::WAITING_CHOICE:
        if (input == "1") {
            // 创建房间
            m_socket->sendCreateRoomRequest(8);
            currentState = State::IN_ROOM;
        } else if (input == "2") {
            // 加入房间
            qDebug() << "Enter room code:";
            currentState = State::WAITING_ROOM_CODE;
        } else if (input == "3") {
            // 观战
            qDebug() << "Enter room code to spectate:";
            currentState = State::WAITING_ROOM_CODE;
            m_socket->setProperty("is_spectating", true);
        } else {
            qDebug() << "Invalid choice. Please enter 1, 2 or 3:";
            showMainMenu();  // 显示主菜单
        }
        break;

    case State::WAITING_ROOM_CODE:
        if (m_socket->property("is_spectating").toBool()) {
            QJsonObject spectateRequest;
            spectateRequest.insert("type", "spectate");
            spectateRequest.insert("room_code", input.trimmed());
            m_socket->write(QJsonDocument(spectateRequest).toJson(QJsonDocument::Compact) + "\n");
            m_socket->setProperty("is_spectating", false);
        } else {
            m_socket->sendJoinRoomRequest(input.trimmed());
        }
        currentState = State::IN_ROOM;
        break;

    case State::IN_ROOM:
        if (input.startsWith("/")) {
            if (input == "/help") {
                qDebug() << "\nAvailable commands:";
                qDebug() << "/help    - Show this help message";
                qDebug() << "/ready   - Toggle ready status";
                qDebug() << "/start   - Start the game (room owner only)";
                qDebug() << "/kick <username>    - Kick a player (room owner only)";
                qDebug() << "/transfer <username> - Transfer room ownership (room owner only)";
                qDebug() << "/leave   - Leave the current room";
                qDebug() << "/say <message>      - Send a chat message to all players";
            }
            else if (input == "/leave") {
                QJsonObject leaveRequest;
                leaveRequest.insert("type", "leave_room");
                m_socket->write(QJsonDocument(leaveRequest).toJson(QJsonDocument::Compact) + "\n");
            }
            else if (input.startsWith("/kick ")) {
                QString playerToKick = input.mid(6).trimmed();
                if (!playerToKick.isEmpty()) {
                    QString currentUsername = m_socket->property("username").toString();
                    if (playerToKick == currentUsername) {
                        qDebug() << "You cannot kick yourself!";
                        return;
                    }
                    QJsonObject kickRequest;
                    kickRequest.insert("type", "kick_player");
                    kickRequest.insert("player", playerToKick);
                    m_socket->write(QJsonDocument(kickRequest).toJson(QJsonDocument::Compact) + "\n");
                } else {
                    qDebug() << "Usage: /kick <username>";
                }
            }
            else if (input.startsWith("/transfer ")) {
                QString newOwner = input.mid(10).trimmed();
                if (!newOwner.isEmpty()) {
                    QString currentUsername = m_socket->property("username").toString();
                    if (newOwner == currentUsername) {
                        qDebug() << "You are already the room owner!";
                        return;
                    }
                    QJsonObject transferRequest;
                    transferRequest.insert("type", "transfer_ownership");
                    transferRequest.insert("new_owner", newOwner);
                    m_socket->write(QJsonDocument(transferRequest).toJson(QJsonDocument::Compact) + "\n");
                } else {
                    qDebug() << "Usage: /transfer <username>";
                }
            }
            else if (input == "/ready") {
                QJsonObject readyRequest;
                readyRequest.insert("type", "player_ready");
                readyRequest.insert("ready", true);
                m_socket->write(QJsonDocument(readyRequest).toJson(QJsonDocument::Compact) + "\n");
            }
            else if (input == "/start") {
                QJsonObject startRequest;
                startRequest.insert("type", "start_game");
                m_socket->write(QJsonDocument(startRequest).toJson(QJsonDocument::Compact) + "\n");
            }
            else if (input.startsWith("/say ")) {
                QString message = input.mid(5).trimmed();
                if (!message.isEmpty()) {
                    m_socket->sendChatMessage(message);
                } else {
                    qDebug() << "Usage: /say <message>";
                }
            }
            else if (input == "/say") {
                qDebug() << "Usage: /say <message>";
            }
            else {
                qDebug() << "Unknown command. Type /help to see available commands.";
            }
        } else {
            qDebug() << "Unknown command. Type /help to see available commands.";
        }
        break;

    case State::WAITING_DEAL:
        // 在发牌阶段，忽略所有输入
        if (input.contains(" ")) {  // 如果是类似"3 2"的输入
            qDebug() << "Please wait for dealing phase to complete.";
        }
        break;

    case State::DISCARD:
        if (!dealingPhaseComplete) {
            qDebug() << "Please wait for dealing phase to complete.";
            break;
        }

        {
            // 在弃牌阶段，直接处理数字输入
            QStringList parts = input.split(" ");
            if (parts.size() == 2) {
                bool ok1, ok2;
                int cardId = parts[0].toInt(&ok1);
                int count = parts[1].toInt(&ok2);

                if (ok1 && ok2) {
                    QJsonObject discardRequest;
                    discardRequest.insert("type", "discard_cards");
                    discardRequest.insert("card_id", cardId);
                    discardRequest.insert("count", count);
                    m_socket->write(QJsonDocument(discardRequest).toJson(QJsonDocument::Compact) + "\n");
                } else {
                    qDebug() << "Invalid input format. Please use: <CardID> <Count>";
                    qDebug() << "Example: '3 2' to discard 2 READ cards";
                }
            } else {
                qDebug() << "Invalid input format. Please use: <CardID> <Count>";
                qDebug() << "Example: '3 2' to discard 2 READ cards";
            }
        }
        break;

    case State::PLAY:
        {
            // 检查是否是当前玩家的回合
            QString currentPlayer = m_socket->property("current_player").toString();
            QString myUsername = m_socket->property("username").toString();
            
            if (currentPlayer != myUsername) {
                fprintf(stderr, "\033[31mError: Not your turn! Current player is: %s\033[0m\n", 
                    currentPlayer.toStdString().c_str());
                return;
            }

            QStringList parts = input.split(" ");
            if (parts.size() == 2) {
                bool ok1, ok2;
                int cardId = parts[0].toInt(&ok1);
                int faceUpInt = parts[1].toInt(&ok2);

                if (ok1 && ok2 && (faceUpInt == 0 || faceUpInt == 1)) {
                    qDebug() << "\n=== Sending Play Card Request ===";
                    qDebug() << "Card ID:" << cardId;
                    qDebug() << "Face Up:" << (faceUpInt == 1 ? "true" : "false");

                    QJsonObject playRequest;
                    playRequest.insert("type", "player_action");
                    playRequest.insert("action_type", PLAY_CARD);
                    playRequest.insert("card_id", cardId);
                    playRequest.insert("face_up", faceUpInt == 1);
                    playRequest.insert("player_id", myUsername);

                    m_socket->write(QJsonDocument(playRequest).toJson(QJsonDocument::Compact) + "\n");
                    qDebug() << "Play card request sent to server";
                } else {
                    qDebug() << "Invalid input format. Please use: <CardID> <FaceUp>";
                    qDebug() << "Example: '3 1' to play card face up";
                    qDebug() << "Example: '3 0' to play card face down";
                }
            } else {
                qDebug() << "Invalid input format. Please use: <CardID> <FaceUp>";
                qDebug() << "Example: '3 1' to play card face up";
                qDebug() << "Example: '3 0' to play card face down";
            }
        }
        break;

    case State::POINT: {
        QStringList parts = input.split(" ");
        if (parts.size() == 2) {
            bool ok;
            QString targetPlayerId = parts[0];
            int cardIndex = parts[1].toInt(&ok);
            
            if (ok) {
                QJsonObject actionJson;
                actionJson.insert("type", "player_action");
                actionJson.insert("action_type", static_cast<int>(POINT_OUT_CARD));
                actionJson.insert("player_id", m_socket->property("username").toString());
                actionJson.insert("target_player_id", targetPlayerId);
                actionJson.insert("card_index", cardIndex);
                
                m_socket->write(QJsonDocument(actionJson).toJson(QJsonDocument::Compact) + "\n");
                m_socket->flush();
                
                qDebug() << "Point out request sent:";
                qDebug() << "- Target player:" << targetPlayerId;
                qDebug() << "- Card index:" << cardIndex;
            }
        }else {
            qDebug() << "Invalid input format. Please use: <PlayerID> <CardIndex>";
            qDebug() << "Example: '123 1' to point out player 123's 1st card.";
        }
        break;
    }
    }
}

void InputHandler::onLoginSuccess()
{
    showMainMenu();
    currentState = State::WAITING_CHOICE;

    connect(m_socket, &CClientSocket::roomCreated, this, [this](const QString& roomCode) {
        qDebug() << "Room created successfully with code:" << roomCode;
        currentState = State::IN_ROOM;
    });

    connect(m_socket, &CClientSocket::roomCreationFailed, this, [](const QString& message) {
        qDebug() << "Failed to create room:" << message;
    });

    connect(m_socket, &CClientSocket::roomJoined, this, [this](const QString& roomCode) {
        qDebug() << "Successfully joined room:" << roomCode;
        currentState = State::IN_ROOM;
    });
}

void InputHandler::onRegistrationSuccess()
{
    showMainMenu();  // 注册成功后显示主菜单
    currentState = State::WAITING_CHOICE;
}

void InputHandler::onLoginFailed(const QString& message)
{
    qDebug() << "Login failed:" << message;
    qDebug() << "\nDo you want to register or login? (register/login):";
    currentState = State::INIT;  // 现在可以访问 currentState 了
}

void InputHandler::onRegistrationFailed(const QString& message)
{
    qDebug() << "Registration failed:" << message;
    qDebug() << "\nDo you want to register or login? (register/login):";
    currentState = State::INIT;  // 现在可以访问 currentState 了
}

// 添加一个显示主菜单的辅助函数
void InputHandler::showMainMenu()
{
    qDebug() << "Do you want to:";
    qDebug() << "1. Create a game room";
    qDebug() << "2. Join a game room";
    qDebug() << "3. Spectate a game";
    qDebug() << "Enter your choice (1/2/3):";
}

// 添加新的错误处理函数
void InputHandler::handleError(const QString& message)
{
    qDebug() << message;
    returnToMainMenu();
}

// 添加返回主菜单的函数
void InputHandler::returnToMainMenu()
{
    currentState = State::WAITING_CHOICE;
    showMainMenu();
}

// 添加一个新的槽函数来处理游戏状态变化
void InputHandler::onGameStateChanged(const QString& state) {
    if (state == "DEALING") {
        currentState = State::WAITING_DEAL;
        dealingPhaseComplete = false;
    } else if (state == "DISCARD") {
        currentState = State::DISCARD;
        dealingPhaseComplete = true;  // 只有在收到明确的发牌完成信号时才设置为 true
        qDebug() << "Dealing phase complete. You can now discard cards.";
    } else if (state == "PLAY") {
        currentState = State::PLAY;
    }else if (state == "POINT") {  // 修改这里
        currentState = State::POINT;
    } else if (state == "IN_ROOM") {
        currentState = State::IN_ROOM;
        showMainMenu();  // 显示房间菜单
    }
}
