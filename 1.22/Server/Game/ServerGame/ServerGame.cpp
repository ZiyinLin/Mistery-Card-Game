#include "Card.h"
#include "GameState.h"
#include "PlayerAction.h"
#include "Player.h"
#include "ServerGame.h"
#include "GameEvent.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>
#include <algorithm>
#include <random>
#include <iostream>
#include <chrono>
using namespace std;
using card_pointer = std::shared_ptr<Card>;

void ServerGame::startGameLogic() {
    try {
        qDebug() << "\n=== Starting New Game ===";

        // 清空所有玩家的手牌和出牌区
        for (Player* player : players) {
            player->getHandCards().clear();
            player->getPlayedCards().clear();
        }

        // 清空公共弃牌堆
        Player::discarded_cards.clear();  // 添加这一行来清空弃牌堆

        // 设置游戏状态为发牌阶段
        setState(SEND_CARD);

        // 发送游戏状态更新
        QJsonObject stateUpdate;
        stateUpdate.insert("type", "game_state_changed");
        stateUpdate.insert("state", "DEALING");
        for (QTcpSocket* socket : playerSockets) {
            socket->write(QJsonDocument(stateUpdate).toJson(QJsonDocument::Compact) + "\n");
        }

        // 开始发牌
        QTimer::singleShot(1000, this, [this]() {
            dealCards();
        });

    } catch (const std::exception& e) {
        qDebug() << "Error starting game:" << e.what();
    }
}

void ServerGame::dealCards() {
    try {
        qDebug() << "\n=== Starting Deal Phase ===";
        qDebug() << "Players in game:";
        for (const auto& player : players) {
            qDebug() << "- Player:" << player->getPlayerID();
        }

        // 1. 检查玩家列表
        if (players.empty()) {
            qWarning() << "Error: No players to deal cards to!";
            return;
        }

        // 2. 初始化牌堆
        deck.clear();
        qDebug() << "\nInitializing deck...";

        // 3. 创建所有类型的卡牌并加入牌堆
        try {
            // 单张牌
            deck.push_back(std::make_shared<Card>(CardType::ANSWER, "Effect for Answer"));
            deck.push_back(std::make_shared<Card>(CardType::FIRST, "Effect for First"));
            deck.push_back(std::make_shared<Card>(CardType::HIDE, "Effect for Hide"));
            deck.push_back(std::make_shared<Card>(CardType::REVERSE, "Effect for Reverse"));
            qDebug() << "Added single cards: Answer, First, Hide, Reverse";

            // 三张牌
            for(int i = 0; i < 3; i++) {
                deck.push_back(std::make_shared<Card>(CardType::THINK, "Effect for Think"));
                deck.push_back(std::make_shared<Card>(CardType::TRADE, "Effect for Trade"));
            }
            qDebug() << "Added 3 copies each of: Think, Trade";

            // 五张牌
            for(int i = 0; i < 5; i++) {
                deck.push_back(std::make_shared<Card>(CardType::COVER, "Effect for Cover"));
                deck.push_back(std::make_shared<Card>(CardType::INDUCE, "Effect for Induce"));
            }
            qDebug() << "Added 5 copies each of: Cover, Induce";

            // 六张牌
            for(int i = 0; i < 6; i++) {
                deck.push_back(std::make_shared<Card>(CardType::SOLVE, "Effect for Solve"));
                deck.push_back(std::make_shared<Card>(CardType::STEAL, "Effect for Steal"));
                deck.push_back(std::make_shared<Card>(CardType::TRAP, "Effect for Trap"));
            }
            qDebug() << "Added 6 copies each of: Solve, Steal, Trap";

            // 十二张牌
            for(int i = 0; i < 12; i++) {
                deck.push_back(std::make_shared<Card>(CardType::READ, "Effect for Read"));
            }
            qDebug() << "Added 12 copies of: Read";

            qDebug() << "\nTotal cards in deck:" << deck.size();
        } catch (const std::exception& e) {
            qWarning() << "Error creating cards:" << e.what();
            return;
        }

        // 4. 洗牌
        try {
            unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
            std::shuffle(deck.begin(), deck.end(), std::default_random_engine(seed));
            qDebug() << "Deck shuffled successfully";
        } catch (const std::exception& e) {
            qWarning() << "Error shuffling cards:" << e.what();
            return;
        }

        // 5. 创建发牌定时器
        QTimer* dealTimer = new QTimer(this);
        int* currentPlayerIndex = new int(0);
        int* totalDealtCards = new int(0);  // 添加计数器

        qDebug() << "Deal timer started - dealing cards every 100ms";

        connect(dealTimer, &QTimer::timeout, this, [this, dealTimer, currentPlayerIndex, totalDealtCards]() {
            try {
                if (deck.empty() || players.empty() || *totalDealtCards >= deck.size()) {
                    qDebug() << "\n=== Deal Phase Complete ===";

                    // 为每个玩家发送手牌总结信息
                    for (Player* player : players) {
                        QJsonObject summary;
                        summary["type"] = "deal_phase_summary";
                        summary["player_id"] = player->getPlayerID();

                        // 统计该玩家的手牌
                        QMap<CardType, int> cardCounts;
                        for (const auto& card : player->getHandCards()) {
                            cardCounts[card->getType()]++;
                        }

                        // 将统计结果转换为JSON数组
                        QJsonObject cardSummary;
                        for (auto it = cardCounts.begin(); it != cardCounts.end(); ++it) {
                            cardSummary[QString::number(static_cast<int>(it.key()))] = it.value();
                        }
                        summary["cards"] = cardSummary;

                        // 找到对应的socket发送
                        for (QTcpSocket* socket : playerSockets) {
                            if (socket->property("username").toString() == player->getPlayerID()) {
                                socket->write(QJsonDocument(summary).toJson(QJsonDocument::Compact) + "\n");
                                socket->flush();
                                break;
                            }
                        }
                    }

                    delete currentPlayerIndex;
                    delete totalDealtCards;
                    dealTimer->stop();
                    dealTimer->deleteLater();

                    // 发牌完成后，等待一段时间再进入弃牌阶段
                    QTimer::singleShot(1000, this, [this]() {
                        handleDiscardPhase();
                    });
                    return;
                }

                // 给当前玩家发一张牌
                Player* currentPlayer = players[*currentPlayerIndex];
                card_pointer card = deck[*totalDealtCards];
                currentPlayer->addHandCard(card);
                (*totalDealtCards)++;  // 增加已发牌计数

                qDebug() << "Dealt" << getCardTypeName(card->getType())
                         << "to player" << currentPlayer->getPlayerID();

                // 创建发牌事件
                GameEvent event;
                event.setEventType("DealCard");
                QJsonObject eventData;
                eventData.insert("player", currentPlayer->toJson());
                eventData.insert("card", card->toJson());
                QQueue<GameEvent> events;
                events.append(event);
                game_state.setEvents(events);

                // 发送游戏状态更新
                sendGameState(game_state);
                game_state.clearEvents();

                // 更新玩家索引
                *currentPlayerIndex = (*currentPlayerIndex + 1) % players.size();

            } catch (const std::exception& e) {
                qWarning() << "Error in deal timer:" << e.what();
                dealTimer->stop();
                dealTimer->deleteLater();
                delete currentPlayerIndex;
                delete totalDealtCards;
            }
        });

        dealTimer->start(100);

    } catch (const std::exception& e) {
        qWarning() << "Fatal error in dealCards:" << e.what();
    }

    // 在所有牌都发完后，发送一个明确的发牌完成信号
    QTimer::singleShot(5000, [this]() {  // 5秒后（确保所有牌都发完）发送完成信号
        QJsonObject dealCompleteMsg;
        dealCompleteMsg.insert("type", "dealing_complete");
        for (QTcpSocket* socket : playerSockets) {
            if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                socket->write(QJsonDocument(dealCompleteMsg).toJson(QJsonDocument::Compact) + "\n");
            }
        }
    });

}

