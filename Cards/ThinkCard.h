#ifndef THINK_CARD_H
#define THINK_CARD_H
#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"

class ThinkCard : public Card{
    public:
        ThinkCard(const std::string e) : Card(CardType::THINK, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
};
#endif