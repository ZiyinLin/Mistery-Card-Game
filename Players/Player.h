#ifndef PLAYER_H
#define PLAYER_H

#include "Card.h"
#include <string>
#include <vector>

class Player{
public:
    Player(const std::string& id, const std::string& nickname):
    player_id(id), player_nickname(nickname){};

    std::string& getPlayerID();
    std::string& getNickname();
    
    /* player's action using new added smart pointer
     * it has been added in Card class
     * however the display of hand cards and played cards
     * are controled by GameState instead of here
     */  
    void drawCard(card_pointer card);
    void playCard(card_pointer card, GameState& gameState, bool faceUp);
    void discardCard(card_pointer card);

    void addHandCard(card_pointer card);//给玩家添加手牌
    void removeHandCard(card_pointer card);//移除手牌
    void addPlayedCard(card_pointer card);//添加出牌
    void addPlayedCardAt(card_pointer card, int index);//添加出牌到特定位置（应对“窃”的效果）
    void removePlayedCard(card_pointer card);//移除出牌
    void addDiscardedCard(card_pointer card);//添加弃牌
    void removeDiscardedCard(card_pointer card);//移除弃牌

    std::vector<card_pointer> getHandCards();//获取玩家手牌
    std::vector<card_pointer> getPlayedCards();//获取玩家出牌
    std::vector<card_pointer> getDiscardedCards();//获取公共的弃牌堆容器

    void setCanNormalPlay(bool canNormal);
    bool getCanNormalPlay();
    
    void setWin(bool win);
    bool getWin();

    void setConnected(bool isConnected);
    bool getConnected();

    int getMysteries();

    card_pointer chooseHandCard(GameState& gameState);//打出“幌”、“易”或“虑”后选择手牌
    card_pointer chooseTargetCard(GameState& GameState);//选择别人打出的牌
    std::string chooseTargetPlayerID(GameState& gameState);//打出“易”后选择场上其他玩家
    
    

private:
    std::string player_id;
    std::string player_nickname;
    std::vector<card_pointer> hand_cards;//手牌
    std::vector<card_pointer> played_cards;//出牌
    static std::vector<card_pointer> discarded_cards;//公共的弃牌堆
    bool is_connected;
    bool can_normal_play;
    bool is_win;
};
#endif