// 新增一个处理弃牌阶段的函数
void ServerGame::handleDiscardPhase() {
    QTimer* discardTimer = new QTimer(this);
    discardTimer->setInterval(3000); // 3秒弃牌时间

    connect(discardTimer, &QTimer::timeout, this, [this, discardTimer]() {
        // 检查所有玩家的手牌数量
        for (Player* player : players) {
            if (player->getHandCards().size() > 6) {
                // 强制弃牌
                forceDiscard(player);
            }
        }

        // 发送最终的弃牌结果
        for (Player* player : players) {
            sendDiscardResult(player);
        }

        // 进入出牌阶段
        setState(PLAY);

        // 创建并发送阶段变更通知
        QJsonObject phaseChangeNotification;
        phaseChangeNotification.insert("type", "game_state_changed");
        phaseChangeNotification.insert("state", "PLAY");
        phaseChangeNotification.insert("game_state", game_state.toJson());

        // 广播给所有玩家
        for (QTcpSocket* socket : playerSockets) {
            if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                socket->write(QJsonDocument(phaseChangeNotification).toJson(QJsonDocument::Compact) + "\n");
                socket->flush();
            }
        }

        // 开始处理出牌阶段
        handlePlayCard();

        // 清理定时器
        discardTimer->stop();
        discardTimer->deleteLater();
    });

    discardTimer->start();
}

void ServerGame::forceDiscard(Player* player) {
    while (player->getHandCards().size() > 6) {
        // 找到第一张可以弃置的牌（不是谜底和匿和先手）
        for (auto& card : player->getHandCards()) {
            if (card->getType() != ANSWER && card->getType() != HIDE && card->getType() != FIRST) {
                player->discardCard(card);
                break;
            }
        }
    }
}

void ServerGame::sendDiscardResult(Player* player) {
    QJsonObject response;
    response.insert("type", "discard_result");
    response.insert("player_id", player->getPlayerID());

    // 统计弃牌堆
    QJsonObject discardedStats;
    int totalDiscarded = 0;
    for (const auto& card : player->getDiscardedCards()) {
        CardType type = card->getType();
        discardedStats[QString::number(static_cast<int>(type))] =
            discardedStats[QString::number(static_cast<int>(type))].toInt() + 1;
        totalDiscarded++;
    }

    // 统计剩余手牌
    QJsonObject remainingStats;
    int totalRemaining = 0;
    for (const auto& card : player->getHandCards()) {
        CardType type = card->getType();
        remainingStats[QString::number(static_cast<int>(type))] =
            remainingStats[QString::number(static_cast<int>(type))].toInt() + 1;
        totalRemaining++;
    }

    // 添加所有统计信息
    response.insert("total_discarded", totalDiscarded);
    response.insert("discarded_cards", discardedStats);
    response.insert("total_remaining", totalRemaining);
    response.insert("remaining_cards", remainingStats);
    response.insert("phase_complete", totalRemaining <= 6);

    // 添加是否为最终结果的标志
    response.insert("is_final_result", true);  // 在发送最终统计时设为 true

    // 发送响应
    QTcpSocket* socket = findPlayerSocket(player);
    if (socket) {
        qDebug() << "Sending discard result to player" << player->getPlayerID();
        qDebug() << "Total discarded:" << totalDiscarded;
        qDebug() << "Total remaining:" << totalRemaining;
        socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
    }
}

bool ServerGame::allPlayerHandEmpty(){
    //检查所有玩家是否手牌为空
    for(auto player : players){
        if(!player->getHandCards().empty()){
            return false;
        }
    }
    return true;
}


//获取所有玩家
std::vector<Player*> ServerGame::getPlayers(){
    return players;
}

void ServerGame::receivePlayerAction(const QByteArray& data, QTcpSocket* playerSocket) {
    if (!playerSocket) {
        qWarning() << "Received action with null socket!";
        return;
    }

    // 存储动作数据
    receivedActions[playerSocket] = data;
    qDebug() << "===================Received action from player:" << playerSocket->property("username").toString()<<"===============";

}



void ServerGame::sendGameState(const GameState& gameState) {
    QJsonObject json = gameState.toJson();

    // 移除不必要的详细信息，只保留关键数据
    if (json.contains("events")) {
        QJsonArray events = json["events"].toArray();
        QJsonArray simplifiedEvents;
        for (const auto& event : events) {
            QJsonObject simplifiedEvent = event.toObject();
            // 简化事件数据，只保留必要信息
            if (simplifiedEvent.contains("data")) {
                QJsonObject data = simplifiedEvent["data"].toObject();
                // 移除详细的卡牌历史记录等
                if (data.contains("player")) {
                    QJsonObject player = data["player"].toObject();
                    player.remove("discarded_cards");  // 移除弃牌历史
                    data["player"] = player;
                }
                simplifiedEvent["data"] = data;
            }
            simplifiedEvents.append(simplifiedEvent);
        }
        json["events"] = simplifiedEvents;
    }

    QJsonDocument doc(json);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";

    qDebug() << "Sending game state, size:" << data.size() << "bytes";

    for (QTcpSocket* socket : playerSockets) {
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            socket->write(data);
            socket->flush();
        }
    }
}

void ServerGame::sendGameState(QTcpSocket* playerSocket, const GameState& gameState) {
    if (playerSocket && playerSocket->state() == QAbstractSocket::ConnectedState) {
        QJsonObject json = gameState.toJson();
        QJsonDocument doc(json);
        QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";
        playerSocket->write(data);
        playerSocket->flush();
    }
}


void ServerGame::setState(State state) {
    game_state.setState(state);

    // 发出状态改变信号
    emit gameStateChanged(game_state.toJson());
}

void ServerGame::setNextPlayer() {
    if (players.empty()) {
        qWarning() << "No players in the game!";
        current_player = nullptr;
        return;
    }

    // 如果当前玩家为空，从第一个玩开始
    if (!current_player) {
        current_player = players.front();
        qDebug() << "Starting with the first player:" << current_player->getPlayerID();
        return;
    }

    // 查找当前玩家的位置
    auto it = std::find(players.begin(), players.end(), current_player);
    if (it == players.end()) {
        qWarning() << "Current player not found in the player list!";
        current_player = players.front(); // 回到第一个玩家
        qDebug() << "Resetting to the first player:" << current_player->getPlayerID();
        return;
    }

    // 移动到下一个玩家
    ++it;
    if (it == players.end()) {
        // 如果到达末尾，回到第一个玩家
        current_player = players.front();
        qDebug() << "Reached end of the player list, looping back to:" << current_player->getPlayerID();
    } else {
        // 否则设置为下一个家
        current_player = *it;
        qDebug() << "Next player is:" << current_player->getPlayerID();
    }
}

