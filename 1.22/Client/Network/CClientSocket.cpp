#include "CClientSocket.h"
#include "../Game/ClientGame/ClientGame.h"
#include <QHostAddress>
#include <QCoreApplication>
#include <QFile>
#include <QString>
#include <QtXml/QDomDocument>
#include <QXmlStreamReader>
#include <QBuffer>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QTimerEvent>
#include <QVariant>
#include <QTextStream>

// 添加红色文本的常量定义
const QString RED_TEXT_START = "\033[31m";
const QString RED_TEXT_END = "\033[0m";

CClientSocket::CClientSocket(QObject *parent,QString strIP, qint16 iPort)
    : QTcpSocket( parent ), m_clientGame(nullptr)
{
    strServerIP = strIP;
    iServerPort = iPort;
    iConnSeconds = 5;
    iTimerId = startTimer(iConnSeconds * 1000);

    connect( this, SIGNAL(readyRead()), this, SLOT(onReadyRead()), Qt::QueuedConnection);
    connect( this, SIGNAL(connected()), this, SLOT(onConnected()));
    connect( this, SIGNAL(disconnected()), this, SLOT(onDisConnected()));

    // 初始化心跳定时器
    heartbeatTimer = new QTimer(this);
    connect(heartbeatTimer, &QTimer::timeout, this, &CClientSocket::sendHeartbeat);
}
CClientSocket::~CClientSocket()
{
    if ( iTimerId != 0 )
        killTimer(iTimerId);
}
void CClientSocket::timerEvent( QTimerEvent *event )
{
    if(iTimerId == event->timerId())
        connectToServer(strServerIP, iServerPort);
}

void CClientSocket::connectToServer( QString ip, quint16 port )
{
    if(ip.contains("localhost", Qt::CaseInsensitive))
        ip="127.0.0.1";
    if(state() == QAbstractSocket::ConnectedState)
    {
        if(ip == strServerIP && port == iServerPort)
            return;
        disconnectFromHost();
    }
    if(state() == QAbstractSocket::UnconnectedState)
    {
        strServerIP = ip;
        iServerPort = port;
        QHostAddress addr(ip);
        connectToHost(addr, port);
    }
}

void CClientSocket::close()
{
    disconnectFromHost();
    QTcpSocket::close();
}

void CClientSocket::onReadyRead() {
    // 读取所有可用数据
    QByteArray data = this->readAll();

    // 如果已经登录，不显示登录相关的原始消息
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isNull() && doc.isObject()) {
        QJsonObject obj = doc.object();
        QString type = obj.value("type").toString();
        if ((type == "login" || type == "registration") && isLoggedIn) {
            return;  // 如已登录，直接忽登录相关消息
        }
    }

    // 将数据按换行符分割
    QList<QByteArray> jsonMessages = data.split('\n');

    // 遍历每个分割得到的字符串
    for (QByteArray messageData : jsonMessages) {
        if (messageData.isEmpty()) continue;

        // 尝试将字符串转换为JSON文档
        QJsonDocument doc = QJsonDocument::fromJson(messageData);
        if (doc.isNull()) {
            qDebug() << "Failed to create JSON doc.";
            continue;
        }
        if (!doc.isObject()) {
            qDebug() << "JSON is not an object.";
            continue;
        }

        // 获取JSON对象并处理
        QJsonObject jsonObj = doc.object();
        processJsonObject(jsonObj);
    }
}

