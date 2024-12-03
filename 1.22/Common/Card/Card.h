#ifndef CARD_H
#define CARD_H

#include <QObject>
#include <QString>
#include <memory>

// 前向声明
class Player;
class GameState;
//Card::CardType
/**
 * @brief  该枚举类型变量表示12种不同类型的卡牌，每种对应不同效果。
 */
enum CardType {
    ANSWER,   //谜底
    SOLVE,    //解
    READ,     //读
    THINK,    //虑
    INDUCE,   //诱
    TRAP,     //陷
    TRADE,    //易
    STEAL,    //窃
    REVERSE,  //溯
    HIDE,     //匿
    COVER,    //幌
    FIRST     //序
};

enum CardID {
    ANSWER_ID = 1,    // 谜底
    SOLVE_ID = 2,     // 解
    READ_ID = 3,      // 读
    THINK_ID = 4,     // 虑
    INDUCE_ID = 5,    // 诱
    TRAP_ID = 6,      // 陷
    TRADE_ID = 7,     // 易
    STEAL_ID = 8,     // 窃
    REVERSE_ID = 9,   // 溯
    HIDE_ID = 10,     // 匿
    COVER_ID = 11,    // 幌
    FIRST_ID = 12     // 序
};

class Card : public std::enable_shared_from_this<Card> {
public:
    using card_pointer = std::shared_ptr<Card>;
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

    virtual void play(Player* player_pointer, GameState& gameState, bool face_up);
    virtual void trigger(Player* player_pointer, GameState& gameState, bool face_up);

    QJsonObject toJson();
    static card_pointer fromJson(const QJsonObject& json);
    QString getCardTypeName() const {
        switch(card_type) {
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

protected:
    CardType card_type;
    QString effect;
    bool face_up;  // to mark is a card is facing up
    bool has_trigger_effect = false;// to meet the requirment of "解" and "匿"
    bool is_last_face_down;
    bool valid_play = true;//判断是否有效执行了play函数
    int mystery_point = 0;
};

#endif // CARD_H