Player* ServerGame::getCurrentPlayer() {
    // 实现获取当前玩家的逻辑
    return current_player; // 临时返回
}

void ServerGame::handleDiscardCard() {

}

void ServerGame::handlePlayCard() {
    // 设置当前玩家为第一个玩家
    qDebug() << "\n=== Starting Play Phase ===";
    // 检查当前玩家是否为空
    if (!current_player) {
        qDebug() << "Error: Current player is null, trying to find First card holder...";

        // 尝试找到持有"First"牌的玩家
        for (Player* player : players) {
            const std::vector<card_pointer>& playerHands = player->getHandCards();
            for (const card_pointer& singleCard : playerHands) {
                if (singleCard->getType() == FIRST) {
                    current_player = player;
                    qDebug() << "Found First card holder:" << player->getPlayerID();
                    break;
                }
            }
            if (current_player) break;
        }

        // 如果还是找不到，使用第一个玩家
        if (!current_player && !players.empty()) {
            current_player = players.front();
            qDebug() << "No First card found, using first player:" << current_player->getPlayerID();
        }

        // 如果还是为空，说明出现严重错误
        if (!current_player) {
            qDebug() << "Fatal error: Cannot find any valid player!";
            return;
        }
    }
    qDebug() << "Current player:" << (current_player ? current_player->getPlayerID() : "null");

    // 通知所有玩家当前回合玩家
    QJsonObject turnNotification;
    turnNotification.insert("type", "turn_change");
    turnNotification.insert("current_player", current_player->getPlayerID());

    // 广播回合信息并为当前玩家发送手牌信息
    for (QTcpSocket* socket : playerSockets) {
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            socket->write(QJsonDocument(turnNotification).toJson(QJsonDocument::Compact) + "\n");
            socket->flush();

            // 如果是当前玩家,发送其手牌信息
            if (socket->property("username").toString() == current_player->getPlayerID()) {
                sendHandCardsInfo(current_player);
            }
        }
    }

    // 创建回合计时器
    QTimer* turnTimer = new QTimer(this);
    turnTimer->setInterval(30000); // 30秒出牌时间

    // 创建轮询计时器，每100ms检查一次玩家动作
    QTimer* pollTimer = new QTimer(this);
    pollTimer->setInterval(3000);

    // 连接轮询计时器的超时信号
    connect(pollTimer, &QTimer::timeout, this, [this, turnTimer, pollTimer]() {
        // 检查当前玩家是否有动作
        if (processPlayerActionForPlayer(current_player)) {
            // 如果处理了玩家动作，停止两个计时器
            turnTimer->stop();
            pollTimer->stop();
            turnTimer->deleteLater();
            pollTimer->deleteLater();

            if (allPlayerHandEmpty()) {
                handleShowHide();
                return;
            }

            // 切换到下一个玩家
            setNextPlayer();

            // 为新玩家开始新的回合
            handlePlayCard();
        }
    });

    // 连接回合计时器的超时信号
    connect(turnTimer, &QTimer::timeout, this, [this, turnTimer, pollTimer]() {
        qDebug() << "\n=== Turn Timeout ===";
        qDebug() << "Player" << current_player->getPlayerID() << "did not play a card";

        // 获取玩家手牌
        auto& handCards = current_player->getHandCards();
        if (!handCards.empty()) {
            card_pointer cardToPlay = handCards.front();

            // 如果找到可以出的牌
            if (cardToPlay) {
                qDebug() << "Auto playing card:" << getCardTypeName(cardToPlay->getType());

                // 执行出牌
                current_player->playCard(cardToPlay, game_state, false);
                // 添加调试信息
                qDebug() << "After auto playing card, player's played cards count:"
                         << current_player->getPlayedCards().size();

                // 创建出牌事件
                GameEvent playEvent;
                playEvent.setEventType("PlayCard");
                QJsonObject eventData;
                eventData.insert("player", current_player->toJson());
                eventData.insert("card", cardToPlay->toJson());
                QQueue<GameEvent> events;
                events.enqueue(playEvent);
                game_state.setEvents(events);

                // 发送游戏状态更新
                sendGameState(game_state);
                game_state.clearEvents();

                // 通知所有玩家有玩家超时自动出牌
                QJsonObject timeoutPlayNotification;
                timeoutPlayNotification.insert("type", "timeout_play_notification");  // 改用不同的type
                timeoutPlayNotification.insert("player", current_player->getPlayerID());
                timeoutPlayNotification.insert("card_type", getCardTypeName(cardToPlay->getType()));
                timeoutPlayNotification.insert("face_up", false);
                timeoutPlayNotification.insert("is_timeout", true);  // 添加超时标记

                for (QTcpSocket* socket : playerSockets) {
                    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                        socket->write(QJsonDocument(timeoutPlayNotification).toJson(QJsonDocument::Compact) + "\n");
                        socket->flush();
                    }
                }
            }
        }
        // 回合时间到，停止两个计时器
        turnTimer->stop();
        pollTimer->stop();
        turnTimer->deleteLater();
        pollTimer->deleteLater();

        // 通知当前玩家超时
        QJsonObject timeoutNotification;
        timeoutNotification.insert("type", "turn_timeout");
        timeoutNotification.insert("player_id", current_player->getPlayerID());

        for (QTcpSocket* socket : playerSockets) {
            if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                socket->write(QJsonDocument(timeoutNotification).toJson(QJsonDocument::Compact) + "\n");
                socket->flush();
            }
        }
        if (allPlayerHandEmpty()) {
            handleShowHide();
            return;
        }
        // 切换到下一个玩家
        setNextPlayer();

        // 为新玩家开始新的回合
        handlePlayCard();
    });

    // 启动两个计时器
    turnTimer->start();
    pollTimer->start();
}

bool ServerGame::processPlayerActionForPlayer(Player* player) {
    qDebug() << "\n=== Processing Player Action For Player ===";
    qDebug() << "Current player ID:" << player->getPlayerID();

    QTcpSocket* playerSocket = nullptr;

    // 找到对应的玩家 socket
    for (QTcpSocket* socket : playerSockets) {
        if (socket->property("username").toString() == player->getPlayerID()) {
            playerSocket = socket;
            break;
        }
    }
    if (!playerSocket) {
        qDebug() << "Error: No socket found for player:" << player->getPlayerID();
        return false;
    }
    qDebug() << "Found player socket";

    // 检查是否有来自该玩家的动作
    if (!receivedActions.contains(playerSocket)) {
        qDebug() << "No actions received from player";
        return false;
    }
    qDebug() << "Found action in receivedActions";

    // 获取动作数据
    QByteArray actionData = receivedActions.take(playerSocket);
    qDebug() << "Action data:" << QString(actionData);

    // 解析动作
    QJsonDocument doc = QJsonDocument::fromJson(actionData);
    if (!doc.isObject()) {
        qDebug() << "Error: Invalid JSON format in action data";
        return false;
    }
    qDebug() << "Successfully parsed JSON data";

    QJsonObject actionJson = doc.object();
    PlayerAction action = PlayerAction::fromJson(actionJson);
    qDebug() << "Created PlayerAction object";


    if(action.getActionType()==PLAY_CARD){
        int cardId = action.getCard() ? getCardID(action.getCard()->getType()) : 0;
        // 检查玩家是否有这张牌
        bool hasCard = false;
        card_pointer cardToPlay = nullptr;
        for (const auto& card : player->getHandCards()) {
            if (getCardID(card->getType()) == cardId) {
                hasCard = true;
                cardToPlay = card;
                break;
            }
        }

        if (!hasCard) {
            qDebug() << "Error: Player does not have this card!";
            // 发送错误消息给玩家
            QTcpSocket* playerSocket = findPlayerSocket(player);
            if (playerSocket) {
                QJsonObject errorMsg;
                errorMsg.insert("type", "error");
                errorMsg.insert("message", "You don't have this card in your hand!");

                QByteArray errorData = QJsonDocument(errorMsg).toJson(QJsonDocument::Compact) + "\n";
                playerSocket->write(errorData);
                playerSocket->flush();
            }
            return false;
        }



        if(cardToPlay->getValidPlay()==false) {
            qDebug() << "Error: Invalid play action for card";
            return false;
        }
        qDebug() << "Card play is valid";

        // 处理动作
        qDebug() << "Calling processPlayerAction...";
        processPlayerAction(action);
        qDebug() << "Action processed successfully for player:" << player->getPlayerID();

        return true;
    }
    else if(action.getActionType()==POINT_OUT_CARD){
        qDebug()<<"In process player action for player, ready to go to process player action......";
        processPlayerAction(action);
        return true;
    }
}

