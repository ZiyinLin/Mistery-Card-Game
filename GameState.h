#ifndef GAMESTATE_H
#define GAMESTATE_H


#include "Player.h"
#include "Card.h"
#include "Message.h"
#include "PlayerAction.h"
#include "GamestateHandler.h"
#include <QString>
#include <QJsonObject>


enum State{
    INIT,//初始化游戏
    SEND_CARD,//发牌
    DISCARD,//弃牌
    PLAY,//出牌
    SHOW_HIDE,//亮匿
    POINT_OUT,//指认
    FLIP_CARD,//翻牌
    SETTLE,//结算
    RESTART//重置游戏
};

class GameState{
    public:
         GameState(){};

          void setState(State stateType);
          State getState();

          std::vector<Player*> getAllPlayers();

          //获取当前玩家
          Player* getCurrentPlayer();

          //获取下家
          Player* getNextPlayer(Player* player);

          //通过ID找到玩家
          Player* getPlayer(std::string playerID);

          //让玩家能否明置出牌（满足“诱”的逻辑）
          void setCanNormalPlay(std::string playerID, bool normal);

          //告知玩家不合法行为或错误处理
          void sendErrorMessage(std::string playerID, std::string error);

          // 上传PlayerAction
          Message sendPlayerAction(PlayerAction& action);

          // 接收PlayerAction
          PlayerAction receivePlayerAction(const Message& message);
          // 上传、接收GameState
          Message sendGameState(GameStateHandler& gameStateHandler);
          GameStateHandler receiveGameState(const Message& message);



    private:
          State state_type;
          const std::vector<Player*> players;

};

#endif // GAMESTATE_H
