#include "StealCard.h"
#include "../Player/Player.h"
#include "../GameState/GameState.h"
#include <QString>
#include <vector>
#include <algorithm>
#include "Card.h"
using namespace std;

//StealCard::play
/**
 * @brief 触发“窃”牌的效果。
 * 具体情况
 * 暗置打出：触发暗置的统一效果，详见AnswerCard和CoverCard
 * 明置打出：拿走任意一张场上其他玩家的明置牌（“ 陷”牌除外），用自己手里一张可以明置打出的牌放回那张明置牌原本的位置，放过去之后不触发效果。
 * 不合法情况：1.明置打出时场上其他玩家没有明置牌。
 *           2.选择卡牌时选择了非法的牌。
 * @param player 玩家指针类型变量，表示打出这张牌的玩家。
 * @param gameState GameState类变量，用以表示当前的游戏状态和数据
 * @param faceUp  布尔类型变量，表示卡牌的明暗置打出。
 */
void StealCard::play(Player* player, GameState& gameState, bool faceUp){
   this->setFaceUp(faceUp);
   //先创建int类型变量，计算场上其他玩家的明置牌数量
   int count=0;

   const auto& allPlayers = gameState.getAllPlayers();
   //遍历除当前玩家外的所有玩家
   for(Player* currentPlayer : allPlayers){
      if(currentPlayer==player){
         continue;
      }
      //遍历每个玩家的出牌容器
      vector<card_pointer>playedCards = currentPlayer->getPlayedCards();
      for(int i=0; i<playedCards.size();++i){
         if(playedCards[i]->getFaceUp()==false){
            count++;
         }
      }
   }

   int handCount=0;
   for(card_pointer card : player->getHandCards()){
      if(card->getType()!=TRAP && card->getType()!=HIDE && card->getType()!=ANSWER){
         handCount++;
      }
   }


    if(this->getFaceUp()==true){
         //场上其他玩家没有明置牌，收回这张牌
         if(count==0){
            gameState.sendErrorMessage(player->getPlayerID(),"其他玩家没有暗置牌，无法明置打出！");
            this->setValidPlay(false);
            return;
         }

         if(handCount==0){
            gameState.sendErrorMessage(player->getPlayerID(),"没有可以明置打出的手牌，无法明置打出！");
            this->setValidPlay(false);
            return;
         }

         player->getPlayedCards().push_back(shared_from_this());

        while(true){

            card_pointer targetCard = player->chooseTargetCard(gameState);
            bool isFaceUp=targetCard->getFaceUp();

            if(isFaceUp){
               Player* targetPlayer = nullptr;
               int targetCardIndex = -1;
               const auto& allPlayers = gameState.getAllPlayers();
               //遍历所有玩家
               for(Player* currentPlayer : allPlayers){
                  vector<card_pointer> playedCards = currentPlayer->getPlayedCards();
                  //找到持有targetCard的玩家
                  if(find(playedCards.begin(), playedCards.end(), targetCard)!=playedCards.end()){
                     targetPlayer=currentPlayer;
                     //记录targetCard的所在位置
                     targetCardIndex=distance(playedCards.begin(),find(playedCards.begin(), playedCards.end(), targetCard));
                     break;//退出遍历循环
                  }
               }

               //从自己手牌中选择一张可以明置打出的牌
               while(true){
                 card_pointer playerCard = player->chooseHandCard(gameState);
                 CardType type = playerCard->getType();
                 //合法，执行完窃牌逻辑后退出当前循环
                 if(type!=TRAP && HIDE && ANSWER){
                    if (targetPlayer && targetCardIndex >= 0){
                        //将targetCard移出他的出牌，并设置为暗置
                        targetPlayer->removePlayedCard(targetCard);
                        targetCard->setFaceUp(false);
                        //将playerCard移出当前玩家手牌并设置为明置
                        player->removeHandCard(playerCard);
                        playerCard->setFaceUp(true);
                        //将targetCard加入当前玩家手牌
                        player->addHandCard(targetCard);
                        //将playerCard加入他出牌中的记录位置
                        targetPlayer->addPlayedCardAt(playerCard, targetCardIndex);
                        break;
                    }
                 }
                 //选择了不能明置打出的牌，重新选择手牌
                 gameState.sendErrorMessage(player->getPlayerID(),"请选择一张可以明置打出的手牌！");
               }


            break;//退出外层循环

               }

            //选择了暗置牌，继续选择其他玩家的一张明置牌
            gameState.sendErrorMessage(player->getPlayerID(),"请选择其他玩家的一张明置牌！");

            }

        }

    else{
      player->getPlayedCards().push_back(shared_from_this());
      this->setMysteryPoints(1);
      }

    }


void StealCard::trigger(Player* player, GameState &gameState, bool faceUp){
   //先创建int类型变量，计算场上其他玩家的明置牌数量
   int count=0;

   const auto& allPlayers = gameState.getAllPlayers();
   //遍历除当前玩家外的所有玩家
   for(Player* currentPlayer : allPlayers){
      if(currentPlayer==player){
         continue;
      }
      //遍历每个玩家的出牌容器
      vector<card_pointer>playedCards = currentPlayer->getPlayedCards();
      for(int i=0; i<playedCards.size();++i){
         if(playedCards[i]->getFaceUp()==false){
            count++;
         }
      }
   }

   if(count==0){
      return;
   }else{
      this->play(player, gameState, true);
   }
}
