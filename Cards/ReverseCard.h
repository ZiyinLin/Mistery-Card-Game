#ifndef REVERSE_CARD_H
#define REVERSE_CARD_H
#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"

class ReverseCard : public Card{
    public:
        ReverseCard(const std::string e) : Card(CardType::REVERSE, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
        void trigger(Player* player_pointer, GameState& gameState, bool face_up) override;
};
#endif