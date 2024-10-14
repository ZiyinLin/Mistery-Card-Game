#ifndef PLAYERACTION_H
#define PLAYERACTION_H

enum Action{

};

class PlayerAction{
public:
    // diffrent cards are played diffrently, therefore we have to override play function
    virtual void play(Player* player_pointer, GameState& gameState, bool face_up);
};

#endif