#ifndef TRADE_CARD_H
#define TRADE_CARD_H
#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"

class TradeCard : public Card{
    public:
        TradeCard(const std::string e) : Card(CardType::TRADE, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
        void trigger(Player* player_pointer, GameState& gameState, bool face_up) override;
};
#endif