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
       //获取玩家出牌堆
      const auto& playedCards = player->getPlayedCards();
       
      //计算出牌堆中暗置牌的数量
      int count=0;
      for(card_pointer card : playedCards){
         bool isFaceUp = card->getFaceUp();
         if(!isFaceUp){
            count++;
         }
      }

    if(this->getFaceUp()==true){
       //出牌堆没有暗置牌，技能无法生效
       if(count==0){
          gameState.sendErrorMessage(player->getPlayerID(),"没有暗置牌，技能无效！");
          this->setValidPlay(false);
          return;
       }
       
       player->getPlayedCards().push_back(shared_from_this());
       
       //收回所有暗置牌
       for(card_pointer card:playedCards){
         if(card->getFaceUp()==false){
            player->removePlayedCard(card);
            player->addHandCard(card);
         }
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

       this->setValidPlay(true);
       
    }
    else{
      player->getPlayedCards().push_back(shared_from_this());
      this->setMysteryPoints(1);
      this->setValidPlay(true);
    }
}


void CoverCard::trigger(Player* player, GameState &gameState, bool faceUp){
   this->setFaceUp(faceUp);
       //获取玩家出牌堆
      const auto& playedCards = player->getPlayedCards();
       
      //计算出牌堆中暗置牌的数量
      int count=0;
      for(card_pointer card : playedCards){
         bool isFaceUp = card->getFaceUp();
         if(!isFaceUp){
            count++;
         }
      }

      if(count==0){
         return;
      }else{
         this->play(player, gameState, true);
      }
   
}

