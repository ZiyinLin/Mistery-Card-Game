#include "Player.h"
#include "Card.h"
#include <string>
#include "GameState.h"
using namespace std;

class GameState{
    public:
       GameState(){

       }

       void setState(State stateType){
          state_type=stateType;
       }

       State getState(){
          return state_type;
       }

       vector<Player*> getAllPlayers(){
          return players;
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

    private:
       State state_type;
       const std::vector<Player*> players;

};