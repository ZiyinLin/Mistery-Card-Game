#ifndef COVER_CARD_H
#define COVER_CARD_H
#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"

class CoverCard : public Card{
    public:
        CoverCard(const std::string e) : Card(CardType::COVER, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
        void trigger(Player* player_pointer, GameState& gameState, bool face_up) override;
};
#endif