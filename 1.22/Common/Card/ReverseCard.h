#ifndef REVERSECARD_H
#define REVERSECARD_H

#include "Card.h"
#include <QString>
#include "../Player/Player.h"
#include "../GameState/GameState.h"

class ReverseCard : public Card {
    public:
        ReverseCard(const QString e) : Card(CardType::REVERSE, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
        void trigger(Player* player_pointer, GameState& gameState, bool face_up) override;
};

#endif // REVERSECARD_H
