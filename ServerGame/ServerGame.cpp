#include "Card.h"
#include "ClientGame.h"
#include "GameState.h"
#include "PlayerAction.h"
#include "Player.h"
#include "ServerGame.h"
#include <string>
#include <QTcpServer>
#include <QTcpSocket>
#include <algorithm>
#include <random>
#include <iostream>
using namespace std;

class ServerGame{
    public:
        
        //启动服务器监听端口
        void startServer(){
            if (!tcpServer->listen(QHostAddress::Any, 9002)) {
                qDebug() << "Server failed to start";
            } else {
                qDebug() << "Server started";
                }
            
        }

        void handlePlayerAction(const PlayerAction& action){
            //处理玩家动作并更新游戏状态
            //例如，更新gamestate，处理action
        }

        void dealCards(){
            //初始化deck
            deck.clear();
            
            //初始化12种牌，并加入到deck中
            deck.push_back(card_pointer(new Card(CardType::ANSWER, "Effect for Answer")));
            deck.push_back(card_pointer(new Card(CardType::FIRST, "Effect for First")));
            deck.push_back(card_pointer(new Card(CardType::HIDE, "Effect for Hide")));
            deck.push_back(card_pointer(new Card(CardType::REVERSE, "Effect for Reverse")));

            for(int i=0; i<3; i++){
                deck.push_back(card_pointer(new Card(CardType::THINK, "Effect for Think")));
                deck.push_back(card_pointer(new Card(CardType::TRADE, "Effect for Trade")));
            }

            for(int i=0; i<5; i++){
                deck.push_back(card_pointer(new Card(CardType::COVER, "Effect for Cover")));
                deck.push_back(card_pointer(new Card(CardType::INDUCE, "Effect for Induce")));
            }

            for(int i=0; i<6; i++){
                deck.push_back(card_pointer(new Card(CardType::SOLVE, "Effect for Solve")));
                deck.push_back(card_pointer(new Card(CardType::STEAL, "Effect for Steal")));
                deck.push_back(card_pointer(new Card(CardType::TRAP, "Effect for Trap")));
            }

            for(int i=0; i<12; i++){
                deck.push_back(card_pointer(new Card(CardType::READ, "Effect for Read")));
            }

            // 洗牌
            unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
            std::shuffle(deck.begin(), deck.end(), std::default_random_engine(seed));

            // 发牌   
            while(!deck.empty()){
                for(auto& player : players){
                    card_pointer card = deck.back();
                    deck.pop_back();
                    player->addHandCard(card);
                }
            }

            

        }
        
        //广播动作给所有客户端
        void broadcastAction(const PlayerAction& action) {
            Message message = action.toMessage();
            QByteArray data = QByteArray::fromStdString(message.getContent());
            for (QTcpSocket *clientSocket: clients) {
                if (clientSocket->state() == QAbstractSocket::ConnectedState) {
                    clientSocket->write(data);
                    clientSocket->flush();
                }
            }
        }

        void setState(State state){
            game_state.setState(state);
        }
        
        //广播游戏状态给所有客户端
        void broadcastGameState(){
            // 将 GameState 序列化为 Message
            Message message = game_state.toMessage();
            QByteArray data = QByteArray::fromStdString(message.getContent());

            // 遍历所有客户端并发送数据
            for (QTcpSocket* clientSocket : clients) {
                if (clientSocket->state() == QAbstractSocket::ConnectedState) {
                    clientSocket->write(data);
                    clientSocket->flush(); 
                }
            }
        }
    

        void setNextPlayer(){
            //设置下一个玩家
        }

        bool allPlayerHandEmpty(){
            //检查所有玩家是否手牌为空
            for(auto& player : players){
                if(!player->getHandCards().empty()){
                    return false;
                }
            }
            return true;
        }

        void revealHideCard(){
            //掀“匿”，并广播给所有客户端，让客户端执行flipcard函数
            Player* player=nullptr;
            for(auto& currentPlayer : players){
                for(auto& card : currentPlayer->getHandCards()){
                    if(card->getType() == CardType::HIDE){
                        if(card->getFaceUp()==true){
                            return;
                        }else{
                            player = currentPlayer;
                            card->setFaceUp(true);
                            card->play(player, game_state, true);
                            PlayerAction action(POINT_OUT_CARD, player->getPlayerID(), card);
                            broadcastAction(action);
                        }
                    }
                }
            }
        }
        
        
        void flipPointedCards(){
            //掀开被指认的牌,并广播给所有客户端，让客户端执行flipcard函数
            for(auto& card : pointedCards){
                card->setFaceUp(true);
                Player* player=nullptr;
                for(auto& currentplayer : players){
                    if(find(currentplayer->getPlayedCards().begin(), currentplayer->getPlayedCards().end(), card) != currentplayer->getPlayedCards().end()){
                        player = currentplayer;
                        break;
                    }
                }
                PlayerAction action(POINT_OUT_CARD, player->getPlayerID(), card);
                broadcastAction(action);
            }
        }

        void addPointedCard(card_pointer card){
            pointedCards.push_back(card);
        }
        
        //翻牌阶段判断获胜者
        void checkWinner(){
            for(auto& currentPlayer : players){
                for(auto& card : currentPlayer->getPlayedCards()){
                    if(card->getType() == CardType::ANSWER){
                        if(card->getFaceUp()==false){
                            currentPlayer->setWin(true);
                            return;
                        }
                    }
                }
            }
            
            int maxMysteries = 0;
            Player* winner = nullptr;
            for(auto& currentPlayer : players){
                if(currentPlayer->getMysteries()>maxMysteries){
                    maxMysteries = currentPlayer->getMysteries();
                    winner = currentPlayer;
                }
            }

            if(winner!=nullptr){
                winner->setWin(true);
            }
        
        }

        void announceWinner(){
            //宣布获胜者
        }

        Player* getCurrentPlayer(){
           //获取当前玩家
        }
        
        //接收玩家动作
        PlayerAction receivePlayerAction(){
            for (QTcpSocket* clientSocket : clients) {
                if (clientSocket->bytesAvailable() > 0) {
                    QByteArray data = clientSocket->readAll();
                    std::string jsonString = data.toStdString();
                    Message message("PlayerAction", jsonString);
                    return PlayerAction::fromMessage(message);
                }
            }
            throw std::runtime_error("No player action received");
        }

        //获取所有玩家
        std::vector<Player*> getPlayers(){
            return players;
        }

    private:
        QTcpServer* tcpServer;
        QList<QTcpSocket*> clients;
        std::vector<card_pointer> deck;
        std::vector<card_pointer> pointedCards;
        std::vector<Player*> players;
        GameState game_state;

        //处理新客户端连接，获取新客户端的套接字，将其添加到clients列表中，并连接readyRead和disconnected信号
        void onNewConnection() {
            QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
            clients.append(clientSocket);
            connect(clientSocket, &QTcpSocket::readyRead, this, &ServerGame::onReadyRead);
            connect(clientSocket, &QTcpSocket::disconnected, this, &ServerGame::onClientDisconnected);
    }

        //读取客户端发送的数据，qobject_cast用于获取信号发送者的套接字对象
        void onReadyRead() {
            QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
            QByteArray data = clientSocket->readAll();
            std::string jsonString = data.toStdString();
            Message message("PlayerAction", jsonString);
            PlayerAction action = PlayerAction::fromMessage(message);
            // 处理接收到的 PlayerAction
        }

        //处理客户端断开连接，从clients列表中移除断开连接的套接字，并删除它 
        void onClientDisconnected() {
            QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
            clients.removeAll(clientSocket);
            clientSocket->deleteLater();
      }

}