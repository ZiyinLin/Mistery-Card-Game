#include "ClientGame.h"
#include "../../../Common/Card/Card.h"
#include "../../../Common/GameState/GameState.h"
#include "../../../Common/PlayerAction/PlayerAction.h"
#include "../../../Common/Player/Player.h"
#include "../../../Common/GameEvent/GameEvent.h"
#include <QString>
#include <QThread>
#include <QDebug>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <QJsonDocument>
#include <QJsonObject>

using namespace std;
using card_pointer = std::shared_ptr<Card>;


ClientGame::ClientGame(Player* player)
    : m_player(player) {
    my_game_state = new GameState(this);
    ui = new UI(this);
    qDebug()<<"ClientGame built for the player.";
    connect(this, &ClientGame::updateMessage, ui, &UI::setGameMessage);
    connect(this, &ClientGame::enableCardSelection, ui, &UI::toggleHandCardSelection);
    connect(this, &ClientGame::enableTableCardSelection, ui, &UI::toggleTableCardSelection);
    this->startGame();
}
//ClientGame::sendPlayerAction
/**
 * @brief 将玩家动作上传至服务器
 * @param action 玩家动作类对象，记录本次操作对应的玩家、卡牌、玩家行为。
 */
void ClientGame::sendPlayerAction(PlayerAction& action) {
    qDebug()<<"ClientGame:sending a player action.";
    QJsonObject json = action.toJson();
    emit sendPlayerAction(json);
}

//ClientGame::getState
/**
 * @brief 获取当前游戏阶段
 * 游戏分为弃牌、出牌和指认三个阶段，在代码中 将其细分为 初始化、 发牌、弃牌、出牌、亮匿、 指认、结算等多个阶段放入枚举类State中。
 * @return   gameState中的私有State类对象
 */
State ClientGame::getState(){
    return my_game_state->getState();
}

//ClientGame::getPlayer
/**
 * @brief 获取该游戏内客户端对应的玩家
 * 函数用途：服务器调用该函数访问某个客户端对应的玩家
 * @return 玩家指针类对象
 */
Player* ClientGame::getPlayer(){
    return m_player;
}

//
QString ClientGame::getMyId(){
    return m_player->getPlayerID();
}

void ClientGame::startGame(){
    State current_state = my_game_state->getState();
    switch(current_state){
    case(DISCARD):
        handleDiscard();
        break;
    case(PLAY):
        handlePlay();
        break;
    case(POINT_OUT):
        handlePointOut();
    default:
        break;
    }
}

void ClientGame::handleDiscard() {
    if (actionCompleted) {
        qDebug()<<"ClientGame:action completed. Return.";
        return;
    }
    qDebug()<<"ClientGame: now discard cards.";
    if (m_player->getHandCards().size() > 6) {
        ui->displayHandCards(m_player->getHandCards());
        ui->displayPrompt("请输入序号弃置卡牌(1-" + QString::number(m_player->getHandCards().size()) + "):");

        emit enableCardSelection(true);
        connect(this, &ClientGame::cardSelected, this, [=](std::shared_ptr<Card> card) {
            discardCard(card);
            qDebug()<<"ClientGame: you have discarded a card";
            if (m_player->getHandCards().size() <= 6) {
                emit enableCardSelection(false);
                actionCompleted = true;
            } else {
                ui->displayHandCards(m_player->getHandCards());
                ui->displayPrompt("请输入序号弃置卡牌(1-" + QString::number(m_player->getHandCards().size()) + "):");
            }
        });
    } else {
        qDebug()<<"ClientGame: finish discarding cards.";
        actionCompleted = true;
    }
}

void ClientGame::handlePlay() {
    if (current_player_id != getMyId() || actionCompleted) {
        return;
    }

    for (auto player : my_game_state->getAllPlayers()) {
        ui->displayTableCards(player->getPlayerID(), player->getPlayedCards());
    }

    ui->displayHandCards(m_player->getHandCards());
    ui->displayPrompt("请输入序号出牌(1-" + QString::number(m_player->getHandCards().size()) + "):");

    emit enableCardSelection(true);
    connect(this, &ClientGame::cardSelected, this, [=](std::shared_ptr<Card> card) {
        ui->displayFaceUpChoice();
        playCard(card);
        emit enableCardSelection(false);
        actionCompleted = true;
    });
}

