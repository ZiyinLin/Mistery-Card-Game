#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"
#include "HideCard.h"
using namespace std;

void HideCard::play(Player* player, GameState& gameState, bool faceUp){
    
    if(gameState.getState()==PLAY_CARD){
        if(face_up){
          this->setFaceUp(faceUp);
          player->getPlayedCards().push_back(shared_from_this());
          
          //获取玩家出牌列表
          std::vector<card_pointer>playedCards=player->getPlayedCards(); 
          
          //遍历玩家出牌列表，找到当前这张“匿”的位置
          for(auto it=playedCards.begin();it!=playedCards.end();++it){
             if(it->get()==this){
                 //这张“匿”之前所有卡牌都设置为face_up=true
                 for(auto priorCardIt=playedCards.begin();priorCardIt!=it;++priorCardIt){
                    (*priorCardIt)->setFaceUp(true);
                    (*priorCardIt)->setHasTrigger(true);
                    //若有牌是“谜底”，直接进入结算阶段
                    CardType type= (*priorCardIt)->getType();
                    if(type==ANSWER){
                        gameState.setState(SETTLE);
                    }
                 }
                 break;//找到并处理完“匿”，退出循环
             }
          }
        }
        
        else{
            this->setMysteryPoints(1);
        }
    }
    
    
    if(gameState.getState()==SHOW_HIDE){
      //将其翻开，并获得最多的谜
      face_up=true;
      this->setMysteryPoints(100);
    }
}