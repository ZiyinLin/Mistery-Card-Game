#ifndef STEALCARD_H
#define STEALCARD_H

#include "Card.h"
#include <QString>
#include "../Player/Player.h"
#include "../GameState/GameState.h"

class StealCard : public Card {
    public:
        StealCard(const QString e) : Card(CardType::STEAL, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
        void trigger(Player* player_pointer, GameState& gameState, bool face_up) override;
};

#endif // STEALCARD_H