//ServerGame::processPlayerAction
/**
 * @brief 处理PlayerAction对象
 * 服务器获取PlayerAction象内封装的玩家动作数据，包括动作类型（弃牌、出牌、选择别人的牌、指认牌），动作玩家，卡，目标玩家和目标卡牌。
 * 服务器先创建 gameState，再根据这些数据，将对应数据写入打包进gameState中发送至户端。
 * @param action PlayerAction对象，记录着本次玩家动作的数据。
 */
void ServerGame::processPlayerAction(const PlayerAction& action) {
    try {
        qDebug() << "\n=== Processing Player Action ===";
        qDebug() << "Player ID:" << action.getPlayerID();
        qDebug() << "Action Type:" << static_cast<int>(action.getActionType());

        // 获取执行动作的玩家
        Player* player = nullptr;
        for (auto p : players) {
            if (p->getPlayerID() == action.getPlayerID()) {
                player = p;
                break;
            }
        }

        if (!player) {
            qDebug() << "Error: Player not found!";
            return;
        }

        // 根据动作类型分别处理
        switch (action.getActionType()) {
        case PLAY_CARD: {
            try {
                int cardId = action.getCard() ? getCardID(action.getCard()->getType()) : 0;
                bool faceUp = action.getCard() ? action.getCard()->getFaceUp() : false;

                qDebug() << "\n=== Processing Play Card ===";
                qDebug() << "Looking for card with ID:" << cardId;
                qDebug() << "Face Up:" << (faceUp ? "true" : "false");

                // 在玩家手牌中找到对应的卡牌
                card_pointer cardToPlay = nullptr;
                for (const auto& card : player->getHandCards()) {
                    if (getCardID(card->getType()) == cardId) {
                        cardToPlay = card;
                        break;
                    }
                }

                if (cardToPlay) {
                    player->playCard(cardToPlay, game_state, faceUp);
                    qDebug() << "After playing card, player's played cards count:"
                             << player->getPlayedCards().size();
                    qDebug() << "Found card to play:" << getCardTypeName(cardToPlay->getType());

                    // 创建出牌成功的通知
                    QJsonObject playSuccessNotification;
                    playSuccessNotification.insert("type", "play_card_success");
                    playSuccessNotification.insert("player_id", player->getPlayerID());
                    playSuccessNotification.insert("card_type", getCardTypeName(cardToPlay->getType()));
                    playSuccessNotification.insert("face_up", faceUp);

                    // 广播通知
                    for (QTcpSocket* socket : playerSockets) {
                        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                            socket->write(QJsonDocument(playSuccessNotification).toJson(QJsonDocument::Compact) + "\n");
                            socket->flush();
                        }
                    }

                    // 创建事件并更新游戏状态
                    GameEvent playEvent;
                    playEvent.setEventType("PlayCard");
                    QJsonObject eventData;
                    eventData["player"] = player->toJson();
                    eventData["card"] = cardToPlay->toJson();
                    playEvent.setEventData(eventData);

                    QQueue<GameEvent> events = game_state.getEvents();
                    events.enqueue(playEvent);
                    game_state.setEvents(events);

                    sendGameState(game_state);
                    game_state.clearEvents();
                    sendHandCardsInfo(player);
                } else {
                    qDebug() << "Error: Card not found in player's hand!";
                }
            } catch (const std::exception& e) {
                qDebug() << "Error in PLAY_CARD action:" << e.what();
                QTcpSocket* socket = findPlayerSocket(player);
                if (socket) {
                    QJsonObject errorMsg;
                    errorMsg.insert("type", "error");
                    errorMsg.insert("message", "Failed to process play card action");
                    socket->write(QJsonDocument(errorMsg).toJson(QJsonDocument::Compact) + "\n");
                }
            }
            break;
        }

        case POINT_OUT_CARD: {
            try {
                qDebug() << "\n========== Processing Point Out Action ==========";
                qDebug() << "Action details:";
                QString pointingPlayerId = action.getPlayerID();
                QString targetPlayerId = action.getTargetPlayerID();
                int cardIndex = action.getCardIndex();
                qDebug() << "- Pointing player ID:" << pointingPlayerId;
                qDebug() << "- Target player ID:" << targetPlayerId;
                qDebug() << "- Card index:" << cardIndex;

                // 检查是否已经指认过
                qDebug() << "\nChecking if player has already pointed:";
                qDebug() << "Current point status:" << playerPointedStatus[pointingPlayerId];
                if (playerPointedStatus[pointingPlayerId]) {
                    qDebug() << "ERROR: Player has already pointed out a card";
                    QJsonObject errorNotification;
                    errorNotification.insert("type", "point_out_invalid");
                    errorNotification.insert("message", "You have already pointed out a card");

                    QTcpSocket* socket = findPlayerSocket(getPlayer(pointingPlayerId));
                    if (socket) {
                        socket->write(QJsonDocument(errorNotification).toJson(QJsonDocument::Compact) + "\n");
                        socket->flush();
                        qDebug() << "Sent error notification to player";
                    }
                    return;
                }
                qDebug() << "Point status check passed";

                // 检查目标玩家是否存在
                qDebug() << "\nLooking for target player:";
                Player* targetPlayer = getPlayer(targetPlayerId);
                if (!targetPlayer) {
                    qDebug() << "ERROR: Target player not found:" << targetPlayerId;
                    QJsonObject errorNotification;
                    errorNotification.insert("type", "point_out_invalid");
                    errorNotification.insert("message", "Invalid target player");

                    QTcpSocket* socket = findPlayerSocket(getPlayer(pointingPlayerId));
                    if (socket) {
                        socket->write(QJsonDocument(errorNotification).toJson(QJsonDocument::Compact) + "\n");
                        socket->flush();
                        qDebug() << "Sent error notification to player";
                    }
                    return;
                }
                qDebug() << "Target player found";

                // 检查是否在指认自己的牌
                if (pointingPlayerId == targetPlayerId) {
                    qDebug() << "ERROR: Player attempting to point out their own card";
                    QJsonObject errorNotification;
                    errorNotification.insert("type", "point_out_invalid");
                    errorNotification.insert("message", "You cannot point out your own cards");

                    QTcpSocket* socket = findPlayerSocket(getPlayer(pointingPlayerId));
                    if (socket) {
                        socket->write(QJsonDocument(errorNotification).toJson(QJsonDocument::Compact) + "\n");
                        socket->flush();
                        qDebug() << "Sent error notification to player";
                    }
                    return;
                }


                // 检查卡牌索引是否有效
                qDebug() << "\nValidating card index:";
                const auto& playedCards = targetPlayer->getPlayedCards();
                qDebug() << "Target player's played cards count:" << playedCards.size();
                qDebug() << "Requested card index:" << cardIndex;
                if (cardIndex <= 0 || cardIndex > static_cast<int>(playedCards.size())) {
                    qDebug() << "ERROR: Invalid card index";
                    game_state.sendErrorMessage(pointingPlayerId, "Invalid card index");
                    return;
                }
                qDebug() << "Card index is valid";

                // 获取被指认的卡牌
                qDebug() << "\nGetting pointed card:";
                card_pointer pointedCard = playedCards[cardIndex - 1];
                qDebug() << "Card type:" << getCardTypeName(pointedCard->getType());
                qDebug() << "Card face up status:" << pointedCard->getFaceUp();

                // 检查卡牌是否暗置
                if (pointedCard->getFaceUp()) {
                    qDebug() << "ERROR: Cannot point out face-up card";
                    game_state.sendErrorMessage(pointingPlayerId, "Cannot point out face-up card");
                    return;
                }
                qDebug() << "Card face-up check passed";

                // 添加到被指认卡牌列表
                qDebug() << "\nAdding card to pointed cards list:";
                qDebug() << "Current pointed cards count:" << pointedCards.size();
                pointedCards.push_back(pointedCard);
                qDebug() << "New pointed cards count:" << pointedCards.size();
                qDebug() << "Card successfully added to pointed cards list";

                // 标记该玩家已完成指认
                qDebug() << "\nUpdating player point status:";
                playerPointedStatus[pointingPlayerId] = true;
                pointed_count++;
                qDebug() << "Total points made:" << pointed_count;

                // 通知所有玩家有新的指认
                qDebug() << "\nBroadcasting point notification:";
                QJsonObject pointOutNotification;
                pointOutNotification.insert("type", "card_pointed");
                pointOutNotification.insert("pointing_player", pointingPlayerId);
                pointOutNotification.insert("target_player", targetPlayerId);
                pointOutNotification.insert("card_index", cardIndex);

                int notifiedPlayers = 0;
                for (QTcpSocket* socket : playerSockets) {
                    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                        socket->write(QJsonDocument(pointOutNotification).toJson(QJsonDocument::Compact) + "\n");
                        socket->flush();
                        notifiedPlayers++;
                    }
                }
                qDebug() << "Notification sent to" << notifiedPlayers << "players";

                // 发送成功消息给指认玩家
                qDebug() << "\nSending success notification to pointing player:";
                QJsonObject successNotification;
                successNotification.insert("type", "point_out_success");

                QTcpSocket* pointingPlayerSocket = findPlayerSocket(getPlayer(pointingPlayerId));
                if (pointingPlayerSocket) {
                    pointingPlayerSocket->write(QJsonDocument(successNotification).toJson(QJsonDocument::Compact) + "\n");
                    pointingPlayerSocket->flush();
                    qDebug() << "Success notification sent";
                }

                qDebug() << "\n========== Point Out Action Completed Successfully ==========\n";
            } catch (const std::exception& e) {
                qDebug() << "\nERROR in POINT_OUT_CARD action:" << e.what();
                QTcpSocket* socket = findPlayerSocket(player);
                if (socket) {
                    QJsonObject errorMsg;
                    errorMsg.insert("type", "error");
                    errorMsg.insert("message", "Failed to process point out action");
                    socket->write(QJsonDocument(errorMsg).toJson(QJsonDocument::Compact) + "\n");
                    qDebug() << "Error notification sent to player";
                }
            }
            break;
        }

        default:
            qDebug() << "Error: Unknown action type:" << static_cast<int>(action.getActionType());
            break;
        }
    } catch (const std::exception& e) {
        qDebug() << "Critical error in processPlayerAction:" << e.what();
    } catch (...) {
        qDebug() << "Unknown error in processPlayerAction";
    }
}