void ClientGame::handlePointOut() {
    if (actionCompleted) {
        return;
    }

    for (auto player : my_game_state->getAllPlayers()) {
        if (player->getPlayerID() != getMyId()) {
            ui->displayTableCards(player->getPlayerID(), player->getPlayedCards());
        }
    }
    ui->displayPrompt("请输入要指认的玩家ID和牌的序号(格式: playerID cardNumber):");

    emit enableTableCardSelection(true);
    connect(this, &ClientGame::tableCardSelected, this, [=](std::shared_ptr<Card> card) {
        pointOutCard(card);
        emit enableTableCardSelection(false);
        actionCompleted = true;
    });
}

void ClientGame::discardCard(card_pointer card){
    PlayerAction action;
    action.setActionType(DISCARD_CARD);
    action.setPlayerID(getMyId());
    action.setCard(card);
    sendPlayerAction(action);
}

void ClientGame::playCard(card_pointer card){
    PlayerAction action;
    action.setActionType(PLAY_CARD);
    action.setPlayerID(getMyId());
    action.setCard(card);
    sendPlayerAction(action);
}

void ClientGame::selectTargetCard(card_pointer targetCard){
    PlayerAction action;
    action.setActionType(SELECT_ORIENTED_CARD);
    action.setPlayerID(getMyId());
    action.setTargetCard(targetCard);
    sendPlayerAction(action);
}

void ClientGame::selectTargetPlayer(QString targetPlayerID){
    PlayerAction action;
    action.setActionType(SELECT_ORIENTED_PLAYER);
    action.setPlayerID(getMyId());
    action.setTargetPlayerID(targetPlayerID);
    sendPlayerAction(action);
}

void ClientGame::pointOutCard(card_pointer targetCard){
    PlayerAction action;
    action.setActionType(POINT_OUT_CARD);
    action.setPlayerID(getMyId());
    action.setTargetCard(targetCard);
    sendPlayerAction(action);
}

void ClientGame::onGameStateReceived(GameState& gameState){
    qDebug()<<"ClientGame:A game_state received, processing it...";
    processGameState(gameState);
}

//GameState::processGameState
/**
 * @brief 处理GameState
 * 函数首先将传入的Json对象转化为GameState类对象，并让此对象成为自己的 私有成员变量my_game_state；接着 取出event队列中的数据，根据 event_type
 * 执行相对应的私有函数。
 * @param json 封装着GameState数据的Json类对象
 */
void ClientGame::processGameState(GameState& gameState) {
    // 不要尝试复制 GameState 对象，而是直接使用它
    if(my_game_state) {
        delete my_game_state;
    }

    my_game_state = GameState::fromJson(gameState.toJson());

    // 获取事件队列
    QQueue<GameEvent> events = gameState.getEvents();

    // 处理事件队列中的每一个事件
    while (!events.isEmpty()) {
        GameEvent event = events.dequeue();

        // 获取事件类型
        QString eventType = event.getEventType();
        QJsonObject eventData = event.getEventData();

        // 解析玩家信息
        QJsonValue playerValue = eventData["player"];
        if (!playerValue.isObject()) {
            qWarning() << "Error: 'player' is not a JSON object in event:" << eventType;
            continue;  // 跳过该事件
        }
        QJsonObject playerJson = playerValue.toObject();
        Player* player = Player::fromJson(playerJson);
        if (!player) {
            qWarning() << "Error: Failed to deserialize Player object in event:" << eventType;
            continue;  // 过该事件
        }

        // 解析牌信息
        QJsonValue cardValue = eventData["card"];
        if (!cardValue.isObject()) {
            qWarning() << "Error: 'card' is not a JSON object in event:" << eventType;
            continue;  // 跳过该事件
        }
        QJsonObject cardJson = cardValue.toObject();
        card_pointer card = Card::fromJson(cardJson);
        if (!card) {
            qWarning() << "Error: Failed to deserialize Card object in event:" << eventType;
            continue;  // 跳过该事件
        }

        // 根据事件类型执行相应操作
        if (eventType == "AddHandCard") {
            try {
                qDebug()<<"ClientGame:Adding a hand card.";
                this->addHandCardUI(player, card);
            } catch (const std::exception& e) {
                qCritical() << "Exception during AddHandCard:" << e.what();
            }
        } else if (eventType == "DiscardCard") {
            try {
                this->discardCardUI(player, card);
            } catch (const std::exception& e) {
                qCritical() << "Exception during DiscardCard:" << e.what();
            }
        } else if (eventType == "PlayCard") {
            try {
                this->playCardUI(player, card);
            } catch (const std::exception& e) {
                qCritical() << "Exception during PlayCard:" << e.what();
            }
        } else if (eventType == "ReadCard") {
            if (m_player == player) {
                try {
                    this->readCardUI(player, card);
                } catch (const std::exception& e) {
                    qCritical() << "Exception during ReadCard:" << e.what();
                }
            }
        } else if (eventType == "TakeBackCard") {
            try {
                this->takeBackCardUI(player, card);
            } catch (const std::exception& e) {
                qCritical() << "Exception during TakeBackCard:" << e.what();
            }
        }else if(eventType=="CurrentPlayer"){
            current_player_id = player->getPlayerID();
        } else if(eventType=="AnnounceWinner"){
            try{
                this->displayWinnerUI(player->getPlayerID());
            }catch(const std::exception& e){
                qCritical()<<"Exception during displaying winner:"<<e.what();
            }
        }else {
            // 未知事件类型的处理
            qWarning() << "Unhandled event type:" << eventType;
        }
    }
}