void CClientSocket::processJsonObject(const QJsonObject &jsonObj) {
    QString type = jsonObj.value("type").toString();

    static bool dealPhaseEnded = false;
    static bool dealPhaseStarted = false;  // 添加新标志
    static bool discardResultShown = false;

    if (type == "game_state") {
        State currentState = static_cast<State>(jsonObj.value("state").toInt());

        // 如果包含出牌信息
        QJsonArray events = jsonObj.value("events").toArray();
        for (const auto& eventValue : events) {
            QJsonObject eventObj = eventValue.toObject();
            QString eventType = eventObj.value("type").toString();
            if (eventType == "PlayCard") {
                QJsonObject eventData = eventObj.value("data").toObject();
            }
        }

        QString myUsername = property("username").toString();

        // 发牌阶段开始时只输出一次提示
        if (currentState == SEND_CARD && !dealPhaseStarted) {
            dealPhaseStarted = true;  // 设置标志,防止重复输出
            qDebug() << "\nWaiting for cards to be dealt...";
        }

        // 发牌阶段结束时的总结
        if (currentState == DISCARD && !dealPhaseEnded) {
            dealPhaseEnded = true;

            QJsonArray players = jsonObj.value("players").toArray();
            for (const auto& playerValue : players) {
                QJsonObject playerObj = playerValue.toObject();
                if (playerObj.value("player_id").toString() == myUsername) {
                    QJsonArray handCards = playerObj.value("hand_cards").toArray();

                    qDebug() << "\n=== Your Hand After Dealing Phase ===";
                    qDebug() << "Total cards in hand:" << handCards.size();

                    if (!handCards.isEmpty()) {
                        qDebug() << "Cards breakdown (CardID : Card Name : Count):";
                        QMap<CardType, int> cardCounts;
                        for (const auto& cardValue : handCards) {
                            QJsonObject cardObj = cardValue.toObject();
                            CardType cardType = static_cast<CardType>(cardObj.value("card_type").toInt());
                            cardCounts[cardType]++;
                        }

                        for (auto it = cardCounts.begin(); it != cardCounts.end(); ++it) {
                            qDebug() << "-" << getCardID(it.key()) << ":"
                                     << getCardTypeName(it.key()) << ":"
                                     << it.value();
                        }

                        qDebug() << "\nYou have 15 seconds to discard cards.";
                        qDebug() << "Format: <CardID> <Count>";
                        qDebug() << "Example: '3 2' to discard 2 READ cards";
                    }
                    break;
                }
            }
        }
    }

    if (type == "login") {
        QString status = jsonObj.value("status").toString();
        if (status == "success") {
            // 设置 socket 的 username 属性
            setProperty("username", jsonObj.value("username").toString());
            isLoggedIn = true;
            startHeartbeat();
            emit loginSuccess();
        } else {
            QString message = jsonObj.value("message").toString();
            emit loginFailed(message);
        }
    } else if (type == "registration") {
        if (isLoggedIn) {
            qDebug() << "Already logged in, ignoring registration message";
            return;
        }

        QString status = jsonObj.value("status").toString();
        if (status == "success") {
            // 设置 socket 的 username 属性
            setProperty("username", jsonObj.value("username").toString());
            isLoggedIn = true;
            startHeartbeat();
            emit registrationSuccess();
        } else {
            QString message = jsonObj.value("message").toString();
            emit registrationFailed(message);
        }
    } else if (type == "heartbeat") {
        // 处理心跳响应，不需要出日志
    } else if (type == "create_room") {
        QString status = jsonObj.value("status").toString();
        if (status == "success") {
            QString roomCode = jsonObj.value("room_code").toString();
            emit roomCreated(roomCode);
        } else {
            QString message = jsonObj.value("message").toString();
            emit roomCreationFailed(message);
        }
    } else if (type == "join_room") {
        QString status = jsonObj.value("status").toString();
        if (status == "success") {
            QString roomCode = jsonObj.value("room_code").toString();
            emit roomJoined(roomCode);
        } else {
            QString message = jsonObj.value("message").toString();
            emit roomJoinFailed(message);
        }
    } else if (type == "room_message") {
        // 处理房间消息
        QString message = jsonObj.value("message").toString();
        qDebug() << "Room message:" << message;
    } else if (type == "room_status") {
        // 检查状态是否发生变化
        if (isRoomStatusChanged(jsonObj)) {
            // 新缓存的状态
            lastRoomStatus = jsonObj;

            // 处理房间状态更新
            int playerCount = jsonObj.value("player_count").toInt();
            bool gameStarted = jsonObj.value("game_started").toBool();
            QString roomCode = jsonObj.value("room_code").toString();
            QJsonArray players = jsonObj.value("players").toArray();
            QJsonArray spectators = jsonObj.value("spectators").toArray();

            qDebug() << "Room status update:";
            qDebug() << "- Room code:" << roomCode;
            qDebug() << "- Players:" << playerCount << "/" << jsonObj.value("capacity").toInt();
            qDebug() << "- Game started:" << (gameStarted ? "Yes" : "No");
            qDebug() << "- Player list:";
            for (const QJsonValue &playerVal : players) {
                QJsonObject playerObj = playerVal.toObject();
                QString username = playerObj["username"].toString();
                bool isReady = playerObj["ready"].toBool();
                bool isOwner = playerObj["is_owner"].toBool();

                QStringList status;
                if (isOwner) {
                    status.append("(Owner)");
                }
                if (isReady) {
                    status.append("(Ready)");
                }

                QString statusStr = status.join(" ");
                qDebug().nospace() << "  * " << username << (statusStr.isEmpty() ? "" : " " + statusStr);
            }

            if (!spectators.isEmpty()) {
                qDebug() << "- Spectators:";
                for (const QJsonValue &spectator : spectators) {
                    qDebug() << "  *" << spectator.toString();
                }
            }
        }
    } else if (type == "player_joined" || type == "player_left" || type == "player_kicked") {
        // 家加入/离开/被踢的消息
        QString player = jsonObj.value("player").toString();
        if (type == "player_joined") {
            qDebug() << player << "has joined the room";
        } else if (type == "player_left") {
            qDebug() << player << "has left the room";
        } else {
            qDebug() << player << "has been kicked from the room";
        }
    } else if (type == "player_ready_state") {
        QString player = jsonObj.value("player").toString();
        bool ready = jsonObj.value("ready").toBool();
        emit playerReadyStateChanged(player, ready);
        qDebug() << "Player" << player << (ready ? "is ready" : "is not ready");
    } else if (type == "spectate_response") {
        static bool handlingSpectateResponse = false;

        if (!handlingSpectateResponse) {
            handlingSpectateResponse = true;

            QString status = jsonObj.value("status").toString();
            if (status == "success") {
                QString roomCode = jsonObj.value("room_code").toString();
                emit spectateSuccess(roomCode);
                qDebug() << "Successfully joined room" << roomCode << "as spectator";
            } else {
                QString message = jsonObj.value("message").toString();
                emit spectateFailed(message);
                qDebug() << "Failed to spectate room:" << message;
            }

            QTimer::singleShot(100, []() {
                handlingSpectateResponse = false;
            });
        }
    } else if (type == "force_logout") {
        QString message = jsonObj.value("message").toString();
        qDebug() << "\nForced logout:" << message;

        // 停止心跳
        stopHeartbeat();

        // 重置状态
        isLoggedIn = false;

        // 断开连接
        disconnectFromHost();

        // 通知用户需要重新登录
        qDebug() << "Please login again:";

        // 可以发送一个信号通知 InputHandler
        emit forceLogout(message);
    } else if (type == "leave_response") {
        QString status = jsonObj.value("status").toString();
        if (status == "success") {
            qDebug() << "You have left the room";
            emit roomLeft();  // 触发返回主菜单
        }
    } else if (type == "kick_response") {
        QString status = jsonObj.value("status").toString();
        if (status == "success") {
            QString player = jsonObj.value("player").toString();
            emit playerKicked(player);
        } else {
            QString message = jsonObj.value("message").toString();
            emit kickFailed(message);
        }
    } else if (type == "kicked") {
        // 处理被踢出的消息
        static bool alreadyKicked = false;

        if (!alreadyKicked) {
            alreadyKicked = true;
            fprintf(stderr, "\033[31mYou have been kicked from the room\033[0m\n");
            emit roomLeft();  // 触发返回主菜单
            flush();

            QTimer::singleShot(1000, []() {
                alreadyKicked = false;
            });
        }
    } else if (type == "player_kicked") {
        // 处理其他玩家被踢出的消息
        QString player = jsonObj.value("player").toString();
        qDebug() << player << "has been kicked from the room";
    } else if (type == "ownership_transferred") {
        QString previousOwner = jsonObj.value("previous_owner").toString();
        QString newOwner = jsonObj.value("new_owner").toString();
        qDebug() << "Room ownership transferred from" << previousOwner << "to" << newOwner;
    } else if (type == "transfer_response") {
        QString status = jsonObj.value("status").toString();
        if (status == "success") {
            QString newOwner = jsonObj.value("new_owner").toString();
            emit ownershipTransferred(newOwner);
        } else {
            QString message = jsonObj.value("message").toString();
            emit transferFailed(message);
        }
    } else if (type == "room_closed") {
        QString message = jsonObj.value("message").toString();
        qDebug() << message;
        emit roomLeft();  // 触发返回主菜单
    } else if (type == "start_game_failed") {
        QString message = jsonObj.value("message").toString();
        qDebug() << "Failed to start game:" << message;
    } else if (type == "game_started") {
        QString message = jsonObj.value("message").toString();
        int playerCount = jsonObj.value("player_count").toInt();
        QJsonArray players = jsonObj.value("players").toArray();

        qDebug() << "\nGame started!";
        qDebug() << message;
        qDebug() << "Players in game:";
        for (const auto& player : players) {
            qDebug() << "- " << player.toString();
        }
    } else if (type == "game_state") {
        State currentState = static_cast<State>(jsonObj.value("state").toInt());

        // 发送状态变化信号
        QString stateStr;
        switch(currentState) {
        case SEND_CARD: stateStr = "DEALING"; break;
        case DISCARD: stateStr = "DISCARD"; break;
        case PLAY: stateStr = "PLAY"; break;
        case SHOW_HIDE: stateStr = "SHOW_HIDE"; break;
        case POINT_OUT: stateStr = "POINT"; break;
        case FLIP_CARD: stateStr = "FLIP_CARD"; break;
        case SETTLE: stateStr = "SETTLE"; break;
        default: stateStr = "UNKNOWN"; break;
        }
        emit gameStateChanged(stateStr);
        if(m_clientGame) {
            GameState* newState = GameState::fromJson(jsonObj);
            if(newState) {
                emit gameStateReceived(*newState);
                qDebug()<<"sending game_state to ClientGame.";
                delete newState;
            }
        }
    } else if (type == "deal_phase_summary") {
        QString playerId = jsonObj.value("player_id").toString();
        if (playerId == property("username").toString()) {
            QJsonObject cardSummary = jsonObj.value("cards").toObject();

            int totalCards = 0;
            QMap<CardType, int> cardCounts;

            // 解析卡牌统计信息
            for (auto it = cardSummary.begin(); it != cardSummary.end(); ++it) {
                CardType cardType = static_cast<CardType>(it.key().toInt());
                int count = it.value().toInt();
                cardCounts[cardType] = count;
                totalCards += count;
            }

            // 输出统计信息
            qDebug() << "\n=== Your Hand After Dealing Phase ===";
            qDebug() << "Total cards in hand:" << totalCards;
            qDebug() << "Cards breakdown (CardID : Card Name : Count):";

            // 使用 CardID 枚举显示每种卡牌的序号
            for (auto it = cardCounts.begin(); it != cardCounts.end(); ++it) {
                qDebug() << "-" << getCardID(it.key()) << ":"
                         << getCardTypeName(it.key()) << ":"
                         << it.value();
            }

            // 修改时间提示
            qDebug() << "\nYou have 60 seconds to discard cards.";  // 改为60秒
            qDebug() << "Format: <CardID> <Count>";
            qDebug() << "Example: '3 2' to discard 2 READ cards";
        }
    } else if (type == "discard_result") {
        QString playerId = jsonObj.value("player_id").toString();
        if (playerId == property("username").toString()) {
            // 获取统计数据
            int totalDiscarded = jsonObj.value("total_discarded").toInt();
            QJsonObject discardedStats = jsonObj.value("discarded_cards").toObject();
            int totalRemaining = jsonObj.value("total_remaining").toInt();
            QJsonObject remainingStats = jsonObj.value("remaining_cards").toObject();
            bool phaseComplete = jsonObj.value("phase_complete").toBool();
            bool isFinalResult = jsonObj.value("is_final_result").toBool();

            // 显示弃牌结果
            qDebug() << "\n=== Discard Phase Result ===";
            qDebug() << "Total discarded:" << totalDiscarded << "cards";

            if (totalDiscarded > 0) {  // 只有在有弃牌时才显示弃牌明细
                qDebug() << "Discarded cards breakdown (CardID : Card Name : Count):";
                for (auto it = discardedStats.begin(); it != discardedStats.end(); ++it) {
                    CardType type = static_cast<CardType>(it.key().toInt());
                    qDebug() << "-" << getCardID(type) << ":"
                             << "\"" << getCardTypeName(type) << "\""
                             << ":" << it.value().toInt();
                }
            }

            qDebug() << "\nRemaining cards:";
            qDebug() << "Total:" << totalRemaining;
            qDebug() << "Cards breakdown (CardID : Card Name : Count):";
            for (auto it = remainingStats.begin(); it != remainingStats.end(); ++it) {
                CardType type = static_cast<CardType>(it.key().toInt());
                qDebug() << "-" << getCardID(type) << ":"
                         << "\"" << getCardTypeName(type) << "\""
                         << ":" << it.value().toInt();
            }

            if (phaseComplete) {
                if (isFinalResult) {
                    qDebug() << "\nDiscard phase completed successfully. Entering play phase...";
                    emit discardPhaseFinalComplete();
                } else {
                    qDebug() << "\nYou have successfully discarded to 6 cards. Please wait for the timer to end...";
                    emit discardPhaseComplete();
                }
            } else if (totalRemaining > 6) {
                qDebug() << "\nYou still need to discard" << (totalRemaining - 6) << "more cards.";
                qDebug() << "Format: <CardID> <Count>";
                qDebug() << "Example: '3 2' to discard 2 READ cards";
            }
        }
    } else if (type == "chat_message") {
        QString sender = jsonObj.value("sender").toString();
        QString content = jsonObj.value("content").toString();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QTextStream out(stdout);
        out.setCodec("UTF-8");
        out << sender << ": " << content << Qt::endl;
#else
        QTextStream(stdout) << sender << ": " << content << Qt::endl;
#endif
    } else if (type == "game_state_changed") {
        QString newState = jsonObj.value("state").toString();

        if (newState == "PLAY") {
            qDebug() << "\n=== Play Phase Started ===";
            qDebug() << "How to play cards:";
            qDebug() << "1. Format: <CardID> <FaceUp>";
            qDebug() << "2. FaceUp: 1 for face up, 0 for face down";
            qDebug() << "3. Example: '3 1' - play READ card face up";
            qDebug() << "4. Example: '3 0' - play READ card face down";
        }

        // 发送状态变化信号
        emit gameStateChanged(newState);
    } else if (type == "dealing_complete") {
        // 发送状态变化信号，表示发牌阶段完成
        emit gameStateChanged("DISCARD");
    } else if (type == "hand_cards_info") {
        QString playerId = jsonObj.value("player_id").toString();
        QString username = property("username").toString();

        // 只显示自己的手牌信息
        if (playerId == username) {
            int totalCards = jsonObj.value("total_cards").toInt();
            QJsonObject handCards = jsonObj.value("hand_cards").toObject();

            qDebug() << "\nYour cards:";
            for (auto it = handCards.begin(); it != handCards.end(); ++it) {
                CardType type = static_cast<CardType>(it.key().toInt());
                int count = it.value().toInt();
                qDebug() << QString("- CardID %1: %2 (x%3)")
                                .arg(getCardID(type))
                                .arg(getCardTypeName(type))
                                .arg(count);
            }
        }
    } else if (type == "turn_change") {
        QString currentPlayer = jsonObj.value("current_player").toString();
        setProperty("current_player", currentPlayer);  // 更新当前玩家

        QString myUsername = property("username").toString();
        if (currentPlayer == myUsername) {
            qDebug() << "\nIt's your turn.";
        } else {
            qDebug() << "\nIt's" << currentPlayer << "'s turn.";
        }
    } else if (type == "your_turn") {
        QString message = jsonObj.value("message").toString();
        qDebug() << "\n" << message;

        // 确保设置当前玩家为自己
        setProperty("current_player", property("username").toString());
    } else if (type == "play_card_success") {
        QString playerId = jsonObj.value("player_id").toString();
        QString cardType = jsonObj.value("card_type").toString();
        bool faceUp = jsonObj.value("face_up").toBool();

        qDebug() << "\n=== Card Play Notification ===";
        qDebug() << "Player" << playerId << "played a" << cardType << "card";
        qDebug() << "Card was played" << (faceUp ? "face up" : "face down");
    } else if (type == "hide_card_revealed") {
        QString playerId = jsonObj.value("player_id").toString();
        int mysteriesCount = jsonObj.value("mysteries_count").toInt();

        qDebug() << "\n=== Hide Card Phase ===";
        qDebug() << "Player" << playerId << "revealed their HIDE card";
        qDebug() << "They now have" << mysteriesCount << "mysteries";
        qDebug() << "This player will become the player with the most mysteries";

        // 如果是自己的HIDE牌被翻开
        if (playerId == property("username").toString()) {
            qDebug() << "Your HIDE card has been revealed!";
        }
    }else if (type == "timeout_play_notification") {
        QString playerId = jsonObj.value("player_id").toString();
        QString cardType = jsonObj.value("card_type").toString();
        bool faceUp = jsonObj.value("face_up").toBool();

        qDebug() << "\n=== Timeout Auto Play ===";
        // 使用 Qt 的方式输出
        fprintf(stderr, "Player %s timeout - auto playing card\n",
                qPrintable(playerId));
        qDebug() << "Auto played card:" << cardType;
        qDebug() << "Card was played" << (faceUp ? "face up" : "face down");
    }else if (type == "point_out_phase_start") {
        qDebug() << "\n=== Point Out Phase Started ===";
        qDebug() << "You have 30 seconds to point out one face-down card.";
        qDebug() << "Format: <PlayerID> <CardIndex>";
        qDebug() << "Example: '123 2' to point out player 123's second card\n";

        QJsonArray playersInfo = jsonObj.value("players").toArray();

        // 添加调试信息
        qDebug() << "Received players info array size:" << playersInfo.size();

        for (const auto& playerValue : playersInfo) {
            QJsonObject playerInfo = playerValue.toObject();
            QString playerId = playerInfo.value("player_id").toString();
            QJsonArray cardsInfo = playerInfo.value("cards").toArray();

            qDebug() << "\nPlayer" << playerId << "cards:";
            qDebug() << "Total cards:" << cardsInfo.size();

            for (const auto& cardValue : cardsInfo) {
                QJsonObject cardInfo = cardValue.toObject();
                int index = cardInfo.value("index").toInt();
                QString cardType = cardInfo.value("type").toString();
                bool faceUp = cardInfo.value("face_up").toBool();

                QString displayText = QString("%1. %2").arg(index)
                                          .arg(faceUp ? cardType : "???");
                qDebug() << displayText;
            }
        }

        // 添加分隔线
        qDebug() << "\nPlease enter your point out command:";
    } else if (type == "card_pointed") {
        QString pointingPlayer = jsonObj.value("pointing_player").toString();
        QString targetPlayer = jsonObj.value("target_player").toString();
        int cardIndex = jsonObj.value("card_index").toInt();

        qDebug() << "\n=== Card Pointed ===";
        if (pointingPlayer == property("username").toString()) {
            qDebug() << "You pointed out player" << targetPlayer << "'s card at index" << cardIndex;
        } else {
            qDebug() << "Player" << pointingPlayer << "pointed out player"
                     << targetPlayer << "'s card at index" << cardIndex;
        }
    }else if (type == "point_out_success") {
        qDebug() << "\nYou have successfully pointed out a card!";
    }
    else if (type == "point_out_invalid") {
        QString errorMessage = jsonObj.value("message").toString();
        qDebug() << "\nInvalid point out. Try again.";
        qDebug() << "Reason:" << errorMessage;
        qDebug() << "\nPlease use format: <PlayerID> <CardIndex>";
        qDebug() << "Example: '123 2' to point out player 123's second card";
    }
    else if (type == "point_out_complete") {
        qDebug() << "\n=== Point Out Phase Complete ===";
        qDebug() << "All players have made their points.";
        qDebug() << "Moving to flip card phase...";

        // 发送状态变化信号
        emit gameStateChanged("FLIP_CARD");
    }
    else if (type == "point_out_timeout") {
        qDebug() << "\n=== Point Out Phase Timeout ===";
        qDebug() << "Time's up! Moving to flip card phase...";
        // 发送状态变化信号
        emit gameStateChanged("FLIP_CARD");
    }else if (type == "flip_card_phase_start") {
        qDebug() << "\n=== Flip Card Phase Started ===";
        qDebug() << "All pointed cards will now be revealed.";
    }else if (type == "pointed_cards_revealed") {
        QString message = jsonObj.value("message").toString();
        qDebug() << "\n=== Pointed Cards Revealed ===";
        qDebug() << message;  // "All pointed cards are: Read Trap ..."
    } else if (type == "settle_phase_start") {
        qDebug() << "\n=== Settlement Phase Started ===";
        QString message = jsonObj.value("message").toString();
        qDebug() << message;
        emit gameStateChanged("SETTLE");
    }
    else if (type == "game_winner") {
        QString winnerId = jsonObj.value("winner_id").toString();
        QString reason = jsonObj.value("reason").toString();

        qDebug() << "\n=== Game Winner Announced ===";
        qDebug() << "Winner:" << winnerId;
        qDebug() << "Reason:" << reason;

        // 如果是当前玩家获胜
        if (winnerId == property("username").toString()) {
            qDebug() << "Congratulations! You have won the game!";
        } else {
            qDebug() << "Game Over. Better luck next time!";
        }
    }else if (type == "return_to_room") {
        QString message = jsonObj.value("message").toString();
        qDebug() << "\n=== Returning to Room ===";
        qDebug() << message;

        // 发送状态变化信号
        emit gameStateChanged("IN_ROOM");

        // 显示提示信息
        qDebug() << "You can start a new game when ready.";
    }else {
        // 未知消类型，记录但不输出警告
        qDebug() << "Received message of type:" << type;
    }
}


