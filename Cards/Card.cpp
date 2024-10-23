#include "GameState.h"
#include "Player.h"
#include "Card.h"
#include <string>
#include <memory>
using namespace std;
class Card{
   public:
       Card(CardType type, string e) {
          card_type=type;
          effect=e;
       }

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


    protected:
         CardType card_type;
         std::string effect;
         bool face_up;  // to mark is a card is facing up
         bool has_trigger_effect;  // to meet the requirment of "解" and "匿"
         int mystery_point=0;
         bool is_last_face_down;
       
};
