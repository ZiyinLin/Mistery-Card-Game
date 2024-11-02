#include "Card.h"
#include "Message.h"
#include "PlayerAction.h"
#include <string>
using namespace std;
using json = nlohmann::json;

class PlayerAction{
    public:
         json toJson() const{
            json j;
            j["action_type"] = action_type;
            j["player_id"] = player_id;
            j["card"] = card ? card->toJson() : nullptr;
            j["target_player_id"] = target_player_id;
            j["target_card"] = target_card ? target_card->toJson() : nullptr;
            return j;
       }

       static PlayerAction fromJson(const json j){
         return PlayerAction(
            j.at("action_type").get<Action>(),
            j.at("player_id").get<string>(),
            j.at("card").is_null() ? nullptr : Card::fromJson(j.at("card")),
            j.at("target_player_id").get<string>(),
            j.at("target_card").is_null() ? nullptr : Card::fromJson(j.at("target_card"))
         );}

         //变成JSON格式后用Message封装
         Message toMessage() const{
            json j = toJson();
            return Message("PlayerAction", j.dump());
         }
         
         PlayerAction fromMessage(Message& message){
            if(message.getType() == "PlayerAction"){
               json j = json::parse(message.getContent());
               return fromJson(j);
            }
            throw invalid_argument("Message is not a PlayerAction");
         }

         Action getActionType(){
            return action_type;
         }

         std::string getPlayerId(){
            return player_id;
         }

         card_pointer getCard(){
            return card;
         }

         std::string getTargetPlayerID(){
            return target_player_id;
         }

         card_pointer getTargetCard(){
            return target_card;
         }

         bool getFaceUp(){
            return face_up;
         }


    private:
       Action action_type;
       std::string player_id;          // current player's id
       card_pointer card;              // to choose your hand card
       card_pointer target_card;       // to choose other's card
       std::string target_player_id;
       bool face_up;

};