void ServerGame::handleShowHide() {
    try {
        // 记录每个玩家的谜点数
        qDebug() << "\n=== Recording Mystery Points ===";
        playerMysteryPoints.clear();  // 清除旧的记录

        for (const auto& player : players) {
            qDebug() << "\nChecking player:" << player->getPlayerID();
            qDebug() << "Played cards:";
            for (const auto& card : player->getPlayedCards()) {
                qDebug() << "- Card:" << getCardTypeName(card->getType())
                << "Face up:" << card->getFaceUp();
            }

            int mysteries = player->getMysteries();
            playerMysteryPoints[player->getPlayerID()] = mysteries;
            qDebug() << "Player" << player->getPlayerID()
                     << "mystery points recorded:" << mysteries;
        }
        qDebug() << "\n=== Starting Show Hide Phase ===";

        flipHideCard();  // 执行亮操作
        qDebug() << "\nShow hide phase complete.";
        // setState 已经在 flipHideCard 中设置
        sendGameState(game_state);
        game_state.clearEvents();
        handlePointOut();
    } catch (const std::exception& e) {
        qDebug() << "Error in handleShowHide:" << e.what();
    }
}

//ServerGame::flipCard
/**
 * @brief 掀开“匿”牌
 * 当所有玩家出牌完毕后，在进入指认阶段前，服务器会先调用此函数，掀开场上暗置的“匿”牌（若明置则直接return）。持有此牌的玩家成为场上“谜”点数
 * 最多的玩家（若出牌阶段被掀则无效）。
 */
void ServerGame::flipHideCard() {
    game_state.setState(SHOW_HIDE);
    Player* player = nullptr;

    qDebug() << "\n=== Flipping Hide Card ===";
    qDebug() << "Start flipping Hide card....";
    qDebug() << "Total players:" << players.size();

    try {
        // 遍历所有玩家
        for(auto& currentPlayer : players) {
            qDebug() << "\nChecking player:" << currentPlayer->getPlayerID();
            const auto& playedCards = currentPlayer->getPlayedCards();
            qDebug() << "Played cards count:" << playedCards.size();

            // 输出该玩家的所有出牌信息
            qDebug() << "Player's played cards:";
            for(const auto& card : playedCards) {
                qDebug() << "- Card type:" << static_cast<int>(card->getType())
                << " (" << card->getCardTypeName() << ")";
                qDebug() << "  Face up:" << card->getFaceUp();
            }

            // 寻找HIDE牌
            for(auto& card : playedCards) {
                qDebug() << "Checking card type:" << static_cast<int>(card->getType())
                << " (" << card->getCardTypeName() << ")";

                if(card->getType() == CardType::HIDE) {
                    qDebug() << "Found HIDE card!";
                    qDebug() << "Face up status:" << card->getFaceUp();

                    if(card->getFaceUp()) {
                        qDebug() << "Hide card had been flipped in play phase.";
                        return;
                    } else {
                        player = currentPlayer;
                        card->setFaceUp(true);

                        try {
                            card->play(player, game_state, true);
                            qDebug() << "Successfully flip the hide card.";
                            qDebug() << "Player mysteries count:" << player->getMysteries();

                            // 创建通知
                            QJsonObject hideRevealNotification;
                            hideRevealNotification.insert("type", "hide_card_revealed");
                            hideRevealNotification.insert("player_id", player->getPlayerID());
                            hideRevealNotification.insert("mysteries_count", player->getMysteries());

                            qDebug() << "Start broadcasting to sockets.";
                            qDebug() << "Total sockets:" << playerSockets.size();

                            // 广播
                            int broadcastCount = 0;
                            for (QTcpSocket* socket : playerSockets) {
                                if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                                    QByteArray notificationData = QJsonDocument(hideRevealNotification).toJson(QJsonDocument::Compact) + "\n";
                                    qDebug() << "Sending notification to socket:" << socket->property("username").toString();
                                    socket->write(notificationData);
                                    socket->flush();
                                    broadcastCount++;
                                }
                            }

                            qDebug() << "Broadcast complete. Sent to" << broadcastCount << "players";

                            // 发送游戏状态更新
                            qDebug() << "Sending updated game state...";
                            sendGameState(game_state);
                            game_state.clearEvents();

                            // 设置下一个阶段
                            setState(POINT_OUT);
                            return;
                        } catch (const std::exception& e) {
                            qDebug() << "Error during card play:" << e.what();
                        }
                    }
                }
            }
        }

        qDebug() << "Warning: No HIDE card found in any player's played cards!";
    } catch (const std::exception& e) {
        qDebug() << "Error in flipHideCard:" << e.what();
    }
}

