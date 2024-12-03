#include "SolveCard.h"
#include "../Player/Player.h"
#include "../GameState/GameState.h"
#include <QString>
#include <algorithm>
#include "Card.h"
using namespace std;
//SolveCard::play
/**
 * @brief 触发“解”牌的效果。
 * 具体情况
 * 暗置打出：触发暗置的统一效果，详见AnswerCard和CoverCard
 * 明置打出：选择场上任意一张其他玩家的 暗置牌，将其掀开并触发效果。
 * 不合法情况：1.明置打出时场上其他玩家没有暗置牌。
 *           2.选择卡牌时选择了非法的牌。
 * @param player 玩家指针类型变量，表示打出这张牌的玩家。
 * @param gameState GameState类变量，用以表示当前的游戏状态和数据
 * @param faceUp  布尔类型变量，表示卡牌的明暗置打出。
 */
void SolveCard::play(Player* player, GameState& gameState, bool faceUp) {
    this->setFaceUp(faceUp);

    // 先创建int类型变量，计算场上其他玩家的暗置牌数量
    int count = 0;

    const auto& allPlayers = gameState.getAllPlayers();
    // 遍历除当前玩家外的所有玩家
    for(Player* currentPlayer : allPlayers) {
        if(currentPlayer == player) {
            continue;
        }
        // 遍历每个玩家的出牌容器
        vector<card_pointer> playedCards = currentPlayer->getPlayedCards();
        for(int i = 0; i < playedCards.size(); ++i) {
            if(playedCards[i]->getFaceUp() == true) {
                count++;
            }
        }
    }

    if(this->getFaceUp() == true) {
        // 场上其他玩家没有暗置牌，收回这张牌
        if(count == 0) {
            gameState.sendErrorMessage(player->getPlayerID(), "其他玩家没有暗置牌，无法明置打出！");
            this->setValidPlay(false);
            return;
        }

        player->getPlayedCards().push_back(shared_from_this());

        while(true) {
            // 选择一张牌
            card_pointer targetCard = player->chooseTargetCard(gameState);
            Player* targetPlayer = nullptr;
            vector<Player*> allPlayers = gameState.getAllPlayers();
            // 找到持有targetCard的玩家，用于之后的合法性判断
            for(Player* currentPlayer : allPlayers) {
                vector<card_pointer> playedCards = currentPlayer->getPlayedCards();
                if(find(playedCards.begin(), playedCards.end(), targetCard) != playedCards.end()) {
                    targetPlayer = currentPlayer;
                    break;
                }
            }
            bool isFaceUp = targetCard->getFaceUp();

            // 选择了自己的牌，重新选择卡牌
            if(targetPlayer == player) {
                gameState.sendErrorMessage(player->getPlayerID(), "不能选择自己打出的卡牌！");
                continue;
            }

            if(isFaceUp) {
                gameState.sendErrorMessage(player->getPlayerID(), "请选择一张别人的暗置牌！");
                continue;
            } else {
                // 将这张暗置牌掀开，触发其trigger函数
                targetCard->trigger(targetPlayer, gameState, true);
                break;
            }
        }
    } else {
        player->getPlayedCards().push_back(shared_from_this());
        this->setMysteryPoints(1);
    }
}

void SolveCard::trigger(Player* player, GameState& gameState, bool faceUp) {
    // 先创建int类型变量，计算场上其他玩家的暗置牌数量
    int count = 0;

    const auto& allPlayers = gameState.getAllPlayers();
    // 遍历除当前玩家外的所有玩家
    for(Player* currentPlayer : allPlayers) {
        if(currentPlayer == player) {
            continue;
        }
        // 遍历每个玩家的出牌容器
        vector<card_pointer> playedCards = currentPlayer->getPlayedCards();
        for(int i = 0; i < playedCards.size(); ++i) {
            if(playedCards[i]->getFaceUp() == true) {
                count++;
            }
        }
    }

    if(count == 0) {
        return;
    } else {
        this->play(player, gameState, true);
    }
}
