#ifndef FIRST_CARD_H
#define FIRST_CARD_H
#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"

class FirstCard : public Card{
    public:
        FirstCard(const std::string e) : Card(CardType::FIRST, e){}
        void play(Player* player_pointer, GameState& gameState, bool faceUp) override;
};
#endif