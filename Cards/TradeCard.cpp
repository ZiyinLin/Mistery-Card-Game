#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"
#include "TradeCard.h"
using namespace std;

void TradeCard::play(Player* player, GameState& gameState, bool faceUp){
    this->setFaceUp(faceUp);
    player->getPlayedCards().push_back(shared_from_this());

    if(face_up){
        bool hasValidTarget=false;
        const auto& allPlayers = gameState.getAllPlayers();
        for(Player* currentPlayer : allPlayers){
            if(currentPlayer->getPlayerID()!=player->getPlayerID() && currentPlayer->getHandCards().size()>=2){
                hasValidTarget=true;
                break;
            }
        }
        //不存在满足条件的玩家，技能直接无效
        if (!hasValidTarget) {
           gameState.sendErrorMessage(player->getPlayerID(), "没有其他玩家有至少两张手牌，无法执行交换。");
           return;
        }

        while(true){
            string targetPlayerID = player->chooseTargetPlayerID(gameState);
            Player* targetPlayer = gameState.getPlayer(targetPlayerID);
            
            //选择了自己，重新选择玩家
            if(targetPlayer==player){
                gameState.sendErrorMessage(player->getPlayerID(), "不能选择自己！");
                continue;
            }
            
            //选择了非法玩家，重新选择玩家
            if(targetPlayer->getHandCards().size()<2){
                gameState.sendErrorMessage(player->getPlayerID(), "目标玩家手牌不足两张！");
                continue;
            }
            
            //选择了合法玩家，进入下一层判定循环
            while(true){
                card_pointer playerCard = player->chooseHandCard(gameState);
                if(playerCard->getType()==ANSWER){
                    gameState.sendErrorMessage(player->getPlayerID(), "不能交换谜底牌！");
                    continue;
                }

                card_pointer targetPlayerCard = targetPlayer->chooseHandCard(gameState);
                if(targetPlayerCard->getType()==ANSWER){
                    gameState.sendErrorMessage(targetPlayer->getPlayerID(),"不能交换谜底牌！");
                    continue;
                }

                if(!playerCard||!targetPlayerCard){
                    gameState.sendErrorMessage(player->getPlayerID(), "交换失败！");
                    gameState.sendErrorMessage(targetPlayer->getPlayerID(), "交换失败！");
                    continue;
                }
                
                //执行交换逻辑
                player->removeHandCard(playerCard);
                targetPlayer->removeHandCard(targetPlayerCard);
                player->addHandCard(targetPlayerCard);
                targetPlayer->addHandCard(playerCard);
                break;
            }

           break;
        }


    }

    else{
        this->setMysteryPoints(1);
    }
}