#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"
#include "InduceCard.h"
using namespace std;
void InduceCard::play(Player* player, GameState& gameState, bool faceUp){
    this->setFaceUp(faceUp);
    player->getPlayedCards().push_back(shared_from_this());
    
    if(face_up){
        //获取下家
        Player* nextPlayer = gameState.getNextPlayer(player);
        if (!nextPlayer) {
             gameState.sendErrorMessage(player->getPlayerID(), "无法找到下家。");
             return;
    }
        gameState.setCanNormalPlay(nextPlayer->getPlayerID(), false);

    }
    
    else{
        this->setMysteryPoints(1);
    }

}