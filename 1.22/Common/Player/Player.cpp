#include "../Card/Card.h"
#include "Player.h"
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <vector>
#include <algorithm>
using card_pointer = std::shared_ptr<Card>;
using namespace std;

// 定义静态成员变量
std::vector<std::shared_ptr<Card>> Player::discarded_cards;

void Player::drawCard(int card) {
    // TODO: 实现抽牌逻辑
    Q_UNUSED(card);
}

void Player::playCard(card_pointer card, GameState& gameState, bool faceUp) {
    // 从手牌中移除
    auto it = std::find(hand_cards.begin(), hand_cards.end(), card);
    if (it != hand_cards.end()) {
        hand_cards.erase(it);
        // 确保添加到出牌堆
        played_cards.push_back(card);  // 这行是否存在？
    }

    // 设置牌的状态
    card->setFaceUp(faceUp);
    // 执行卡牌效果
    card->play(this, gameState, faceUp);
}

void Player::debugCardStatus() {
    qDebug() << "Player" << player_id << "status:";
    qDebug() << "Hand cards:" << hand_cards.size();
    qDebug() << "Played cards:" << played_cards.size();
    qDebug() << "Played cards details:";
    for (const auto& card : played_cards) {
        qDebug() << "- Type:" << card->getCardTypeName()
        << "FaceUp:" << card->getFaceUp();
    }
}
void Player::discardCard(std::shared_ptr<Card> card) {
    if (!card) return;
    this->removeHandCard(card);
    this->addDiscardedCard(card);
}

void Player::addHandCard(std::shared_ptr<Card> card) {
    if (!card) return;
    hand_cards.push_back(card);
}

void Player::removeHandCard(std::shared_ptr<Card> card) {
    if (!card) return;
    auto it = std::find(hand_cards.begin(), hand_cards.end(), card);
    if (it != hand_cards.end()) {
        hand_cards.erase(it);
    }
}

void Player::addPlayedCard(std::shared_ptr<Card> card) {
    if (!card) return;
    played_cards.push_back(card);
}

void Player::addPlayedCardAt(std::shared_ptr<Card> card, int index) {
    if (!card || index < 0 || index > static_cast<int>(played_cards.size())) return;
    played_cards.insert(played_cards.begin() + index, card);
}

void Player::removePlayedCard(std::shared_ptr<Card> card) {
    if (!card) return;
    auto it = std::find(played_cards.begin(), played_cards.end(), card);
    if (it != played_cards.end()) {
        played_cards.erase(it);
    }
}

void Player::addDiscardedCard(std::shared_ptr<Card> card) {
    if (!card) return;
    discarded_cards.push_back(card);
}

void Player::removeDiscardedCard(std::shared_ptr<Card> card) {
    if (!card) return;
    auto it = std::find(discarded_cards.begin(), discarded_cards.end(), card);
    if (it != discarded_cards.end()) {
        discarded_cards.erase(it);
    }
}

std::vector<Player::card_pointer>& Player::getHandCards() {
    return hand_cards;
}

const std::vector<Player::card_pointer>& Player::getHandCards() const {
    return hand_cards;
}

std::vector<std::shared_ptr<Card>> Player::getPlayedCards() {
    return played_cards;
}

std::vector<std::shared_ptr<Card>> Player::getDiscardedCards() {
    return discarded_cards;
}

void Player::setCanNormalPlay(bool canNormal) {
    can_normal_play = canNormal;
}

bool Player::getCanNormalPlay() {
    return can_normal_play;
}

void Player::setWin(bool win) {
    is_win = win;
}

bool Player::getWin() {
    return is_win;
}

void Player::setConnected(bool isConnected) {
    is_connected = isConnected;
}

bool Player::getConnected() {
    return is_connected;
}

