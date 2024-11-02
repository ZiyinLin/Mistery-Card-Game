#include "Card.h"
#include <string>
#include "Player.h"
#include "GameState.h"
#include "FirstCard.h"
using namespace std;

void FirstCard::play(Player* player, GameState& gameState, bool faceUp){
    this->setFaceUp(faceUp);
    player->getPlayedCards().push_back(shared_from_this());
    //无论明置还是暗置，都算1个谜
    this->setMysteryPoints(1);
}


void FirstCard::trigger(Player* player, GameState &gameState, bool faceUp){
    this->play(player, gameState, true);
}