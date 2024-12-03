#include "CoverCard.h"
#include "../Player/Player.h"
#include "../GameState/GameState.h"
#include <QString>
#include <vector>
#include <random>
#include "Card.h"
using namespace std;
//CoverCard::play
/**
 * @brief 触发“幌”牌的效果
 * 具体效果
 * 暗置打出：将 牌移出手牌堆加入弃牌堆， 此牌“谜”数为1
 * 明置打出：收回出牌堆中所有的暗置牌至手牌，再从手牌打回等量的暗置牌。
 * 不合法情况：明置打出时自己的出牌堆中没有暗置牌
 * @param player 玩家指针类型变量，表示打出这张牌的玩家。
 * @param gameState GameState类变量，用以表示当前的游戏状态和数据
 * @param faceUp  布尔类型变量，表示卡牌的明暗置打出。
 */
void CoverCard::play(Player* player, GameState &gameState, bool faceUp) {
    this->setFaceUp(faceUp);
    // 获取玩家出牌堆
    const auto& playedCards = player->getPlayedCards();

    // 计算出牌堆中暗置牌的数量
    int count = 0;
    for(card_pointer card : playedCards) {
        bool isFaceUp = card->getFaceUp();
        if(!isFaceUp) {
            count++;
        }
    }

    if(this->getFaceUp() == true) {
        // 出牌堆没有暗置牌，技能无法生效
        if(count == 0) {
            gameState.sendErrorMessage(player->getPlayerID(), "没有暗置牌，技能无效！");
            this->setValidPlay(false);
            return;
        }

        player->getPlayedCards().push_back(shared_from_this());

        // 收回所有暗置牌
        for(card_pointer card : playedCards) {
            if(card->getFaceUp() == false) {
                player->removePlayedCard(card);
                player->addHandCard(card);
            }
        }

        // 打出等量的暗置牌
        int remain_cover = count;
        while(remain_cover > 0) {
            card_pointer card = player->chooseHandCard(gameState);
            card->setFaceUp(false);
            player->removeHandCard(card);
            player->addPlayedCard(card);
            remain_cover--;
        }

        this->setValidPlay(true);
    } else {
        player->getPlayedCards().push_back(shared_from_this());
        this->setMysteryPoints(1);
        this->setValidPlay(true);
    }
}
//CoverCard::trigger
/**
 * @brief 触发明置时的效果
 * 函数用途：当此牌被解牌掀开时调用。若执行效果时包含不合法因素，则直接 return。
 * @param player 玩家指针类型变量，表示打出这张牌的玩家。
 * @param gameState GameState类变量，用以表示当前的游戏状态和数据
 * @param faceUp  布尔类型变量，表示卡牌的明暗置打出。
 */
void CoverCard::trigger(Player* player, GameState &gameState, bool faceUp) {
    this->setFaceUp(faceUp);
    // 获取玩家出牌堆
    const auto& playedCards = player->getPlayedCards();

    // 计算出牌堆中暗置牌的数量
    int count = 0;
    for(card_pointer card : playedCards) {
        bool isFaceUp = card->getFaceUp();
        if(!isFaceUp) {
            count++;
        }
    }

    if(count == 0) {
        return;
    } else {
        this->play(player, gameState, true);
    }
}
