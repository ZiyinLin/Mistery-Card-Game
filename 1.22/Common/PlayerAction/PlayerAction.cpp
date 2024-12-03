#include "PlayerAction.h"
#include "../Card/Card.h"
#include <QJsonDocument>
#include <QString>
#include <QJsonObject>
// 添加类型别名
using card_pointer = std::shared_ptr<Card>;
// 添加 getCardTypeName 函数
QString getCardTypeName(CardType type) {
    switch(type) {
        case CardType::ANSWER: return "ANSWER";
        case CardType::SOLVE: return "SOLVE";
        case CardType::READ: return "READ";
        case CardType::THINK: return "THINK";
        case CardType::INDUCE: return "INDUCE";
        case CardType::TRAP: return "TRAP";
        case CardType::TRADE: return "TRADE";
        case CardType::STEAL: return "STEAL";
        case CardType::REVERSE: return "REVERSE";
        case CardType::HIDE: return "HIDE";
        case CardType::COVER: return "COVER";
        case CardType::FIRST: return "FIRST";
        default: return "UNKNOWN";
    }
}
//PlayerAction::toString
/**
 * @brief 序列化 PlayerAction并以字符串形式上传
 * 当玩家完成某一动作后（如弃牌、出牌、指认牌），游戏内客户端会创建一个 PlayerAction对象封装这次动作的数据，并调用CClientSocket的sendData
 * 接口将数据 打包上传至服务器。由于sendData函数传入的是字符串，因此在序列化 PlayerAction之后，还要将其 转化至QString
 *  函数用途：游戏内客户端ClientGame上传 PlayerAction时调用，将玩家动作对象转化至 可传输形式。
 * @return 封装玩家动作相关信息的字符串，用以上传至游戏内服务器ServerGame
 */
QJsonObject PlayerAction::toJson() const{
    QJsonObject json;
    json["type"] = "player_action";
    json["action_type"] = static_cast<int>(action_type);
    json["player_id"] = player_id;
    json["card"] = card ? card->toJson() : QJsonObject();
    json["target_player_id"] = target_player_id;
    json["target_card"] = target_card ? target_card->toJson() : QJsonObject();
    return json;
}
//PlayerAction::fromString
/**
 * @brief 反序列化PlayerAction
 * 函数用途：游戏内服务器ServerGame接收PlayerAction 时调用，还原玩家动作类对象。根据 对象内封装的信息，ServerGame创建gameState 并发送至
 * 每一个ClientGame
 * @param str 接收的字符串，需要转化成PlayerAction对象
 * @return 转化后的PlayerAction类对象。
 */
PlayerAction PlayerAction::fromJson(const QJsonObject& json) {
    PlayerAction action;
    
    try {
        qDebug() << "Parsing PlayerAction from JSON:" << QJsonDocument(json).toJson();
        
        // 设置动作类型
        action.action_type = static_cast<Action>(json.value("action_type").toInt());
        qDebug() << "Action type set to:" << static_cast<int>(action.action_type);
        
        // 设置玩家ID
        action.player_id = json.value("player_id").toString();
        qDebug() << "Player ID set to:" << action.player_id;
        
        // 设置目标玩家ID
        if (json.contains("target_player_id")) {
            action.target_player_id = json.value("target_player_id").toString();
            qDebug() << "Target player ID set to:" << action.target_player_id;
        }
        
        // 设置卡牌索引
        if (json.contains("card_index")) {
            action.card_index = json.value("card_index").toInt();
            qDebug() << "Card index set to:" << action.card_index;
        }
        
        // 如果是指认动作，确保必要字段存在
        if (action.action_type == POINT_OUT_CARD) {
            if (action.target_player_id.isEmpty()) {
                qDebug() << "Error: Missing target_player_id for point out action";
            }
            if (action.card_index <= 0) {
                qDebug() << "Error: Invalid card_index for point out action";
            }
        }
        
        // 创建卡牌
        int cardId = json.value("card_id").toInt();
        bool faceUp = json.value("face_up").toBool();
        
        // 根据 cardId 确定卡牌类型
        CardType cardType;
        QString effect;
        switch(cardId) {
            case ANSWER_ID:
                cardType = CardType::ANSWER;
                effect = "Effect for Answer";
                break;
            case SOLVE_ID:
                cardType = CardType::SOLVE;
                effect = "Effect for Answer";
                break;
            case READ_ID:
                cardType = CardType::READ;
                effect = "Effect for Answer";
                break;
            case THINK_ID:
                cardType = CardType::THINK;
                effect = "Effect for Answer";
                break;
            case INDUCE_ID:
                cardType = CardType::INDUCE;
                effect = "Effect for Answer";
                break;
            case TRAP_ID:
                cardType = CardType::TRAP;
                effect = "Effect for Answer";
                break;
            case TRADE_ID:
                cardType = CardType::TRADE;
                effect = "Effect for Answer";
                break;
            case STEAL_ID:
                cardType = CardType::STEAL;
                effect = "Effect for Answer";
                break;
            case REVERSE_ID:
                cardType = CardType::REVERSE;
                effect = "Effect for Answer";
                break;
            case HIDE_ID:
                cardType = CardType::HIDE;
                effect = "Effect for Answer";
                break;
            case COVER_ID:
                cardType = CardType::COVER;
                effect = "Effect for Answer";
                break;
            case FIRST_ID:
                cardType = CardType::FIRST;
                effect = "Effect for Answer";
                break;
            default:
                qDebug() << "Error: Unknown card ID:" << cardId;
                return action;
        }
        
        // 使用基类创建卡牌
        card_pointer card = std::make_shared<Card>(cardType, effect);
        if (card) {
            card->setFaceUp(faceUp);
            action.card = card;
            qDebug() << "Successfully created card of type:" << getCardTypeName(card->getType());
        } else {
            qDebug() << "Failed to create card for ID:" << cardId;
        }
        
    } catch (const std::exception& e) {
        qDebug() << "Error parsing PlayerAction from JSON:" << e.what();
    }
    
    return action;
}

void PlayerAction::setActionType(Action actionType) {
    action_type = actionType;
}

Action PlayerAction::getActionType() const {
    return action_type;
}

void PlayerAction::setPlayerID(QString id) {
    player_id = id;
}

QString PlayerAction::getPlayerID() const {
    return player_id;
}

void PlayerAction::setCard(std::shared_ptr<Card> card) {
    this->card = card;
}

void PlayerAction::setTargetPlayerID(QString id) {
    target_player_id = id;
}

QString PlayerAction::getTargetPlayerID() const {
    return target_player_id;
}

void PlayerAction::setTargetCard(std::shared_ptr<Card> card) {
    this->target_card = card;
}

std::shared_ptr<Card> PlayerAction::getTargetCard() const {
    return target_card;
}