void ServerGame::handlePointOut() {
    qDebug() << "\n=== Starting Point Out Phase ===";

    // 设置游戏状态为指认阶段
    setState(POINT_OUT);

    // 重置指认状态
    playerPointedStatus.clear();
    pointed_count = 0;
    pointedCards.clear();

    // 初始化每个玩家的指认状态
    for (const auto& player : players) {
        playerPointedStatus[player->getPlayerID()] = false;
        qDebug() << "Initialized point status for player:" << player->getPlayerID();
    }

    // 发送牌面信息
    sendPointOutPhaseInfo();

    // 发送游戏状态更新
    sendGameState(game_state);
    game_state.clearEvents();

    // 创建阶段计时器
    QTimer* phaseTimer = new QTimer(this);
    phaseTimer->setSingleShot(true);

    // 连接阶段计时器
    connect(phaseTimer, &QTimer::timeout, this, [this, phaseTimer]() {
        qDebug() << "\n=== Point Out Phase Timeout ===";
        qDebug() << "Processing all received point out actions...";

        // 处理所有收到的指认动作
        for (auto it = receivedActions.begin(); it != receivedActions.end(); ++it) {
            QTcpSocket* playerSocket = it.key();
            QByteArray actionData = it.value();

            // 解析动作
            QJsonDocument doc = QJsonDocument::fromJson(actionData);
            if (!doc.isObject()) {
                qDebug() << "Error: Invalid JSON format in action data";
                continue;
            }

            QJsonObject actionJson = doc.object();
            PlayerAction action = PlayerAction::fromJson(actionJson);

            // 只处理指认动作
            if (action.getActionType() == POINT_OUT_CARD) {
                qDebug() << "Processing point out action from player:"
                         << playerSocket->property("username").toString();
                processPlayerAction(action);
            }
        }

        // 清空接收到的动作
        receivedActions.clear();

        // 发送指认阶段结束通知
        QJsonObject phaseEndNotification;
        phaseEndNotification.insert("type", "point_out_complete");
        phaseEndNotification.insert("total_points", pointed_count);

        for (QTcpSocket* socket : playerSockets) {
            if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                socket->write(QJsonDocument(phaseEndNotification).toJson(QJsonDocument::Compact) + "\n");
                socket->flush();
            }
        }

        // 进入翻牌阶段
        setState(FLIP_CARD);
        sendGameState(game_state);
        game_state.clearEvents();
        handleFlipCard();

        // 清理计时器
        phaseTimer->deleteLater();
    });

    // 启动计时器
    phaseTimer->start(30000);  // 30秒超时
}

// 添加新的辅助函数
void ServerGame::sendPointOutPhaseInfo() {
    QJsonObject cardDisplayInfo;
    cardDisplayInfo.insert("type", "point_out_phase_start");

    QJsonArray playersInfo;
    for (const auto& player : players) {
        QJsonObject playerInfo;
        playerInfo.insert("player_id", player->getPlayerID());

        // 只发送必要的牌面信息
        QJsonArray cardsInfo;
        int cardIndex = 1;
        for (const auto& card : player->getPlayedCards()) {
            QJsonObject cardInfo;
            cardInfo.insert("index", cardIndex++);
            cardInfo.insert("type", card->getCardTypeName());
            cardInfo.insert("face_up", card->getFaceUp());
            cardsInfo.append(cardInfo);
        }
        playerInfo.insert("cards", cardsInfo);
        playersInfo.append(playerInfo);
    }
    cardDisplayInfo.insert("players", playersInfo);

    // 分别发送给每个玩家
    for (QTcpSocket* socket : playerSockets) {
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            QByteArray data = QJsonDocument(cardDisplayInfo).toJson(QJsonDocument::Compact) + "\n";
            socket->write(data);
            socket->flush();
        }
    }
}

void ServerGame::handlePointOutPhaseEnd() {
    qDebug() << "=== Point Out Phase Complete ===";
    qDebug() << "Total points made:" << pointed_count;

    // 发送指认阶段结束通知
    QJsonObject phaseEndNotification;
    phaseEndNotification.insert("type", "point_out_complete");

    for (QTcpSocket* socket : playerSockets) {
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            socket->write(QJsonDocument(phaseEndNotification).toJson(QJsonDocument::Compact) + "\n");
            socket->flush();
        }
    }

    // 进入翻牌阶段
    setState(FLIP_CARD);

    // 发送游戏状态更新
    sendGameState(game_state);
    game_state.clearEvents();

    // 处理翻牌阶段
    handleFlipCard();
}

void ServerGame::handleFlipCard() {
    qDebug() << "\n=== Starting Flip Card Phase ===";

    // 发送翻牌阶段开始通知
    QJsonObject flipPhaseNotification;
    flipPhaseNotification.insert("type", "flip_card_phase_start");

    for (QTcpSocket* socket : playerSockets) {
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            socket->write(QJsonDocument(flipPhaseNotification).toJson(QJsonDocument::Compact) + "\n");
            socket->flush();
        }
    }

    // 生成被指认卡牌的类型列表
    QString cardsList;
    for (const auto& card : pointedCards) {
        cardsList += getCardTypeName(card->getType()) + " ";
        card->setFaceUp(true);  // 将卡牌翻开
    }

    // 发送所有被指认的卡牌信息
    QJsonObject pointedCardsNotification;
    pointedCardsNotification.insert("type", "pointed_cards_revealed");
    pointedCardsNotification.insert("message", QString("All pointed cards are: %1").arg(cardsList.trimmed()));

    qDebug() << "Revealing pointed cards:" << cardsList;

    // 广播给所有玩家
    for (QTcpSocket* socket : playerSockets) {
        if (socket && socket->state() == QAbstractSocket::ConnectedState) {
            socket->write(QJsonDocument(pointedCardsNotification).toJson(QJsonDocument::Compact) + "\n");
            socket->flush();
        }
    }

    // 创建翻牌事件
    for (const auto& card : pointedCards) {
        GameEvent flipEvent;
        flipEvent.setEventType("FlipCard");
        QJsonObject eventData;
        eventData["card"] = card->toJson();
        flipEvent.setEventData(eventData);

        QQueue<GameEvent> events = game_state.getEvents();
        events.enqueue(flipEvent);
        game_state.setEvents(events);
    }

    // 发送游戏状态更新
    sendGameState(game_state);
    game_state.clearEvents();

    qDebug() << "ServerGame: Flip-card phase complete.";
    setState(State::SETTLE);

    // 进入结算阶段
    handleSettle();
}

