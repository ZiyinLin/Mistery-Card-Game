#ifndef INDUCE_CARD_H
#define INDUCE_CARD_H
#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"

class InduceCard : public Card{
    public:
        InduceCard(const std::string e) : Card(CardType::INDUCE, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
        void trigger(Player* player_pointer, GameState& gameState, bool face_up) override;
};
#endif