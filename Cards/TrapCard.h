#ifndef TRAP_CARD_H
#define TRAP_CARD_H
#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"

class TrapCard : public Card{
    public:
        TrapCard(const std::string e) : Card(CardType::TRAP, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
};
#endif