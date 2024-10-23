#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"
#include "AnswerCard.h"
using namespace std;
void AnswerCard::play(Player* player, GameState &gameState, bool faceUp){
    this->setFaceUp(faceUp);
    player->getPlayedCards().push_back(shared_from_this());
    if(face_up){
        gameState.setState(SETTLE);
    }
    
    else{
        this->setMysteryPoints(1);
    }
}