int Player::getMysteries() {
    int count = 0;
    qDebug() << "\n=== Calculating Mystery Points for" << player_id << "===";
    qDebug() << "Total played cards:" << played_cards.size();

    for (const auto& card : played_cards) {
        qDebug() << "Checking card:" << card->getCardTypeName();
        qDebug() << "Face up status:" << card->getFaceUp();

        if (!card->getFaceUp()&&card->getType()!=HIDE) {  // 如果卡牌是暗置的
            count++;
            qDebug() << "Card is face-down, adding to count";
        }else if(!card->getFaceUp()&&card->getType()==HIDE){
            count+=100;
            qDebug()<<"Hide card hasn't been flipped, get most points.";
        }else if(card->getFaceUp()&&card->getType()==TRAP){
            count+=3;
        }
    }

    qDebug() << "Final mystery points:" << count;
    return count;
}

bool Player::chooseFaceUp(GameState& gameState) {
    // 实现选择明暗置的逻辑
    return false; // 临时返回
}

std::shared_ptr<Card> Player::chooseHandCard(GameState& gameState) {
    // 实现...
    return nullptr; // 临时返回
}

std::shared_ptr<Card> Player::chooseTargetCard(GameState& gameState) {
    // 实现...
    return nullptr; // 临时返回
}

QString Player::chooseTargetPlayerID(GameState& gameState) {
    // 实现选择目标玩家的逻辑
    return QString(); // 临时返回
}

//QJsonObject Player::toJson
/**
 * @brief 序列化 Player
 * 服务器向客户端上传 gameState，其中需要将场上所有玩家的实时数据封装，因此需要在玩家类里实现Player的序列化。
 * 函数用途：gameState序列化时，在序列化自己的 Player列表时调用该函数，将每位玩家的数据封装。
 * @return QJsonObject类对象，记录着玩家实时数据
 */
QJsonObject Player::toJson() {
    QJsonObject json;
    json["player_id"] = player_id;
    json["player_name"] = player_nickname;
    json["is_connected"] = is_connected;
    json["can_normal_play"] = can_normal_play;
    json["mystery_points"] = this->getMysteries();
    QJsonArray handCardArray;
    auto handCards = this->getHandCards();
    for (auto card : handCards) {
        handCardArray.append(card->toJson());
    }
    json["hand_cards"] = handCardArray;
    QJsonArray playedCardArray;
    auto playedCards = this->getPlayedCards();
    for (auto card : playedCards) {
        playedCardArray.append(card->toJson());
    }
    json["played_cards"] = playedCardArray;
    QJsonArray discardedCardArray;
    auto discardedCards = this->getDiscardedCards();
    for (auto card : discardedCards) {
        discardedCardArray.append(card->toJson());
    }
    json["discarded_cards"] = discardedCardArray;
    return json;
}

//Player::fromJson
/**
 * @brief 反序列化Player
 * 函数用途：GameState 反序列化时，当反序列化到封装 Player数据的json类 时，调用该函数还原玩家指针类对象。
 * @param json 接收的QJsonObject类对象，封装着玩家的数据。
 * @return 玩家指针类对象。
 */
Player* Player::fromJson(const QJsonObject& json) {
    QString id = json["player_id"].toString();
    QString nickname = json["player_name"].toString();
    Player* player = new Player(id, nickname);
    player->setConnected(json["is_connected"].toBool());
    player->setCanNormalPlay(json["can_normal_play"].toBool());
    QJsonArray handCardArray = json["hand_cards"].toArray();
    for (const QJsonValue& cardValue : handCardArray) {
        QJsonObject cardJson = cardValue.toObject();
        auto card = Card::fromJson(cardJson);
        player->addHandCard(card);
    }
    QJsonArray playedCardArray = json["played_cards"].toArray();
    for (const QJsonValue& cardValue : playedCardArray) {
        QJsonObject cardJson = cardValue.toObject();
        auto card = Card::fromJson(cardJson);
        player->addPlayedCard(card);
    }
    QJsonArray discardedCardArray = json["discarded_cards"].toArray();
    for (const QJsonValue& cardValue : discardedCardArray) {
        QJsonObject cardJson = cardValue.toObject();
        auto card = Card::fromJson(cardJson);
        player->addDiscardedCard(card);
    }

    return player;  // 返回反序列化后的 Player 指针
}

QString& Player::getPlayerID() {
    return player_id;
}

QString& Player::getNickname() {
    return player_nickname;
}
