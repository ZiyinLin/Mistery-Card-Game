#include "Card.h"
#include "Player.h"
#include "GameState.h"
#include "CoverCard.h"
#include <string>
#include <vector>
#include <random>
using namespace std;

void CoverCard::play(Player* player, GameState &gameState, bool faceUp){
    this->setFaceUp(faceUp);
    player->getPlayedCards().push_back(shared_from_this());

    if(face_up){
       //获取玩家出牌堆
       vector<card_pointer> playerPlayedCards = player->getPlayedCards();
       
       //计算出牌堆中暗置牌的数量
       int count=0;
       for(card_pointer card : playerPlayedCards){
          bool isFaceUp = card->getFaceUp();
          if(!isFaceUp){
            count++;
            //将这些暗置牌收回手牌
            player->removePlayedCard(card);
            player->addHandCard(card);
          }
       }
       
       //出牌堆没有暗置牌，技能无法生效
       if(count==0){
          gameState.sendErrorMessage(player->getPlayerID(),"没有暗置牌，技能无效！");
          return;
       }
        
       //打出等量的暗置牌
       int remain_cover=count;
       while(remain_cover>0){
          card_pointer card = player->chooseHandCard(gameState);
          card->setFaceUp(false);
          player->removeHandCard(card);
          player->addPlayedCard(card);
          remain_cover--;
       }
       
    }
    else{
       this->setMysteryPoints(1);
    }
}