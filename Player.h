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
    
private:
    std::string player_id;
    std::string player_nickname;
    std::vector<Card> hand_cards;
    std::vector<Card> played_cards;
    bool is_connected;
};
#endif