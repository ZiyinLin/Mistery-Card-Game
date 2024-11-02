#include "Player.h"
#include "Card.h"
#include <string>
#include "GameState.h"
#include "PlayerAction.h"
using namespace std;


class GameState{
    public:

       void setState(State stateType){
          state_type=stateType;
       }

       State getState(){
          return state_type;
       }

       vector<Player*> getAllPlayers(){
          return players;
       }

       Player* getCurrentPlayer(){

       }

       Player* getNextPlayer(Player* player){
          for(int i=0; i<players.size(); ++i){
             if(players[i]==player){
                 if(i<players.size()-1){
                    return players[i+1];
                    break;
                 }
                 return players[0];
                 break;
             }
          }
       }

       Player* getPlayer(string playerID){
          for(Player* player : players){
             if(player->getPlayerID()==playerID){
                 return player;
                 break;
             }
          }
       }

       void setCanNormalPlay(string playerID, bool normal){
           Player* player = this->getPlayer(playerID);
           player->setCanNormalPlay(normal);
       }

       void sendErrorMessage(string playerID, string error){

       }

       // 序列化 PlayerAction
      Message serializeAction(PlayerAction& action) {
        return action.toMessage();
    }

    // 反序列化 PlayerAction
      PlayerAction deserializeAction(const Message& message) {
        return PlayerAction::fromMessage(message) ;
    }

    // 序列化 GameState 为 JSON
    json GameState::toJson() const {
      json j;
      j["state_type"] = state_type;
      return j;
}

// 从 JSON 反序列化 GameState
GameState GameState::fromJson(const json& j) {
    GameState gameState;
    gameState.state_type = j.at("state_type").get<State>();
    return gameState;
}

// 序列化 GameState 为 Message
Message GameState::toMessage() const {
    json j = toJson();
    return Message("GameState", j.dump());
}

// 从 Message 反序列化 GameState
GameState GameState::fromMessage(const Message& message) {
    if (message.getType() == "GameState") {
        json j = json::parse(message.getContent());
        return fromJson(j);
    }
    throw std::invalid_argument("Message is not a GameState");
}

    private:
       State state_type;
       const std::vector<Player*> players;

};