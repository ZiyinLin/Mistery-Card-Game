#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"
#include "ReadCard.h"
using namespace std;

void ReadCard::play(Player* player, GameState& gameState, bool faceUp){
    this->setFaceUp(faceUp);
    player->getPlayedCards().push_back(shared_from_this());

    if(face_up){
        //先创建int类型变量，计算场上其他玩家的暗置牌数量
        int count=0;
        
        vector<Player*> allPlayers = gameState.getAllPlayers();
        //遍历除当前玩家外的所有玩家
        for(Player* currentPlayer : allPlayers){
            if(currentPlayer==player){
                continue;
            }
            //遍历每个玩家的出牌容器
            vector<card_pointer>playedCards = currentPlayer->getPlayedCards();
            for(int i=0; i<playedCards.size();++i){
                if(playedCards[i]->getFaceUp()==true){
                    count++;
                }
            }
        }
        //场上其他玩家没有暗置牌，技能不生效
        if(count==0){
            gameState.sendErrorMessage(player->getPlayerID(),"其他玩家没有暗置牌，技能无法生效！");
            return;
        }
        
        //其他玩家有暗置牌，继续执行函数
        while(true){
            card_pointer targetCard = player->chooseTargetCard(gameState);
            bool isFaceUp=targetCard->getFaceUp();
            bool lastFaceDown=targetCard->getLastFaceDown();
            
            //选择了非法牌，重新选择卡牌
            if(faceUp||!lastFaceDown){
                gameState.sendErrorMessage(player->getPlayerID(), "请选择一位玩家最晚打出的暗置牌！");
                continue;
            }

            //非“陷”牌则给玩家显示，“陷”牌直接掀开
            if(targetCard->getType()!=TRAP){
                targetCard->showCard(player);
                break;
            }else{
                targetCard->setFaceUp(true);
                break;
            }
        }
    }

    else{
        this->setMysteryPoints(1);
    }
}