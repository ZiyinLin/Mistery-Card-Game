#include "Card.h"
#include "GameState.h"
#include "Player.h"
#include "Card.h"
#include <QString>
#include <QJsonObject>
#include <memory>
using namespace std;

       void Card::setFaceUp(bool faceUp){
          face_up=faceUp;
       }
       bool Card::getFaceUp(){
          return face_up;
       }

       CardType Card::getType(){
        return card_type;
       }

       int Card::getMysteryPoints(){
          return mystery_point;
       };
       void Card::setMysteryPoints(int points){
          mystery_point=points;
       };

       void Card::setLastFaceDown(bool lastFaceDown){
          is_last_face_down=lastFaceDown;
       }
       bool Card::getLastFaceDown(){
          return is_last_face_down;
       }

       void Card::setHasTrigger(bool hasTrigger){
         has_trigger_effect=hasTrigger;
       }
       bool Card::getHasTrigger(){
         return has_trigger_effect;
       }

       void Card::setValidPlay(bool validPlay){
          valid_play=validPlay;
       }
       bool Card::getValidPlay(){
          return valid_play;
       }


        QJsonObject Card::toJson() {
           QJsonObject j;
           j["card_type"] = static_cast<int>(card_type);
           j["effect"] = effect;
           j["face_up"] = face_up;
           j["has_trigger_effect"] = has_trigger_effect;
           j["is_last_face_down"] = is_last_face_down;
           j["valid_play"] = valid_play;
           j["mystery_point"] = mystery_point;
           return j;
       }

        static card_pointer fromJson(const QJsonObject& j) {
            auto card = std::make_shared<Card>(
                static_cast<CardType>(j.value("card_type").toInt()),
                j.value("effect").toString().toStdString()
            );
            card->setFaceUp(j.value("face_up").toBool());
            card->setHasTrigger(j.value("has_trigger_effect").toBool());
            card->setLastFaceDown(j.value("is_last_face_down").toBool());
            card->setValidPlay(j.value("valid_play").toBool());
            card->setMysteryPoints(j.value("mystery_point").toInt());

            return card;
        }

