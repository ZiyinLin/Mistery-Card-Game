#include "ReverseCard.h"
#include "../Player/Player.h"
#include "../GameState/GameState.h"
#include <QString>
#include <vector>
#include "Card.h"
using namespace std;
//ReverseCard::play
/**
 * @brief 触发“溯”牌的效果。
 * 具体情况
 * 暗置打出：触发暗置的统一效果，详见AnswerCard和CoverCard
 * 明置打出：所有玩家将最近打出的一张 牌收回（对于出牌者，将 除此牌外最近打出的牌收回）。
 * @param player 玩家指针类型变量，表示打出这张牌的玩家。
 * @param gameState GameState类变量，用以表示当前的游戏状态和数据
 * @param faceUp  布尔类型变量，表示卡牌的明暗置打出。
 */
void ReverseCard::play(Player* player, GameState& gameState, bool faceUp) {
    this->setFaceUp(faceUp);
    player->getPlayedCards().push_back(shared_from_this());

    if(this->getFaceUp() == true) {
        // 获取所有玩家
        const auto& allPlayers = gameState.getAllPlayers();
        // 遍历所有玩家
        for(Player* currentPlayer : allPlayers) {
            vector<card_pointer> playedCards = currentPlayer->getPlayedCards();

            if(!playedCards.empty()) {
                if(currentPlayer == player) {
                    // 对于打出"溯"的玩家，如果有倒数第二张牌，则收回倒数第二张牌
                    if(playedCards.size() >= 2) {
                        card_pointer secondLastCard = playedCards[playedCards.size() - 2];
                        playedCards.erase(playedCards.end() - 2);
                        currentPlayer->addHandCard(secondLastCard);
                    }
                } else {
                    // 其他玩家收回最后一张牌
                    card_pointer lastPlayedCard = playedCards.back();
                    playedCards.pop_back();
                    currentPlayer->addHandCard(lastPlayedCard);
                }
            }
        }
    } else {
        this->setMysteryPoints(1);
    }
}

void ReverseCard::trigger(Player* player, GameState& gameState, bool faceUp) {
    this->play(player, gameState, faceUp);
}