void CClientSocket::onConnected()
{
    // 设置 socket 的编码
    setTextModeEnabled(true);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QTextStream stream(this);
    stream.setCodec("UTF-8");
#endif

    qDebug() << "Connected to server";
    // 不要在这里启动心跳，而是等待登录成功后再启动
}

void CClientSocket::onDisConnected()
{
    qDebug() << "Disconnected from server";
    stopHeartbeat();  // 在断开连接时停止心跳
    if (iTimerId == 0) {
        iTimerId = startTimer(iConnSeconds * 1000);
    }
    emit connectionLost();  // 添加新信号通知断开
}

bool CClientSocket::socketIsConnected()
{
    return (state() == QAbstractSocket::ConnectedState);
}


void CClientSocket::sendDataToServer(QString str)
{
    if (state() == QAbstractSocket::ConnectedState) {
        // 如果是 JSON 字符串，直接发送
        if (str.startsWith("{")) {
            write(str.toUtf8() + "\n");
        } else {
            // 如果是普通字符串转换为 JSON 格式
            QJsonObject obj;
            obj.insert("type", "message");
            obj.insert("content", str);
            QJsonDocument doc(obj);
            write(doc.toJson(QJsonDocument::Compact) + "\n");
        }
    } else {
        qDebug() << "Cannot send data: socket is not connected.";
    }
}

