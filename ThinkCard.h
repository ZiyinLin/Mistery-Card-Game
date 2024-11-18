#ifndef THINKCARD_H
#define THINKCARD_H

#include "Card.h"
#include <QString>
#include "Player.h"
#include "GameState.h"

class ThinkCard : public Card{
    public:
        ThinkCard(const QString e) : Card(CardType::THINK, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
        void trigger(Player* player_pointer, GameState& gameState, bool face_up) override;
};
#endif // THINKCARD_H
