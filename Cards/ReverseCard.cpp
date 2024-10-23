#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"
#include "ReverseCard.h"
using namespace std;

void ReverseCard::play(Player* player, GameState& gameState, bool faceUp){
    this->setFaceUp(faceUp);
    player->getPlayedCards().push_back(shared_from_this());
    
    if(face_up){
        //获取所有玩家
        const auto& allPlayers = gameState.getAllPlayers();
        //遍历所有玩家
        for(Player* currentPlayer : allPlayers){
            //跳过打出“溯”的当前玩家
            if(currentPlayer==player){
                continue;
            }
            
            vector<card_pointer> playedCards = currentPlayer->getPlayedCards();
            
            if(!playedCards.empty()){
                //将其他玩家打出的最后一张牌收回他们的手牌
                card_pointer lastPlayedCard = playedCards.back();
                playedCards.pop_back();
                currentPlayer->addHandCard(lastPlayedCard);
            }
        }
    }
   
    else{
        this->setMysteryPoints(1);
    }
}