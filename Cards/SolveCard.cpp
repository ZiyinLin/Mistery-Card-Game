#include "Card.h"
#include "Player.h"
#include "GameState.h"
#include "SolveCard.h"
#include <string>
#include <algorithm>
using namespace std;

void SolveCard::play(Player* player, GameState& gameState, bool faceUp){
    this->setFaceUp(faceUp);
    player->getPlayedCards().push_back(shared_from_this());

    if(face_up){
        //选择一张牌
        card_pointer targetCard = player->chooseTargetCard(gameState);
        
        while(true){
            bool isFaceUp = targetCard->getFaceUp();
            
            if(!isFaceUp){
                //将这张暗置牌掀开
                targetCard->setFaceUp(true);
                
                //如果掀到谜底牌则游戏结束，打出“解”的当前玩家获胜
                if(targetCard->getType()==ANSWER){
                    gameState.setState(SETTLE);
                    player->setWin(true);
                }
                
                //如果掀到“匿”
                if(targetCard->getType()==HIDE){
                    vector<Player*> allPlayers = gameState.getAllPlayers();
                    for(Player* currentPlayer : allPlayers){
                         vector<card_pointer> playedCards = currentPlayer->getPlayedCards();
                         //找到持有这张“匿”的玩家
                         if(find(playedCards.begin(), playedCards.end(), targetCard)!=playedCards.end()){
                             //遍历该玩家的出牌堆
                             for(int i=0; i<playedCards.size();++i){
                                 //若出牌堆中有谜底牌，则游戏结束，打出“解”的当前玩家获胜
                                 if(playedCards[i]->getType()==ANSWER){
                                    //将由“匿”牌的play函数改变游戏状态，不在这里改变
                                    player->setWin(true);
                                    break;
                                 }
                             }
                        }
                   }
                }
                break;//处理完“解”，退出循环
            }
            //不合法，继续点击其他卡牌
            gameState.sendErrorMessage(player->getPlayerID(),"请选择其他人的暗置牌！");
        }
    }

    else{
        this->setMysteryPoints(1);
    }
}