void CClientSocket::sendLoginRequest(const QString& username, const QString& password)
{
    QJsonObject requestObj;
    requestObj.insert("type", "login");
    requestObj.insert("username", username);
    requestObj.insert("password", password);
    QJsonDocument requestDoc(requestObj);
    QByteArray jsonData = requestDoc.toJson(QJsonDocument::Compact);
    write(jsonData + "\n");
}

void CClientSocket::sendStartGameRequest() {
    QJsonObject requestObj;
    requestObj.insert("type", "start_game");
    QJsonDocument requestDoc(requestObj);
    write(requestDoc.toJson(QJsonDocument::Compact) + "\n");
}

void CClientSocket::sendRegisterRequest(const QString& username, const QString& password)
{
    QJsonObject requestObj;
    requestObj.insert("type", "register");
    requestObj.insert("username", username);
    requestObj.insert("password", password);
    QJsonDocument requestDoc(requestObj);
    QByteArray jsonData = requestDoc.toJson(QJsonDocument::Compact);
    write(jsonData + "\n");
}
void CClientSocket::sendCreateRoomRequest(int capacity)
{
    QJsonObject requestObj;
    requestObj.insert("type", "create_room");
    requestObj.insert("capacity", capacity);
    QJsonDocument requestDoc(requestObj);
    write(requestDoc.toJson(QJsonDocument::Compact) + "\n");
}
void CClientSocket::sendJoinRoomRequest(const QString& roomCode)
{
    QJsonObject requestObj;
    requestObj.insert("type", "join_room");
    requestObj.insert("room_code", roomCode);
    QJsonDocument requestDoc(requestObj);
    QByteArray jsonData = requestDoc.toJson(QJsonDocument::Compact);
    write(jsonData + "\n");
}

