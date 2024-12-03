#include "TrapCard.h"
#include "../Player/Player.h"
#include "../GameState/GameState.h"
#include <QString>
#include <vector>
#include "Card.h"
using namespace std;

//TrapCard::play
/**
 * @brief 触发“陷”牌的效果。
 * 具体情况
 * 暗置打出：触发暗置的统一效果，详见AnswerCard和CoverCard
 * 明置打出：此牌无法明置打出，否则 会判定为非法，直接 return。
 * 出牌阶段被掀开：此牌的“谜”数设为3
 * @param player 玩家指针类型变量，表示打出这张牌的玩家。
 * @param gameState GameState类变量，用以表示当前的游戏状态和数据
 * @param faceUp  布尔类型变量，表示卡牌的明暗置打出。
 */
void TrapCard::play(Player* player, GameState& gameState, bool faceUp){
    this->setFaceUp(faceUp);

    if(this->getFaceUp()==true){
        gameState.sendErrorMessage(player->getPlayerID(),"不能明置打出此卡牌！");
        this->setValidPlay(false);
        return;
    }
    else{
        player->getPlayedCards().push_back(shared_from_this());
        this->setMysteryPoints(1);
    }
}

void TrapCard::trigger(Player* player, GameState& gameState, bool faceUp){
    this->setMysteryPoints(3);
}
