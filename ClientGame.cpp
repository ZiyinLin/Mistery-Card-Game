#include "Card.h"
#include "ClientGame.h"
#include "GameState.h"
#include "PlayerAction.h"
#include "Player.h"
#include "ServerGame.h"
#include <QString>
#include <QThread>
#include <QDebug>
#include <QTcpSocket>
#include <QAbstractSocket>
#include <iostream>
#include <algorithm>
using namespace std;


        //连接到服务器
        void ClientGame:: connectToServer(QString ip, quint16 port){
            tcpSocket->connectToServer(ip, port);
        }
        
        void ClientGame::disconnectFromServer(){
            tcpSocket->close();
        }

        //将PlayerAction转换为字节数据并发送到服务器
        void ClientGame::sendDataToServer(QString str){
           return tcpSocket->sendDataToServer(str);
        }

        void ClientGame::receiveUpdates(){
            //处理接收到的更新在onMessageReceived中
            if (tcpSocket->bytesAvailable() > 0) {
                QByteArray data = tcpSocket->readAll();
                std::string jsonString = data.toStdString();
                onMessageReceived(jsonString);
            }
        }


        void ClientGame::setGameState(State state){
            my_game_state.setState(state);
        }

        void ClientGame::updateGameState(){
            //更新游戏状态
            QByteArray data = tcpSocket->readAll();
            std::string jsonString = data.toStdString();
            Message message("GameState", jsonString);

            // 解析消息并更新游戏状态
            GameState newState = GameState::fromMessage(message);
            my_game_state = newState;
        }

        void ClientGame::updatePlayerHand(const std::string& playerId, card_pointer card){
            //更新其他玩家的手牌展示
        }

        void ClientGame::updatePlayerPlayed(const std::string& playerId, card_pointer card, bool isFaceUp){
            //更新其他玩家的出牌展示
        }

        void ClientGame::updateHandCards(){
            //更新自己的手牌展示
        }

        void ClientGame::updatePlayedCards(){
            //更新自己的出牌展示
        }

        void ClientGame::flipCard(const std::string& playerId, card_pointer card){
            //翻牌动画
        }

        void ClientGame::displayWinner(){
            //显示胜利者
        }

        bool ClientGame::isValidInput(Player* player, card_pointer card){
            //检验合法性(玩家是否拥有该牌)
            if(find(player->getHandCards().begin(),player->getHandCards().end(),card)==player->getHandCards().end()){
                return false;
            }
            return true;
        }

        State ClientGame::getGameState(){
            return my_game_state.getState();
        }

        Player* ClientGame::getPlayer(){
            return my_player;
        }

        QString ClientGame::getMyId(){
            return my_player->getPlayerID();
        }

        //处理接收到的消息（PlayerAction）
        void ClientGame::onMessageReceived(const std::string& message){
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

