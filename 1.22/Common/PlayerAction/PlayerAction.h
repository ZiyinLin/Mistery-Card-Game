#ifndef PLAYERACTION_H
#define PLAYERACTION_H

#include <QString>
#include <QJsonObject>
#include "../Card/Card.h"

enum Action {
    DISCARD_CARD,                 // 弃牌阶段时弃置手牌
    PLAY_CARD,                   // 出牌阶段时打出手牌
    SELECT_ORIENTED_CARD, 
    SELECT_ORIENTED_PLAYER,       // 打出解或读时选择他人卡牌
    POINT_OUT_CARD,              // 指认阶段时指认他人卡牌
};

class PlayerAction {
public:
    PlayerAction(){}

    QJsonObject toJson() const;
    static PlayerAction fromJson(const QJsonObject& json);
    void setActionType(Action actionType);
    Action getActionType() const;
    void setPlayerID(QString id);
    QString getPlayerID() const;
    void setCard(std::shared_ptr<Card> card);
    std::shared_ptr<Card> getCard() const {
        if (!card) {
            qDebug() << "Warning: Attempting to get null card from PlayerAction";
        }
        return card;
    }
    void setTargetPlayerID(QString id);
    QString getTargetPlayerID() const;
    void setTargetCard(std::shared_ptr<Card>);
    std::shared_ptr<Card> getTargetCard() const;
    void setCardIndex(int index) { card_index = index; }
    int getCardIndex() const { return card_index; }

private:
    Action action_type;
    QString player_id;          // current player's id
    std::shared_ptr<Card> card;              // to choose your hand card
    std::shared_ptr<Card> target_card;       // to choose other's card
    QString target_player_id;
    bool face_up;
    int card_index;           // 被指认的卡牌索引
};

#endif // PLAYERACTION_H
