#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"
#include "TrapCard.h"
using namespace std;

void TrapCard::play(Player* player, GameState& gameState, bool faceUp){
    this->setFaceUp(faceUp);

    if(this->getFaceUp()==true){
        gameState.sendErrorMessage(player->getPlayerID(),"不能明置打出此卡牌！");
        this->setValidPlay(false);
        return;
    }
    else{
        player->getPlayedCards().push_back(shared_from_this());
        this->setMysteryPoints(1);
    }
}

void TrapCard::trigger(Player* player, GameState& gameState, bool faceUp){
    this->setMysteryPoints(3);
}