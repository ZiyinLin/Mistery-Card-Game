#ifndef SOLVECARD_H
#define SOLVECARD_H

#include "Card.h"
#include <QString>
#include "../Player/Player.h"
#include "../GameState/GameState.h"

class SolveCard : public Card {
    public:
        SolveCard(const QString e) : Card(CardType::SOLVE, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
        void trigger(Player* player_pointer, GameState& gameState, bool face_up) override;
};

#endif // SOLVECARD_H
