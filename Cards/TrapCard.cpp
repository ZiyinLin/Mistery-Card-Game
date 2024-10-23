#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"
#include "TrapCard.h"
using namespace std;

void TrapCard::play(Player* player, GameState& gameState, bool faceUp){
    this->setFaceUp(faceUp);
    player->getPlayedCards().push_back(shared_from_this());

    if(face_up){
        this->setMysteryPoints(3);
    }
    else{
        this->setMysteryPoints(1);
    }
}