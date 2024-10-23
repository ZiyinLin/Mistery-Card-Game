#ifndef SOLVE_CARD_H
#define SOLVE_CARD_H
#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"

class SolveCard : public Card{
    public:
        SolveCard(const std::string e) : Card(CardType::SOLVE, e){}
        void play(Player* player, GameState& gameState, bool faceUp) override;
};
#endif