/*  This file define the basic data types and methord
 *  of card, including types, effects, mystery points,
 *  whether it's facing-up or has triggered effect.
 *  The key point is that we need to override function
 * "play" in order to satisfy different effects.
 */
#ifndef CARD_H
#define CARD_H

#include "GameState.h"
#include "Player.h"
#include <string>
#include <memory>

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
public:
    // use member initialization list to initialize
    Card(CardType t, std::string e) : card_type(t), effect(e){}
    
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
    
    // diffrent cards are played diffrently, therefore we have to override play function
    virtual void play(Player* player_pointer, GameState& gameState, bool face_up);

protected:
    CardType card_type;
    std::string effect;
    bool face_up;  // to mark is a card is facing up
    bool has_trigger_effect=false;  // to meet the requirment of "解" and "匿"
    bool is_last_face_down;
    int mystery_point=0;
    
};

using card_pointer = std::shared_ptr<Card>;

#endif