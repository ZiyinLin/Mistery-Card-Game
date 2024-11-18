#include "Card.h"
#include "Message.h"
#include "PlayerAction.h"
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
using namespace std;

         //变成JSON格式后用Message封装
         Message PlayerAction::toMessage(){
             QJsonObject jsonObj;
             jsonObj["action_type"] = static_cast<int>(action_type);  // 使用枚举类型时，需要转换为整数
             jsonObj["player_id"] = player_id;
             jsonObj["card"] = card ? card->toJson() : nullptr;
             jsonObj["target_player_id"] = target_player_id;
             jsonObj["target_card"] = target_card ? target_card->toJson() : nullptr;
             QJsonDocument doc(jsonObj);
             return Message("PlayerAction", doc.toJson(QJsonDocument::Compact));
         }

         PlayerAction PlayerAction::fromMessage(Message& message){
             if (message.getType() == "PlayerAction") {
                        QJsonDocument doc = QJsonDocument::fromJson(message.getContent().toUtf8());
                        QJsonObject jsonObj = doc.object();
                    return PlayerAction(
                        static_cast<Action>(jsonObj["action_type"].toInt()),
                        jsonObj["player_id"].toString().toStdString(),
                        jsonObj["card"].toObject().isEmpty() ? nullptr : Card::fromJson(jsonObj["card"].toObject()),
                        jsonObj["target_player_id"].toString().toStdString(),
                        jsonObj["target_card"].toObject().isEmpty() ? nullptr : Card::fromJson(jsonObj["target_card"].toObject()),
                    );
                    }
                    throw std::invalid_argument("Message is not a PlayerAction");
         }

         Action PlayerAction::getActionType(){
            return action_type;
         }

         QString PlayerAction::getPlayerId(){
            return player_id;
         }

         card_pointer PlayerAction::getCard(){
            return card;
         }

         QString PlayerAction::getTargetPlayerID(){
            return target_player_id;
         }

         card_pointer PlayerAction::getTargetCard(){
            return target_card;
         }




