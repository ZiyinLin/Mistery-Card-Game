#ifndef GAMESTATE_H
#define GAMESTATE_H
#include "Player.h"
#include "Card.h"
#include "Message.h"
#include "PlayerAction.h"
#include <string>


enum State{
    INIT,//初始化游戏
    SEND_CARD,//发牌
    DISCARD_CARD,//弃牌
    PLAY_CARD,//出牌
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

          // 序列化 PlayerAction
          Message serializeAction(const PlayerAction& action);

          // 反序列化 PlayerAction
          PlayerAction deserializeAction(const Message& message);

          nlohmann::json toJson() const;
          static GameState fromJson(const nlohmann::json& j);

          Message toMessage() const;
          static GameState fromMessage(const Message& message);

    private:
          State state_type;
          const std::vector<Player*> players;
    
};

#endif