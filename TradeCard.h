#ifndef TRADECARD_H
#define TRADECARD_H

#include "Card.h"
#include <QString>
#include "Player.h"
#include "GameState.h"

class TradeCard : public Card{
    public:
        TradeCard(const QString e) : Card(CardType::TRADE, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
        void trigger(Player* player_pointer, GameState& gameState, bool face_up) override;
};

#endif // TRADECARD_H