void ClientGame::onCardNumberSelected(int number) {
    if (number < 1 || number > m_player->getHandCards().size()) {
        ui->displayPrompt("无效的卡牌序号!");
        return;
    }

    auto card = m_player->getHandCards()[number - 1];
    emit cardSelected(card);
}

void ClientGame::onFaceUpSelected(bool faceUp) {
    // 处理明暗置选择
    // ...
}

//ClientGame::flipCard
/**
 * @brief  掀牌动作的实现
 * 函数用途：当某暗置卡牌被打出“解”的玩家选中时，调用该函数展示将其掀开的动画。
 * @param player 玩家指针类型对象，表示形参card的持有者。
 * @param card 卡牌智能指针类型对象，表示被选中的暗置牌。
 */
void ClientGame::flipCardUI(Player* player, std::shared_ptr<Card> card){
    //需要UI
}

//ClientGame::playCard
/**
 * @brief  出牌动作的实现
 * 函数用途：当某张牌被手牌中打出时，调用该函数展示将其打出的动画。
 * @param player 玩家指针类型对象，表示形参card的持有者。
 * @param card 卡牌智能指针类型对象，表示打出的牌。
 */
void ClientGame::playCardUI(Player* player, std::shared_ptr<Card> card){

}

//ClientGame::readCard
/**
 * @brief  读牌动作的实现
 * 函数用途：当某玩家明置打出“读”选中了场上其他的暗置牌后，调用该函数将该牌展示给此玩家看
 * @param player 玩家指针类型对象，表示明置打出“读”的玩家。
 * @param card 卡牌智能指针类型对象，表示被选中的暗置牌。
 */
void ClientGame::readCardUI(Player* player, std::shared_ptr<Card> card){

}

//ClientGame::discardCard
/**
 * @brief 弃牌动作的实现
 * 函数用途：玩家弃置某张牌后，调用该函数展示弃牌的动画
 * @param player 玩家指针类型对象，表示弃牌的玩家
 * @param card 卡牌智能指针类型对象，表示弃置的卡牌。
 */
void ClientGame::discardCardUI(Player* player, std::shared_ptr<Card> card){

}

//ClientGame::addHandCard
/**
 * @brief  将牌加入手牌的动画
 * 函数用途：当有卡牌加入玩家手牌时（如发牌或某些牌执行效果），调用该函数。
 * @param player 玩家指针类型对象，表示 形参 card的持有者。
 * @param card 卡牌智能指针类型对象，表示加入手牌的卡牌。
 */
void ClientGame::addHandCardUI(Player* player, std::shared_ptr<Card> card){

}

//ClientGame::takeBackCard
/**
 * @brief 将牌从出牌堆中收回的动画
 * 函数用途：当有玩家从出牌堆中收回牌 至手牌中时（例如，有人明置打出“溯”，所有玩家回收最近打出的一张牌；明置打出“幌”后将出牌堆中所有暗置牌收回），
 * 调用该函数展示收回的动画
 * @param player 玩家指针类型对象，表示形参 card的持有者。
 * @param card 卡牌智能指针类型对象，表示收回的卡牌。
 */
void ClientGame::takeBackCardUI(Player* player, std::shared_ptr<Card> card){

}

//ClientGame::disPlayWinner
/**
 * @brief  展示胜利者
 * 函数用途：当进入结算阶段时，服务器会传回获胜玩家对应的玩家ID，客户端将其传入该函数，展示获胜玩家。
 * @param playerID 获胜玩家ID
 */
void ClientGame::displayWinnerUI(const QString playerID){

}

ClientGame::~ClientGame() {
    delete my_game_state;
    delete ui;
}