void CClientSocket::startHeartbeat()
{
    qDebug() << "Client: Starting heartbeat timer...";
    if (!heartbeatTimer->isActive()) {  // 确保定时器没有在运行
        heartbeatTimer->start(5000);  // 每5秒发送一次心跳
        qDebug() << "Client: Heartbeat timer started";
        sendHeartbeat();  // 立即发送第一个心跳
    }
}

void CClientSocket::stopHeartbeat()
{
    if (heartbeatTimer->isActive()) {
        qDebug() << "Client: Stopping heartbeat timer";
        heartbeatTimer->stop();
    }
}

void CClientSocket::sendHeartbeat()
{
    if (state() == QAbstractSocket::ConnectedState) {
        QJsonObject heartbeatObj;
        heartbeatObj.insert("type", "heartbeat");
        QJsonDocument doc(heartbeatObj);
        write(doc.toJson(QJsonDocument::Compact) + "\n");
    } else {
        qDebug() << "Client: Cannot send heartbeat - not connected";
        stopHeartbeat();  // 如果连接断开，停止心跳
    }
}

void CClientSocket::sendReadyRequest(bool ready)
{
    QJsonObject requestObj;
    requestObj.insert("type", "player_ready");
    requestObj.insert("ready", ready);
    QJsonDocument requestDoc(requestObj);
    write(requestDoc.toJson(QJsonDocument::Compact) + "\n");
}

