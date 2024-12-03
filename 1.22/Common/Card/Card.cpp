#include "Card.h"
#include "../GameState/GameState.h"
#include "../Player/Player.h"
#include <QString>
#include <QJsonObject>
#include <memory>

using namespace std;
using card_pointer = std::shared_ptr<Card>;
//Card::setFaceUp
/**
 * @brief 设置卡牌的 明、暗置情况。
 *
 * 玩家出牌时，可以选择将牌明置或暗置打出。因此，每张打出的牌要么是暗置（即面朝下）、要么是明置（即面朝上）。
 * 将牌暗置打出，玩家的"谜"数+1；
 * 将牌明置打出，每种牌会发挥不一样的效果。
 * 函数用途：将牌从手牌中打出去时根据玩家选择将牌设为明或暗。
 * @param faceUp  布尔类型变量，表示卡牌明暗情况。true时为明置，false时为暗置。
 */
void Card::setFaceUp(bool faceUp){
    face_up=faceUp;
}

//Card::getFaceUp
/**
 * @brief  获取卡牌的 明、暗 置情况。
 * @return 返回成员函数   face_up。
 * 函数用途：让 其他对象访问卡牌的 明暗情况。
 */
bool Card::getFaceUp(){
    return face_up;
}

//Card::getType
/**
 * @brief 获取卡牌类型
 * @return 返回枚举类成员变量  card_type。该枚举类对象表示12 种不同类型的卡牌，每种对应不同的效果。 具体见Card.h。
 */
CardType Card::getType(){
    return card_type;
}

//Card::getMysteryPoints
/**
 * @brief 获取每张牌的"谜"点数。
 *   暗置情况下，所有卡牌的" 谜"数为1； 明置情况下，大多数 卡牌的" 谜"数为0；
 *  个别卡牌明置 时例外。如：" 陷"牌 明置后"谜" 数为3；" 匿"牌在   亮匿阶段被系统掀开后" 谜"数为 无穷大。
 * @return 返回整型成员变量，"谜"的 点数。
 */
int Card::getMysteryPoints(){
    return mystery_point;
}

//Card::setMysteryPoints
/**
 * @brief 设置每张牌的"谜"点数
 * @param points  整型变量，表示"谜"的点数。
 */
void Card::setMysteryPoints(int points){
    mystery_point=points;
}

//Card::setLastFaceDown
/**
 * @brief 设置某张卡牌是否是该玩家 出牌堆中的最后一张暗置牌
 * 函数用途："读"牌的效果是，明置打出后，选择其他玩家一张最晚打出的暗置牌， 自己查看 且不发挥 该牌效果。因此，在玩家
 * 打出" 读"后选择其他  暗置牌时，调用该函数检查玩家选择的牌是否合法。
 * @param lastFaceDown   布尔类型成员变量，表示某牌是否是最晚打出的暗置牌。
 */
void Card::setLastFaceDown(bool lastFaceDown){
    is_last_face_down=lastFaceDown;
}

//Card::getLastFaceDown
/**
 * @brief 获取某张牌是否是最晚打出的暗置牌
 * @return   布尔类型成员变量is_last_face_down。
 */
bool Card::getLastFaceDown(){
    return is_last_face_down;
}

void Card::setHasTrigger(bool hasTrigger){
    has_trigger_effect=hasTrigger;
}

bool Card::getHasTrigger(){
    return has_trigger_effect;
}

//Card::setValidPlay
/**
 * @brief 设置 一张牌是否 已经合法打出
 * 函数用途：卡牌的play函数会检测玩家的选择合不合法；若不合法，则会 先将false传入该函数，表明这张牌 并未合法打出， 接着直接 return，
 * 结束函数。在游戏流程中，玩家不断选择卡牌打出，直到被play判定为合法， 则将true传入该函数。 游戏内客户端ClientGame检测到true，才
 * 会将数据上传给服务器。
 * @param validPlay 布尔类型变量，表示是否合法打出。
 */
void Card::setValidPlay(bool validPlay){
    valid_play=validPlay;
}

//Card::getValidPlay
/**
 * @brief 获取某张牌是否已经合法打出
 * 函数用途：游戏内客户端ClientGame通过函数访问 valid_play，决定是否将数据上传。
 * @return  布尔类成员变量valid_play
 */
bool Card::getValidPlay(){
    return valid_play;
}

//Card::toJson
/**
 * @brief 序列化卡牌为json格式
 * 当玩家合法 出牌后，客户端会创建一个 PlayerAction对象（具体见PlayerAction的头文件和源文件），用以打包数据并序列化上传。
 * 数据包含卡牌类型对象（即自己打出的牌，有时也会包含选择的其他玩家的牌），因此在卡牌类中需要 单独写一个 toJson函数用以将卡
 * 牌对象序列化。
 * 玩家类 序列化时，会将自己的手牌，出牌和弃牌堆中所有的牌序列化。
 * 函数用途：1.PlayerAction序列化时，调用该函数将里面的卡牌类序列化。
 *         2. Player  序列化时，调用该函数将自己的手牌、出牌和公共弃牌堆序列化。
 * @return
 */
QJsonObject Card::toJson() {
    QJsonObject json;
    json["card_type"] = static_cast<int>(card_type);
    json["effect"] = effect;
    json["face_up"] = face_up;
    json["has_trigger_effect"] = has_trigger_effect;
    json["is_last_face_down"] = is_last_face_down;
    json["valid_play"] = valid_play;
    json["mystery_point"] = mystery_point;
    return json;
}

//Card::fromJson
/**
 * @brief 反序列化卡牌对象。
 * 函数用途：1.反序列化PlayerAction 时，取出封装卡牌数据的json对象，调用该函数还原 card_pointer对象，包括玩家打出/弃置/指认的牌， 也可能包括
 *           玩家选择的其他玩家的牌。
 *         2.反序列化 Player 时，调用该函数还原 card_pointer对象，包括该玩家的所有手牌、出牌以及共同的弃牌堆。
 * @param json 传入的序列化对象，存储着卡牌数据。
 * @return 卡牌智能指针类型对象。
 */
std::shared_ptr<Card> Card::fromJson(const QJsonObject& json) {
    auto card = std::make_shared<Card>(
        static_cast<CardType>(json.value("card_type").toInt()),
        json.value("effect").toString()
    );
    card->setFaceUp(json.value("face_up").toBool());
    card->setHasTrigger(json.value("has_trigger_effect").toBool());
    card->setLastFaceDown(json.value("is_last_face_down").toBool());
    card->setValidPlay(json.value("valid_play").toBool());
    card->setMysteryPoints(json.value("mystery_point").toInt());

    return card;
}

void Card::play(Player* player_pointer, GameState& gameState, bool face_up) {
    // 默认实现
}

void Card::trigger(Player* player_pointer, GameState& gameState, bool face_up) {
    // 默认实现
}

void Card::showCard(Player* player) {
    // 实现显示卡牌逻辑
}

