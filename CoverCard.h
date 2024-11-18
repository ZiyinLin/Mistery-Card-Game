#ifndef COVERCARD_H
#define COVERCARD_H

#include "Card.h"
#include <QString>
#include "Player.h"
#include "GameState.h"

class CoverCard : public Card{
    public:
        CoverCard(const QString e) : Card(CardType::COVER, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
        void trigger(Player* player_pointer, GameState& gameState, bool face_up) override;
};
#endif // COVERCARD_H
