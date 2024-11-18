#ifndef HIDECARD_H
#define HIDECARD_H
#include "Card.h"
#include <QString>
#include "Player.h"
#include "GameState.h"

class HideCard : public Card{
    public:
        HideCard(const QString e) : Card(CardType::HIDE, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
        void trigger(Player* player_pointer, GameState& gameState, bool face_up) override;
};

#endif // HIDECARD_H
