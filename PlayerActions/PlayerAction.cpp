#include "Card.h"
#include "PlayerAction.h"
#include <string>
using namespace std;

class PlayerAction{
    public:
       PlayerAction(Action a, string s){
          a=action_type;
          s=player_id;
       }

    private:
       Action action_type;
       std::string player_id;          // current player's id
       card_pointer card;              // to choose your hand card
       card_pointer target_card;       // to choose other's card
       std::string target_player_id;
       bool face_up;

};