#include "GameState.h"
#include "Player.h"
#include "Card.h"
#include <string>
#include <memory>
using namespace std;

class Card{
   public:

       void setFaceUp(bool faceUp){
          face_up=faceUp;
       }
       bool getFaceUp(){
          return face_up;
       }

       CardType getType(){
        return card_type;
       }

       void reveal(){
          face_up=true;
       }

       int getMysteryPoints(){
          return mystery_point;
       };
       void setMysteryPoints(int points){
          mystery_point=points;
       };

       void setLastFaceDown(bool lastFaceDown){
          is_last_face_down=lastFaceDown;
       }
       bool getLastFaceDown(){
          return is_last_face_down;
       }

       void setHasTrigger(bool hasTrigger){
         has_trigger_effect=hasTrigger;
       }
       bool getHasTrigger(){
         return has_trigger_effect;
       }

       void setValidPlay(bool validPlay){
          valid_play=validPlay;
       }
       bool getValidPlay(){
          return valid_play;
       }


        nlohmann::json toJson() const {
           nlohmann::json j;
           j["card_type"] = static_cast<int>(card_type);
           j["effect"] = effect;
           j["face_up"] = face_up;
           j["has_trigger_effect"] = has_trigger_effect;
           j["is_last_face_down"] = is_last_face_down;
           j["valid_play"] = valid_play;
           j["mystery_point"] = mystery_point;
           return j;
       }

       static card_pointer fromJson(const nlohmann::json& j) {
           auto card = std::make_shared<Card>(
               static_cast<CardType>(j.at("card_type").get<int>()),
               j.at("effect").get<std::string>()
           );
           card->setFaceUp(j.at("face_up").get<bool>());
           card->setHasTrigger(j.at("has_trigger_effect").get<bool>());
           card->setLastFaceDown(j.at("is_last_face_down").get<bool>());
           card->setValidPlay(j.at("valid_play").get<bool>());
           card->setMysteryPoints(j.at("mystery_point").get<int>());
           return card;
       }

    protected:
         CardType card_type;
         std::string effect;
         bool face_up;  // to mark is a card is facing up
         bool has_trigger_effect=false;  // to meet the requirment of "解" and "匿"
         bool is_last_face_down;
         bool valid_play=true;//判断是否有效执行了play函数
         int mystery_point=0;
};
