#ifndef ANSWER_CARD_H
#define ANSWER_CARD_H
#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"

class AnswerCard : public Card{
    public:
        AnswerCard(const std::string e) : Card(CardType::ANSWER, e){}
        void play(Player* player_pointer, GameState& gameState, bool faceUp) override;
};
#endif