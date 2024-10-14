#ifndef PLAYER_H
#define PLAYER_H

#include "Card.h"
#include <string>
#include <vector>

class Player{
public:
    Player(const std::string& id, const std::string& nickname):
    player_id(id), player_nickname(nickname){};

    std::string& getPlayerID();
    std::string& getNickname();
    
    /* player's action using new added smart pointer
     * it has been added in Card class
     * however the display of hand cards and played cards
     * are controled by GameState instead of here
     */  
    void drawCard(card_pointer card);
    void playCard(card_pointer card, GameState& gameState, bool face_up);
    void discardCard(card_pointer card);

private:
    std::string player_id;
    std::string player_nickname;
    std::vector<card_pointer> hand_cards;
    std::vector<card_pointer> played_cards;
    bool is_connected;
};
#endif