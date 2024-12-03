#ifndef TRAPCARD_H
#define TRAPCARD_H

#include "Card.h"
#include <QString>
#include "../Player/Player.h"
#include "../GameState/GameState.h"

class TrapCard : public Card {
    public:
        TrapCard(const QString e) : Card(CardType::TRAP, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
        void trigger(Player* player_pointer, GameState& gameState, bool face_up) override;
};

#endif // TRAPCARD_H
