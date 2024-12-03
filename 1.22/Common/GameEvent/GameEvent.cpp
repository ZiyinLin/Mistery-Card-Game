#include "GameEvent.h"

// 在文件开头注册元类型
static int gameEventTypeId = qRegisterMetaType<GameEvent>("GameEvent");
//GameEvent::setEvent
/**
 * @brief 设置单次游戏事件。
 * 函数用途：1.服务器在接收PlayerAction后，根据其记录的玩家动作把不同的event写入GameEvent中，最后封装进GameState上传。
 *         2.服务器执行发牌、亮匿、翻指认牌等自己完成的动作后，将event写入并封装上传。
 * @param event 游戏事件。目前包括：
 * AddHandCard(添加手牌)，DiscardCard（弃牌），PlayCard（出牌），ReadCard（读牌），FlipCard（掀牌），TakeBackCard（拿回牌），
 * AnnounceWinner(宣布赢家)
 */
void GameEvent::setEventType(QString type) {
    event_type = type;
}
//GameEvent::getEvent
/**
 * @brief 获取单次游戏事件
 * 函数用途：游戏内客户端ClientGame接收gameState后访问event，据此执行相对应的私有函数。
 * @return 字符串成员变量event_type
 */
QString GameEvent::getEventType() const {
    return event_type;
}
//GameEvent::setEventData
/**
 * @brief 传入游戏事件的数据
 * 函数用途：ServerGame接收到PlayerAction后创建GameEvent列表，接着根据PlayerAction中的数据调用该函数将对应的json格式数据
 * 写入单个event中，并将event加入列表，最后把列表封装进GameState中打包上传至ClientGame
 * @param json QJsonObject类成员变量event_data
 */
void GameEvent::setEventData(const QJsonObject& json) {
    event_data = json;
}
//GameEvent::getEventData
/**
 * @brief 获取游戏事件的数据
 * 数据为PlayerAction传上来的，包括player，card，action，（targetplayer），（targetcard）
 * 函数用途：ClientGame接收gameState后访问event_data，将其中的对象作为形参传入到要执行的私有函数中。
 * @return QJsonObject类成员变量event_data
 * event_data封装两类对象：玩家指针类Player*和卡牌智能指针类card_pointer
 * 这两者在不同的event_type下分别对应的是：
 * AddHandCard--添加手牌的玩家和被添加的卡牌
 * DiscardCard--弃牌的玩家和被弃置的卡牌
 * PlayCard--出牌的玩家和打出的牌
 * ReadCard--读牌的玩家和被读的牌
 * FlipCard--翻牌的玩家和被翻的牌
 * TakeBackCard--收回牌的玩家和被收回的牌
 * AnnounceWinner--获胜的玩家
 */
QJsonObject GameEvent::getEventData() const {
    return event_data;
}