void CClientSocket::sendSpectateRequest(const QString& roomCode)
{
    QJsonObject requestObj;
    requestObj.insert("type", "spectate");
    requestObj.insert("room_code", roomCode);
    QJsonDocument requestDoc(requestObj);
    write(requestDoc.toJson(QJsonDocument::Compact) + "\n");
}

bool CClientSocket::isRoomStatusChanged(const QJsonObject& newStatus) {
    // 比较关键字段是否发生变化
    if (lastRoomStatus.isEmpty()) return true;  // 第一次总是显示

    // 比较基本信息
    if (lastRoomStatus["room_code"] != newStatus["room_code"]) return true;
    if (lastRoomStatus["capacity"] != newStatus["capacity"]) return true;
    if (lastRoomStatus["game_started"] != newStatus["game_started"]) return true;
    if (lastRoomStatus["player_count"] != newStatus["player_count"]) return true;

    // 比较玩家列表
    QJsonArray lastPlayers = lastRoomStatus["players"].toArray();
    QJsonArray newPlayers = newStatus["players"].toArray();
    if (lastPlayers.size() != newPlayers.size()) return true;

    for (int i = 0; i < lastPlayers.size(); ++i) {
        QJsonObject lastPlayer = lastPlayers[i].toObject();
        QJsonObject newPlayer = newPlayers[i].toObject();

        if (lastPlayer["username"] != newPlayer["username"]) return true;
        if (lastPlayer["ready"] != newPlayer["ready"]) return true;
        if (lastPlayer["is_owner"] != newPlayer["is_owner"]) return true;
    }

    // 比较观战者列表
    QJsonArray lastSpectators = lastRoomStatus["spectators"].toArray();
    QJsonArray newSpectators = newStatus["spectators"].toArray();
    if (lastSpectators.size() != newSpectators.size()) return true;

    for (int i = 0; i < lastSpectators.size(); ++i) {
        if (lastSpectators[i] != newSpectators[i]) return true;
    }

    return false;  // 如果所有比较都同，返回false
}

