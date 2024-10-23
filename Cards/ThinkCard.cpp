#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"
#include "ThinkCard.h"
#include "random"
using namespace std;

void ThinkCard::play(Player* player, GameState& gameState, bool faceUp){
    this->setFaceUp(faceUp);
    player->getPlayedCards().push_back(shared_from_this());
        
    if(face_up){
       int remain_think=2;
       //共换两次
       while(remain_think>0){
          
          while(true){
            card_pointer playerCard = player->chooseHandCard(gameState);
            CardType type = playerCard->getType();
            //选择合法牌，执行完一次虑牌逻辑后退出当前循环
            if(type!=ANSWER){
               //获取弃牌堆容器
                vector<card_pointer> discardedCards = player->getDiscardedCards();
                //若弃牌堆为空，则直接return，技能无效
                if(discardedCards.empty()){
                    gameState.sendErrorMessage(player->getPlayerID(), "弃牌堆为空，无法交换！");
                    return;
                }
            
                 random_device rd; // 获取随机数种子
                 mt19937 gen(rd()); // 使用梅森旋转算法生成随机数
                 uniform_int_distribution<> dis(0, discardedCards.size() - 1); // 创建均匀分布
                 int randomIndex = dis(gen); // 随机生成一个索引
                 card_pointer discardedCard = discardedCards[randomIndex]; // 获取随机丢弃的卡牌

                 //与弃牌堆换牌
                 player->removeDiscardedCard(discardedCard);
                 player->addHandCard(discardedCard);
                 player->removeHandCard(playerCard);
                 player->addDiscardedCard(playerCard);

                 break;
            }
            //选择了谜底牌，继续选择自己手牌中的非谜底牌
            gameState.sendErrorMessage(player->getPlayerID(), "不能交换谜底牌！");
          
          }
          
         
        remain_think--;
       }
    }
    
    else{
    this->setMysteryPoints(1);
      }

}
