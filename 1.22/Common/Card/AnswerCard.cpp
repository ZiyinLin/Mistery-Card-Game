#include "AnswerCard.h"
//AnswerCard::play
/**
 * @brief 触发“谜底”牌的效果
 * 具体效果
 * 明置打出：此牌十分重要，不能明置打出，否则 会被判定为不合法，直接return
 * 暗置打出：将卡牌移出手牌堆加入弃牌堆，且“谜”点数设为1
 * @param player 玩家指针类型变量，表示打出这张牌的玩家。
 * @param gameState GameState类变量，用以表示当前的游戏状态和数据
 * @param faceUp  布尔类型变量，表示卡牌的明暗置打出。
 */
void AnswerCard::play(Player* player, GameState &gameState, bool faceUp){
    this->setFaceUp(faceUp);
    if(this->getFaceUp()==true){
        gameState.sendErrorMessage(player->getPlayerID(),"不能明置打出此卡牌！");
        this->setValidPlay(false);
    }else{
        player->getPlayedCards().push_back(shared_from_this());
        this->setMysteryPoints(1);
        this->setValidPlay(true);
    }
}

//AnswerCard::trigger
/**
 * @brief 触发明置时的效果
 * 函数用途：当此牌被解牌掀开时调用。若执行效果时包含不合法因素，则直接 return。
 * @param player 玩家指针类型变量，表示打出这张牌的玩家。
 * @param gameState GameState类变量，用以表示当前的游戏状态和数据
 * @param faceUp  布尔类型变量，表示卡牌的明暗置打出。
 */
void AnswerCard::trigger(Player* player, GameState &gameState, bool faceUp){
    gameState.setState(SETTLE);
}
