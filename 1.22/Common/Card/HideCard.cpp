#include "HideCard.h"
#include "../Player/Player.h"
#include "../GameState/GameState.h"
#include <QString>
#include <vector>
#include "Card.h"
using namespace std;
//HideCard::play
/**
 * @brief 触发“匿”牌的效果。
 * 具体情况
 * 暗置打出：触发暗置的统一效果，详见AnswerCard和CoverCard
 * 明置打出：此牌不能明置打出，否则会判定为不合法直接 return。
 * 出牌阶段被掀开： 一并掀开出牌堆中在它之前的所有暗置牌，不触发这些牌的效果。若其中含有谜底牌，则进入结算阶段，掀开此”匿“牌的玩家获胜。
 * 出牌阶段未被掀开：出牌阶段结束后系统将其掀开，其“谜”数变为无穷大（这里将其设为100点，效果相同：持有该牌的玩家成为拥有“谜”数最多的人）。
 * 若出牌阶段最后一张打出此牌，则不触发此效果。
 * @param player 玩家指针类型变量，表示打出这张牌的玩家。
 * @param gameState GameState类变量，用以表示当前的游戏状态和数据
 * @param faceUp  布尔类型变量，表示卡牌的明暗置打出。
 */
void HideCard::play(Player* player, GameState& gameState, bool faceUp) {
    if(gameState.getState() == PLAY) {
        this->setFaceUp(faceUp);
        // 明置打出，非法
        if(this->getFaceUp() == true) {
            gameState.sendErrorMessage(player->getPlayerID(), "不能明置打出此牌！");
            this->setValidPlay(false);
            return;
        } else {
            player->getPlayedCards().push_back(shared_from_this());
            this->setMysteryPoints(1);
        }
    }

    if(gameState.getState() == SHOW_HIDE) {
        face_up = true;
        auto playedCards = player->getHandCards();
        int index = 0;
        for(const auto& card : playedCards) {
            if(card.get() != this) {
                ++index;
            } else {
                break;
            }
        }
        if(index != 6) {
            this->setMysteryPoints(100);
        } else {
            return;
        }
    }
}

void HideCard::trigger(Player* player, GameState& gameState, bool faceUp) {
    // 获取玩家出牌列表
    const auto& playedCards = player->getPlayedCards();

    // 遍历玩家出牌列表，找到当前这张"匿"的位置
    for(auto it = playedCards.begin(); it != playedCards.end(); ++it) {
        if(it->get() == this) {
            // 这张"匿"之前所有卡牌都设置为face_up=true
            for(auto priorCardIt = playedCards.begin(); priorCardIt != it; ++priorCardIt) {
                (*priorCardIt)->setFaceUp(true);
                (*priorCardIt)->setHasTrigger(true);
                // 若有牌是"谜底"，直接进入结算阶段
                CardType type = (*priorCardIt)->getType();
                if(type == ANSWER) {
                    gameState.setState(SETTLE);
                }
            }
            break;  // 找到并处理完"匿"，退出循环
        }
    }
}
