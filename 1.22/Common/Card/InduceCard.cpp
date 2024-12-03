#include "InduceCard.h"
#include "../Player/Player.h"
#include "../GameState/GameState.h"
#include <QString>
#include "Card.h"
//InduceCard::play

/**
 * @brief 触发“诱”牌的效果。
 * 具体情况
 * 暗置打出：触发暗置的统一效果，详见AnswerCard和CoverCard
 * 明置打出：下家的下一张牌只能暗置打出。
 * 不合法情况：1.明置打出时无法找到下家。
 *           2.明置打出时下家已经没有手牌，即自己是最后一个出完牌的。
 * @param player 玩家指针类型变量，表示打出这张牌的玩家。
 * @param gameState GameState类变量，用以表示当前的游戏状态和数据
 * @param faceUp  布尔类型变量，表示卡牌的明暗置打出。
 */
void InduceCard::play(Player* player, GameState& gameState, bool faceUp) {
    this->setFaceUp(faceUp);
    // 获取下家
    Player* nextPlayer = gameState.getNextPlayer(player);

    if(this->getFaceUp() == true) {
        if (!nextPlayer) {
            gameState.sendErrorMessage(player->getPlayerID(), "无法找到下家。");
            this->setValidPlay(false);
            return;
        }
        if(nextPlayer->getHandCards().empty()) {
            gameState.sendErrorMessage(player->getPlayerID(), "下家已没有手牌！");
            this->setValidPlay(false);
            return;
        }
        player->getPlayedCards().push_back(shared_from_this());
        gameState.setCanNormalPlay(nextPlayer->getPlayerID(), false);
    } else {
        player->getPlayedCards().push_back(shared_from_this());
        this->setMysteryPoints(1);
    }
}

void InduceCard::trigger(Player* player, GameState& gameState, bool faceUp) {
    Player* nextPlayer = gameState.getNextPlayer(player);
    if(!nextPlayer || nextPlayer->getHandCards().empty()) {
        return;
    } else {
        this->play(player, gameState, true);
    }
}
