#ifndef STEAL_CARD_H
#define STEAL_CARD_H
#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"

class StealCard : public Card{
    public:
        StealCard(const std::string e) : Card(CardType::STEAL, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
};
#endif