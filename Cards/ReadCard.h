#ifndef READ_CARD_H
#define READ_CARD_H
#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"

class ReadCard : public Card{
    public:
        ReadCard(const std::string e) : Card(CardType::READ, e){}
        void play(Player* player_pointer, GameState& gameState, bool face_up) override;
};
#endif