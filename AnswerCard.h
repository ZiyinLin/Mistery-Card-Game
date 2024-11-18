#ifndef ANSWERCARD_H
#define ANSWERCARD_H

#include "Card.h"
#include <QString>
#include "Player.h"
#include "GameState.h"

class AnswerCard : public Card{
    public:
        AnswerCard(const QString e) : Card(CardType::ANSWER, e){}
        void play(Player* player_pointer, GameState& gameState, bool faceUp) override;
        void trigger(Player* player_pointer, GameState& gameState, bool face_up) override;
};

#endif // ANSWERCARD_H
