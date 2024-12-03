#ifndef INDUCECARD_H
#define INDUCECARD_H

#include "Card.h"
#include <QString>
#include "../Player/Player.h"
#include "../GameState/GameState.h"

class InduceCard : public Card {
    public:
        InduceCard(const QString e) : Card(CardType::INDUCE, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
        void trigger(Player* player_pointer, GameState& gameState, bool face_up) override;
};

#endif // INDUCECARD_H
