#include "Card.h"
#include "ClientGame.h"
#include "GameState.h"
#include "PlayerAction.h"
#include "Player.h"
#include "ServerGame.h"
#include <string>
#include <QTcpSocket>
#include <iostream>
#include <algorithm>
using namespace std;

class ClientGame{
    public:
        //连接到服务器
        void connectToServer(){
           tcpSocket->connectToHost(QString::fromStdString(serverUri), 9002);
           if (!tcpSocket->waitForConnected(3000)) {
               qDebug() << "Connection failed";
           }
        }
        
        //将PlayerAction转换为字节数据并发送到服务器
        void sendAction(const PlayerAction& action){
            Message message = action.toMessage();
            QByteArray data = QByteArray::fromStdString(message.getContent());
            tcpSocket->write(data);
        }

        void receiveUpdates(){
            //处理接收到的更新在onMessageReceived中
            if (tcpSocket->bytesAvailable() > 0) {
                QByteArray data = tcpSocket->readAll();
                std::string jsonString = data.toStdString();
                onMessageReceived(jsonString);
            }
        }


        void setGameState(State state){
            my_game_state.setState(state);
        }

        void updateGameState(){
            //更新游戏状态
            QByteArray data = tcpSocket->readAll();
            std::string jsonString = data.toStdString();
            Message message("GameState", jsonString);

            // 解析消息并更新游戏状态
            GameState newState = GameState::fromMessage(message);
            my_game_state = newState;
        }
        
        void updatePlayerHand(const std::string& playerId, card_pointer card){
            //更新其他玩家的手牌展示
        }

        void updatePlayerPlayed(const std::string& playerId, card_pointer card, bool isFaceUp){
            //更新其他玩家的出牌展示
        }

        void updateHandCards(){
            //更新自己的手牌展示
        }

        void updatePlayedCards(){
            //更新自己的出牌展示
        }

        void flipCard(const std::string& playerId, card_pointer card){
            //翻牌动画
        }

        void displayWinner(){
            //显示胜利者
        }

        bool isValidInput(Player* player, card_pointer card){
            //检验合法性(玩家是否拥有该牌)
            if(find(player->getHandCards().begin(),player->getHandCards().end(),card)==player->getHandCards().end()){
                return false;
            }
            return true;
        }

        State getGameState(){
            return my_game_state.getState();
        }

        Player* getPlayer(){
            return my_player;
        }

        std::string getMyId(){
            return my_player->getPlayerID();
        }
    
    private:
        std::string serverUri;
        QTcpSocket* tcpSocket;
        GameState my_game_state;
        Player* my_player;

        //处理接收到的消息（PlayerAction）
        void onMessageReceived(const std::string& message){
            Message msg("PlayerAction",message);
            PlayerAction action = PlayerAction::fromMessage(msg);

            switch(action.getActionType()){
                case Action::PLAY_CARD:
                  updatePlayerHand(action.getPlayerId(), action.getCard());
                  updatePlayerPlayed(action.getPlayerId(), action.getCard(),action.getFaceUp());
                  break;
                
                case Action::POINT_OUT_CARD:
                  flipCard(action.getPlayerId(),action.getCard());
                  break;

                case Action::SELECT_ORIENTED_CARD:
                  if(action.getCard()->getType()==SOLVE){
                    flipCard(action.getTargetPlayerID(),action.getTargetCard());
                  }
                  break;

                

            }

        }

}