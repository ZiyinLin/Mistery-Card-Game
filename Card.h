#ifndef CARD_H
#define CARD_H

#include <QObject>
#include <QString>
#include <memory>
#include "Player.h"

enum CardType{
    ANSWER,   //谜底
    SOLVE,    //解
    READ,     //读
    THINK,     //虑
    INDUCE,    //诱
    TRAP,   //陷
    TRADE,     //易
    STEAL,    //窃
    REVERSE,     //溯
    HIDE,     //匿
    COVER,  //幌
    FIRST      //序
};

class Card : public std::enable_shared_from_this<Card>{
    using card_pointer = std::shared_ptr<Card>;
public:
    // use member initialization list to initialize
    Card(CardType t, QString e) : card_type(t), effect(e){}

    void setFaceUp(bool faceUp);
    bool getFaceUp();

    // to reveal or read a card
    void showCard(Player* player);

    CardType getType();

    int getMysteryPoints();
    void setMysteryPoints(int points);

    void setLastFaceDown(bool lastFaceDown);
    bool getLastFaceDown();

    void setHasTrigger(bool hasTrigger);
    bool getHasTrigger();

    void setValidPlay(bool validPlay);
    bool getValidPlay();

    // diffrent cards are played diffrently, therefore we have to override play function
    virtual void play(Player* player_pointer, GameState& gameState, bool face_up);
    virtual void trigger(Player* player_pointer, GameState& gameState, bool face_up);

    QJsonObject toJson();

    static card_pointer fromJson(const QJsonObject j);

protected:
    CardType card_type;
    QString effect;
    bool face_up;  // to mark is a card is facing up
    bool has_trigger_effect=false;  // to meet the requirment of "解" and "匿"
    bool is_last_face_down;
    bool valid_play=true;//判断是否有效执行了play函数
    int mystery_point=0;
};
 using card_pointer = std::shared_ptr<Card>;



#endif // CARD_H
