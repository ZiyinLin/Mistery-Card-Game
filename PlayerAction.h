#ifndef PLAYERACTION_H
#define PLAYERACTION_H

#include "Card.h"
#include <string>

enum Action{
    dicard_card,                 // to discard cards in 1st phrase
    play_card,                   // to play a card in 2nd phrase
    select_oriented_card,        // to select objective card of "JIE" and "du"
    point_out_card,              // to point out a card in 3rd phrase
};

class PlayerAction{
public:
    PlayerAction(Action a, const std::string s) :
    action_type(a), player_id(s){}



private:
    Action action_type;
    std::string player_id;          // current player's id
    card_pointer card;              // to choose your hand card
    card_pointer target_card;       // to choose other's card
    std::string target_player_id;
    bool faceUp;
};

#endif