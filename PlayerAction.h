#ifndef PLAYERACTION_H
#define PLAYERACTION_H


#include "Card.h"
#include "Message.h"
#include <QString>


enum Action{
    DISCARD_CARD,                 // 弃牌阶段时弃置手牌
    PLAY_CARD,                   // 出牌阶段时打出手牌
    SELECT_ORIENTED_CARD,        // 打出解或读时选择他人卡牌
    POINT_OUT_CARD,              // 指认阶段时指认他人卡牌
};

class PlayerAction{
public:

    PlayerAction(Action a, QString s, card_pointer c, QString ts="", card_pointer tc=nullptr):
    action_type(a), player_id(s), card(c), target_player_id(ts), target_card(tc){}


    //序列化后用Message封装
    Message toMessage();

    //从Message对象中提取内容并反序列化
    PlayerAction fromMessage(Message& message);

    Action getActionType();

    QString getPlayerId();

    card_pointer getCard();

    QString getTargetPlayerID();

    card_pointer getTargetCard();

    bool getFaceUp();



private:
    Action action_type;
    QString player_id;          // current player's id
    card_pointer card;              // to choose your hand card
    card_pointer target_card;       // to choose other's card
    QString target_player_id;
    bool face_up;
};


#endif // PLAYERACTION_H
