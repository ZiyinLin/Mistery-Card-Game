#include "Card.h"
#include "Player.h"
#include "GameState.h"
#include "StealCard.h"
#include <string>
#include <algorithm>
using namespace std;

void StealCard::play(Player* player, GameState& gameState, bool faceUp){
    this->setFaceUp(faceUp);
    player->getPlayedCards().push_back(shared_from_this());

    if(face_up){
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
        this->setMysteryPoints(1);
      }

    }