//ServerGame::flipPointedCards
/**
 * @brief 掀开被指认的牌
 * 服务器有一个私有卡牌类vector容器：pointedCards，在指认阶段每有玩家指认场上的一张暗置牌，服务器将此牌加入到vector中。所有玩家指认完毕后，
 * 服务器调用此函数，将被指认的暗置牌全部掀开。
 */
void ServerGame::flipPointedCards(){
    GameState gameState;
    QQueue<GameEvent> events;
    gameState.setState(FLIP_CARD);
    for(auto card:pointedCards){
        GameEvent event;
        QJsonObject eventData;
        event.setEventType("FlipCard");
        eventData["card"]=card->toJson();
        event.setEventData(eventData);
        events.append(event);
    }
    gameState.setEvents(events);
    vector<Player*> players = game_state.getAllPlayers();
    for(Player*currentPlayer:players){
        gameState.appendPlayer(currentPlayer);
    }
    this->sendGameState(gameState);
}

void ServerGame::handleSettle() {
    try {
        qDebug() << "\n=== Starting Settlement Phase ===";

        // 发送结算阶段开始通知
        QJsonObject settleNotification;
        settleNotification.insert("type", "settle_phase_start");
        settleNotification.insert("message", "Game is now entering settlement phase...");

        for (QTcpSocket* socket : playerSockets) {
            if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                socket->write(QJsonDocument(settleNotification).toJson(QJsonDocument::Compact) + "\n");
                socket->flush();
            }
        }

        checkWinner();  // 判断获胜者
        qDebug() << "ServerGame: Game settlement complete.";

        // 等待3秒后返回房间
        QTimer::singleShot(3000, this, [this]() {
            returnToRoom();
        });
    } catch (const std::exception& e) {
        qDebug() << "Error in handleSettle:" << e.what();
    }
}

void ServerGame::checkWinner() {
    try {
        qDebug() << "\n=== Checking Winner ===";
        Player* winner = nullptr;
        QString winReason;

        // 检查谜底牌
        for (auto& currentPlayer : players) {
            try {
                for (auto& card : currentPlayer->getPlayedCards()) {
                    if (card->getType() == CardType::ANSWER) {
                        if (!card->getFaceUp()) {
                            winner = currentPlayer;
                            winReason = "Answer card remained face-down";
                            qDebug() << "Winner found:" << winner->getPlayerID();
                            qDebug() << "Win reason:" << winReason;
                            announceWinner(winner, winReason);
                            return;
                        }
                    }
                }
            } catch (const std::exception& e) {
                qDebug() << "Error checking player's cards:" << e.what();
                qDebug() << "Player ID:" << currentPlayer->getPlayerID();
                continue;
            }
        }

        // 使用记录的谜点数来判断获胜者
        try {
            int maxMysteries = 0;
            for (auto& currentPlayer : players) {
                try {
                    // 使用记录的谜点数而不是重新计算
                    int mysteries = playerMysteryPoints.value(currentPlayer->getPlayerID(), 0);
                    qDebug() << "Player" << currentPlayer->getPlayerID()
                             << "recorded mystery points:" << mysteries;
                    if (mysteries > maxMysteries) {
                        maxMysteries = mysteries;
                        winner = currentPlayer;
                    }
                } catch (const std::exception& e) {
                    qDebug() << "Error getting player mysteries:" << e.what();
                    qDebug() << "Player ID:" << currentPlayer->getPlayerID();
                    continue;
                }
            }

            if (winner) {
                winReason = QString("Most mysteries: %1").arg(maxMysteries);
                qDebug() << "Winner found:" << winner->getPlayerID();
                qDebug() << "Win reason:" << winReason;
                announceWinner(winner, winReason);
            } else {
                qDebug() << "Warning: No winner found!";
            }
        } catch (const std::exception& e) {
            qDebug() << "Error checking mysteries:" << e.what();
        }
    } catch (const std::exception& e) {
        qDebug() << "Critical error in checkWinner:" << e.what();
    } catch (...) {
        qDebug() << "Unknown error in checkWinner";
    }
}

void ServerGame::announceWinner(Player* player, const QString& reason) {
    try {
        qDebug() << "\n=== Announcing Winner ===";

        if (!player) {
            qDebug() << "Error: Winner is null!";
            return;
        }

        try {
            // 创建获胜通知
            QJsonObject winnerNotification;
            winnerNotification.insert("type", "game_winner");
            winnerNotification.insert("winner_id", player->getPlayerID());
            winnerNotification.insert("reason", reason);

            // 广播获胜消息
            for (QTcpSocket* socket : playerSockets) {
                if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                    socket->write(QJsonDocument(winnerNotification).toJson(QJsonDocument::Compact) + "\n");
                    socket->flush();
                }
            }

            // 创建游戏结束事件
            GameEvent event;
            event.setEventType("AnnounceWinner");
            QJsonObject eventData;
            eventData["player"] = player->toJson();
            eventData["reason"] = reason;
            event.setEventData(eventData);

            QQueue<GameEvent> events;
            events.append(event);

            // 设置游戏状态
            game_state.setState(SETTLE);
            game_state.setEvents(events);

            // 添加所有玩家信息
            for (Player* currentPlayer : players) {
                if (currentPlayer) {
                    game_state.appendPlayer(currentPlayer);
                }
            }

            // 发送最终游戏状态
            sendGameState(game_state);
            game_state.clearEvents();

            // 发送信号通知 GameRoom
            emit gameReturningToRoom();

        } catch (const std::exception& e) {
            qDebug() << "Error announcing winner:" << e.what();
        }

    } catch (const std::exception& e) {
        qDebug() << "Critical error in announceWinner:" << e.what();
    } catch (...) {
        qDebug() << "Unknown error in announceWinner";
    }
}

void ServerGame::returnToRoom() {
    try {
        qDebug() << "\n=== Returning to Room ===";

        // 重置游戏状态
        deck.clear();
        pointedCards.clear();
        current_player = nullptr;
        pointed_count = 0;
        receivedActions.clear();
        playerPointedStatus.clear();
        playerMysteryPoints.clear();

        // 重置所有玩家状态
        for (auto player : players) {
            player->getHandCards().clear();
            player->getPlayedCards().clear();
            player->getDiscardedCards().clear();
            player->setCanNormalPlay(true);
            player->setWin(false);
        }

        // 设置状态为初始状态
        setState(INIT);

        // 发送返回房间通知
        QJsonObject returnNotification;
        returnNotification.insert("type", "return_to_room");
        returnNotification.insert("message", "Game has ended. Returned to room.");

        // 广播给所有玩家
        for (QTcpSocket* socket : playerSockets) {
            if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                socket->write(QJsonDocument(returnNotification).toJson(QJsonDocument::Compact) + "\n");
                socket->flush();
            }
        }

        // 发送最终的游戏状态
        sendGameState(game_state);
        game_state.clearEvents();

        qDebug() << "Successfully returned to room";
    } catch (const std::exception& e) {
        qDebug() << "Error returning to room:" << e.what();
    }
}

