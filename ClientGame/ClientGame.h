#ifndef CLIENTGAME_H
#define CLIENTGAME_H

#include "Card.h"
#include "GameState.h"
#include "PlayerAction.h"
#include "Player.h"
#include <string>
#include <QTcpSocket>

class ClientGame{
    public:
        ClientGame(const std::string& serverUri);
        void connectToServer();//连接到服务器
        void sendAction(PlayerAction& action);//发送动作
        void receiveUpdates();//接收更新
        void setGameState(State state);//设置游戏状态
        void updateGameState();//更新游戏状态   
        void updateHandCards();//更新手牌
        void updatePlayerHand(const std::string& playerId, card_pointer card);//更新其他玩家的手牌展示  
        void updatePlayerPlayed(const std::string& playerId, card_pointer card, bool isFaceUp);//更新其他玩家的出牌展示
        void updatePlayedCards();//更新自己的出牌展示
        void flipCard(const std::string& playerId, card_pointer card);//翻牌动画 
        void displayWinner();//显示胜利者
        bool isValidInput(Player* player, card_pointer card);//检验合法性
        State getGameState();//获取游戏状态
        Player* getPlayer();//获取玩家
        std::string getMyId();//获取玩家ID  
    
    private:
        std::string serverUri;//服务器地址
        QTcpSocket* tcpSocket;//TCP套接字
        GameState my_game_state;//游戏状态
        Player* my_player;//玩家
        void onMessageReceived(const std::string& message);//接收消息


}

#endif
