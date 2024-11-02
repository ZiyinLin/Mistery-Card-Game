#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"
#include "AnswerCard.h"
using namespace std;
void AnswerCard::play(Player* player, GameState &gameState, bool faceUp){
    this->setFaceUp(faceUp);
    if(this->getFaceUp()==true){
        gameState.sendErrorMessage(player->getPlayerID(),"不能明置打出此卡牌！");
        this->setValidPlay(false);
    }else{
        player->getPlayedCards().push_back(shared_from_this());
        this->setMysteryPoints(1);
        this->setValidPlay(true);
    }
}

void AnswerCard::trigger(Player* player, GameState &gameState, bool faceUp){
    gameState.setState(SETTLE);
}