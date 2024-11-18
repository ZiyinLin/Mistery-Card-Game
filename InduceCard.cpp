#include "Card.h"
#include <QString>
#include "Player.h"
#include "GameState.h"
#include "InduceCard.h"
using namespace std;
void InduceCard::play(Player* player, GameState& gameState, bool faceUp){
    this->setFaceUp(faceUp);
    //获取下家
    Player* nextPlayer = gameState.getNextPlayer(player);

    if(this->getFaceUp()==true){
        if (!nextPlayer) {
            gameState.sendErrorMessage(player->getPlayerID(), "无法找到下家。");
            this->setValidPlay(false);
            return;
        }
        if(nextPlayer->getHandCards().empty()){
            gameState.sendErrorMessage(player->getPlayerID(), "下家已没有手牌！");
            this->setValidPlay(false);
            return;
        }
        player->getPlayedCards().push_back(shared_from_this());
        gameState.setCanNormalPlay(nextPlayer->getPlayerID(), false);

    }

    else{
        this->setMysteryPoints(1);
    }

}

void InduceCard::trigger(Player* player, GameState &gameState, bool faceUp){
    Player* nextPlayer = gameState.getNextPlayer(player);
    if(!nextPlayer || nextPlayer->getHandCards().empty()){
        return;
    }else{
        this->play(player, gameState, true);
    }
}
