#ifndef FIRSTCARD_H
#define FIRSTCARD_H

#include "Card.h"
#include <QString>
#include "../Player/Player.h"
#include "../GameState/GameState.h"

class FirstCard : public Card {
    public:
        FirstCard(const QString e) : Card(CardType::FIRST, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
        void trigger(Player* player_pointer, GameState& gameState, bool face_up) override;
};

#endif // FIRSTCARD_H
