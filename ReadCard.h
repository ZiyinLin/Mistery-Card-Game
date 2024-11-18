#ifndef READCARD_H
#define READCARD_H

#include "Card.h"
#include <QString>
#include "Player.h"
#include "GameState.h"

class ReadCard : public Card{
    public:
        ReadCard(const QString e) : Card(CardType::READ, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
        void trigger(Player* player_pointer, GameState& gameState, bool face_up) override;
};
#endif // READCARD_H
