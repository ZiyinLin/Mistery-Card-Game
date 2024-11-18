#ifndef SERVERGAME_H
#define SERVERGAME_H


#include "Card.h"
#include "ClientGame.h"
#include "GameState.h"
#include "PlayerAction.h"
#include "Player.h"
#include <string>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>


class ServerGame{
    public:
        //构造函数，初始化QTcpServer对象，并连接newConnection信号到onNewConnection槽，以便处理新客户端练接
        ServerGame(){
            tcpServer = new QTcpServer(this);
            connect(tcpServer, &QTcpServer::newConnection, this, &ServerGame::onNewConnection);
        }

        void startServer();//启动服务器
        void handlePlayerAction(const PlayerAction& action);//处理玩家动作并更新游戏状态
        void dealCards();//发牌
        void broadcastAction(const PlayerAction& action);//广播动作给所有客户端
        void setState(State state);//设置游戏状态
        void broadcastGameState();//广播游戏状态
        void setNextPlayer();//设置下一个玩家
        void revealHideCard();//掀“匿”
        void addPointedCard(card_pointer card);//添加被指认的牌
        void flipPointedCards();//掀开被指认的牌
        void checkWinner();//判断获胜者
        void announceWinner();//宣布获胜者
        bool allPlayerHandEmpty(); //判断所有玩家是否都出完牌
        std::vector<Player*> getPlayers();//获取玩家
        Player* getCurrentPlayer();//获取当前玩家
        PlayerAction receivePlayerAction(); //接收玩家动作


    private:
        QTcpServer* tcpServer;
        QList<QTcpSocket*> clients;
        std::vector<card_pointer> deck;
        std::vector<card_pointer> pointedCards;
        std::vector<Player*> players;
        GameState game_state;

        void onNewConnection();//玩家连接
        void onReadyRead();//读取客户端发送的数据
        void onClientDisconnected();//玩家断开连接
};

#endif // SERVERGAME_H