void ServerGame::onPlayerActionReceived(const PlayerAction& action) {
    processPlayerAction(action);
}

ServerGame::ServerGame(QObject* parent) : QObject(parent), current_player(nullptr), pointed_count(0) {
    // 初始化其他成员变量
    deck.clear();
    pointedCards.clear();
    players.clear();
    playerSockets.clear();
    receivedActions.clear();
}

void ServerGame::handleRestart() {
    // 重置戏状态
    deck.clear();
    pointedCards.clear();
    current_player = nullptr;
    pointed_count = 0;
    receivedActions.clear();

    // 重置玩家状态
    for(auto player : players) {
        player->getHandCards().clear();
        player->getPlayedCards().clear();
        player->getDiscardedCards().clear();
    }

    // 重新开始游戏
    game_state.setState(SEND_CARD);
    startGameLogic();
}

// 辅助函数：获取卡牌类型的字符串表示
QString ServerGame::getCardTypeName(CardType type) {
    switch(type) {
    case CardType::ANSWER: return "Answer";
    case CardType::SOLVE: return "Solve";
    case CardType::READ: return "Read";
    case CardType::THINK: return "Think";
    case CardType::INDUCE: return "Induce";
    case CardType::TRAP: return "Trap";
    case CardType::TRADE: return "Trade";
    case CardType::STEAL: return "Steal";
    case CardType::REVERSE: return "Reverse";
    case CardType::HIDE: return "Hide";
    case CardType::COVER: return "Cover";
    case CardType::FIRST: return "First";
    default: return "Unknown";
    }
}

// 保留一个空的实现，因为我们现在直接使用 dealCards()
void ServerGame::handleDealCard() {
    // 空实现，我们现在在 startGameLogic() 中直接调 dealCards()
    qDebug() << "handleDealCard() is deprecated, using dealCards() directly";
}

void ServerGame::processPlayerDiscard(Player* player, int cardId, int count) {
    try {
        qDebug() << "\n=== Processing Discard Request ===";
        qDebug() << "Initial hand state:";
        for (const auto& card : player->getHandCards()) {
            qDebug() << "- Card:" << getCardTypeName(card->getType());
        }

        // 检查是否试图弃掉谜底或匿牌或先手牌
        if (cardId == ANSWER_ID || cardId == HIDE_ID || cardId == FIRST_ID) {
            QJsonObject response;
            response.insert("type", "discard_failed");
            response.insert("message", "Cannot discard Answer, Hide or First cards");
            QTcpSocket* socket = findPlayerSocket(player);
            if (socket) {
                socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
                socket->flush();
            }
            return;
        }

        // 检查弃牌后是否会少于6张
        int currentHandSize = player->getHandCards().size();
        if (currentHandSize - count < 6) {
            QJsonObject response;
            response.insert("type", "discard_failed");
            response.insert("message", "Cannot discard so many cards. You must keep exactly 6 cards.");
            QTcpSocket* socket = findPlayerSocket(player);
            if (socket) {
                socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
                socket->flush();
            }
            return;
        }

        // 记录弃牌前的手牌状态
        QMap<CardType, int> initialHandStats;
        for (const auto& card : player->getHandCards()) {
            initialHandStats[card->getType()]++;
        }

        // 执行弃牌
        int discarded = 0;
        auto& handCards = player->getHandCards();
        std::vector<card_pointer> cardsToDiscard;

        // 先找到所有要弃掉的牌
        for (auto it = handCards.begin(); it != handCards.end() && discarded < count; ++it) {
            if (getCardID((*it)->getType()) == cardId) {
                cardsToDiscard.push_back(*it);
                discarded++;
            }
        }

        // 然后一次性弃掉这些牌
        for (const auto& card : cardsToDiscard) {
            player->discardCard(card);
        }

        // 统计弃牌后的手牌状态
        QMap<CardType, int> finalHandStats;
        int totalRemaining = 0;
        for (const auto& card : player->getHandCards()) {
            finalHandStats[card->getType()]++;
            totalRemaining++;
        }

        // 计算本次弃掉牌的统计
        QJsonObject discardedStats;
        int totalDiscarded = 0;
        for (auto it = initialHandStats.begin(); it != initialHandStats.end(); ++it) {
            int initialCount = it.value();
            int finalCount = finalHandStats.value(it.key(), 0);
            if (initialCount > finalCount) {
                discardedStats[QString::number(static_cast<int>(it.key()))] = initialCount - finalCount;
                totalDiscarded += initialCount - finalCount;
            }
        }

        // 创建响应
        QJsonObject response;
        response.insert("type", "discard_result");
        response.insert("player_id", player->getPlayerID());
        response.insert("total_discarded", totalDiscarded);
        response.insert("discarded_cards", discardedStats);
        response.insert("total_remaining", totalRemaining);

        QJsonObject remainingStats;
        for (auto it = finalHandStats.begin(); it != finalHandStats.end(); ++it) {
            remainingStats[QString::number(static_cast<int>(it.key()))] = it.value();
        }
        response.insert("remaining_cards", remainingStats);
        response.insert("phase_complete", totalRemaining == 6);  // 修改为必须等于6

        // 在玩家手动弃牌到6张时发送的消息
        if (player->getHandCards().size() == 6) {
            QJsonObject response;
            response.insert("type", "discard_result");
            response.insert("player_id", player->getPlayerID());
            response.insert("total_discarded", totalDiscarded);
            response.insert("discarded_cards", discardedStats);
            response.insert("total_remaining", totalRemaining);
            response.insert("remaining_cards", remainingStats);
            response.insert("phase_complete", true);
            response.insert("is_final_result", false);  // 手动弃牌到6张时设为 false

            QTcpSocket* socket = findPlayerSocket(player);
            if (socket) {
                socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
            }
        }

        // 发送响应
        QTcpSocket* socket = findPlayerSocket(player);
        if (socket) {
            socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
            socket->flush();
        }

    } catch (const std::exception& e) {
        qDebug() << "Error processing discard:" << e.what();
    }
}

// 添加 getCardID 函数实现
int ServerGame::getCardID(CardType type) {
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

// 添加新函数用于发送玩家手牌信息
void ServerGame::sendHandCardsInfo(Player* player) {
    QJsonObject response;
    response.insert("type", "hand_cards_info");
    response.insert("player_id", player->getPlayerID());

    // 统计手牌
    QJsonObject handCardsStats;
    int totalCards = 0;
    for (const auto& card : player->getHandCards()) {
        CardType type = card->getType();
        handCardsStats[QString::number(static_cast<int>(type))] =
            handCardsStats[QString::number(static_cast<int>(type))].toInt() + 1;
        totalCards++;
    }

    // 添加统计信息
    response.insert("total_cards", totalCards);
    response.insert("hand_cards", handCardsStats);

    // 发送响应
    QTcpSocket* socket = findPlayerSocket(player);
    if (socket) {
        qDebug() << "Sending hand cards info to player" << player->getPlayerID();
        qDebug() << "Total cards:" << totalCards;
        socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
        socket->flush();
    }
}
