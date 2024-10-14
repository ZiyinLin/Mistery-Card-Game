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
    midi,   //谜底
    jie,    //解
    du,     //读
    lv,     //虑
    you,    //诱
    xian,   //陷
    yi,     //易
    qie,    //窃
    su,     //溯
    ni,     //匿
    huang,  //幌
    xu      //序
};

class Card{
public:
    // use member initialization list to initialize
    Card(CardType t, std::string e) : card_type(t), effect(e){}

    // to reveal or read a card
    void reveal();  
    CardType getType();

    int getMysteryPoints();
    void setMysteryPoints(int points);
    
    // diffrent cards are played diffrently, therefore we have to override play function
    virtual void play(Player* player_pointer, GameState& gameState, bool face_up);

private:
    CardType card_type;
    std::string effect;
    bool face_up;  // to mark is a card is facing up
    bool has_trigger_effect;  // to meet the requirment of "解" and "匿"
    int mystery_point;
    
};

using card_pointer = std::shared_ptr<Card>;

#endif