void CClientSocket::sendGameAction(const QJsonObject& action) {
    if (state() == QAbstractSocket::ConnectedState) {
        QJsonDocument doc(action);
        QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";
        write(data);
        flush();
        qDebug() << "Sent game action:" << QString(data);
    } else {
        qDebug() << "Cannot send game action: socket is not connected";
    }
}

// 添加一个辅助函数来获取卡牌类型的名称
QString CClientSocket::getCardTypeName(CardType type) {
    switch(type) {
    case ANSWER: return "Answer";
    case SOLVE: return "Solve";
    case READ: return "Read";
    case THINK: return "Think";
    case INDUCE: return "Induce";
    case TRAP: return "Trap";
    case TRADE: return "Trade";
    case STEAL: return "Steal";
    case REVERSE: return "Reverse";
    case HIDE: return "Hide";
    case COVER: return "Cover";
    case FIRST: return "First";
    default: return "Unknown";
    }
}

int CClientSocket::getCardID(CardType type) {
    switch(type) {
    case ANSWER: return ANSWER_ID;
    case SOLVE: return SOLVE_ID;
    case READ: return READ_ID;
    case THINK: return THINK_ID;
    case INDUCE: return INDUCE_ID;
    case TRAP: return TRAP_ID;
    case TRADE: return TRADE_ID;
    case STEAL: return STEAL_ID;
    case REVERSE: return REVERSE_ID;
    case HIDE: return HIDE_ID;
    case COVER: return COVER_ID;
    case FIRST: return FIRST_ID;
    default: return 0;
    }
}

void CClientSocket::sendChatMessage(const QString& message) {
    QJsonObject chatMsg;
    chatMsg.insert("type", "chat_message");
    chatMsg.insert("content", message);
    chatMsg.insert("sender", property("username").toString());

    QJsonDocument doc(chatMsg);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    // 确保使用 UTF-8 编码
    write(data.append('\n'));
}
