#include "TradeCard.h"
#include "../Player/Player.h"
#include "../GameState/GameState.h"
#include <QString>
#include <vector>
#include <algorithm>
#include "Card.h"
using namespace std;
//TradeCard::play
/**
 * @brief 触发“易”牌的效果。
 * 具体情况
 * 暗置打出：触发暗置的统一效果，详见AnswerCard和CoverCard
 * 明置打出：选择场上一名至少有两张手牌的其他玩家， 两人各自从手牌中选择一张牌交换（“谜底” 牌不能交换）。
 * 不合法情况：1.明置打出时没有符合条件的玩家。
 *           2.选择玩家时选择了自己或非法玩家。
 *           3.交换时有人选择了非法牌。
 * @param player 玩家指针类型变量，表示打出这张牌的玩家。
 * @param gameState GameState类变量，用以表示当前的游戏状态和数据
 * @param faceUp  布尔类型变量，表示卡牌的明暗置打出。
 */
void TradeCard::play(Player* player, GameState& gameState, bool faceUp) {
    this->setFaceUp(faceUp);

    if(this->getFaceUp() == true) {
        bool hasValidTarget = false;
        const auto& allPlayers = gameState.getAllPlayers();
        for(Player* currentPlayer : allPlayers) {
            if(currentPlayer->getPlayerID() != player->getPlayerID() && currentPlayer->getHandCards().size() >= 2) {
                hasValidTarget = true;
                break;
            }
        }
        // 不存在满足条件的玩家，技能直接无效
        if (!hasValidTarget) {
            gameState.sendErrorMessage(player->getPlayerID(), "没有其他玩家有至少两张手牌，无法执行交换。");
            this->setValidPlay(false);
            return;
        }

        player->getPlayedCards().push_back(shared_from_this());

        while(true) {
            QString targetPlayerID = player->chooseTargetPlayerID(gameState);
            Player* targetPlayer = gameState.getPlayer(targetPlayerID);

            // 选择了自己，重新选择玩家
            if(targetPlayer == player) {
                gameState.sendErrorMessage(player->getPlayerID(), "不能选择自己！");
                return;
            }

            // 选择了非法玩家，重新选择玩家
            if(targetPlayer->getHandCards().size() < 2) {
                gameState.sendErrorMessage(player->getPlayerID(), "目标玩家手牌不足两张！");
                return;
            }

            // 选择了合法玩家，进入下一层判定循环
            while(true) {
                card_pointer playerCard = player->chooseHandCard(gameState);
                if(playerCard->getType() == ANSWER) {
                    gameState.sendErrorMessage(player->getPlayerID(), "不能交换谜底牌！");
                    return;
                }

                card_pointer targetPlayerCard = targetPlayer->chooseHandCard(gameState);
                if(targetPlayerCard->getType() == ANSWER) {
                    gameState.sendErrorMessage(targetPlayer->getPlayerID(), "不能交换谜底牌！");
                    return;
                }

                if(playerCard == nullptr || targetPlayerCard == nullptr) {
                    gameState.sendErrorMessage(player->getPlayerID(), "交换失败！");
                    gameState.sendErrorMessage(targetPlayer->getPlayerID(), "交换失败！");
                    return;
                }

                // 执行交换逻辑
                player->removeHandCard(playerCard);
                targetPlayer->removeHandCard(targetPlayerCard);
                player->addHandCard(targetPlayerCard);
                targetPlayer->addHandCard(playerCard);
                break;
            }
            break;
        }
    } else {
        player->getPlayedCards().push_back(shared_from_this());
        this->setMysteryPoints(1);
    }
}

void TradeCard::trigger(Player* player, GameState& gameState, bool faceUp) {
    bool hasValidTarget = false;
    const auto& allPlayers = gameState.getAllPlayers();
    for(Player* currentPlayer : allPlayers) {
        if(currentPlayer->getPlayerID() != player->getPlayerID() && currentPlayer->getHandCards().size() >= 2) {
            hasValidTarget = true;
            break;
        }
    }
    // 不存在满足条件的玩家，技能直接无效
    if (!hasValidTarget) {
        return;
    } else {
        this->play(player, gameState, true);
    }
}
