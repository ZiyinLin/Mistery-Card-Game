#include "Card.h"
#include <string>
#include <vector>
#include "Player.h"
#include <algorithm>
using namespace std;


      void Player::drawCard(card_pointer card){

      }

      void Player::playCard(card_pointer card, GameState& gameState, bool faceUp){
           card->setFaceUp(faceUp);
           this->removeHandCard(card);
           this->addPlayedCard(card);
      }

      void Player::discardCard(card_pointer card){
           this->removeHandCard(card);
           this->addDiscardedCard(card);
      }

      void Player::addHandCard(card_pointer card){
           hand_cards.push_back(card);
      }

      void Player::removeHandCard(card_pointer card){
           for(vector<card_pointer>::iterator it=hand_cards.begin();it!=hand_cards.end();){
               if(*it==card){
                  it=hand_cards.erase(it);
                  break;
              }else{
                  ++it;
              }
           }
      }

      void Player::addPlayedCard(card_pointer card){
           played_cards.push_back(card);
      }

      void Player::addPlayedCardAt(card_pointer card, int index){
           played_cards.insert(played_cards.begin()+index, card);
      }

      void Player::removePlayedCard(card_pointer card){
           for(vector<card_pointer>::iterator it=played_cards.begin();it!=played_cards.end();){
               if(*it==card){
                  it=played_cards.erase(it);
                  break;
              }else{
                  ++it;
              }
           }
      }

      void Player::addDiscardedCard(card_pointer card){
            discarded_cards.push_back(card);
      }

      void Player::removeDiscardedCard(card_pointer card){
            for(vector<card_pointer>::iterator it=discarded_cards.begin();it!=discarded_cards.end();){
               if(*it==card){
                  it=discarded_cards.erase(it);
                  break;
              }else{
                  ++it;
              }
           }
      }

      vector<card_pointer> Player::getHandCards(){
          return hand_cards;
      }

      vector<card_pointer> Player::getPlayedCards(){
          return played_cards;
      }

      vector<card_pointer> Player::getDiscardedCards(){
          return discarded_cards;
      }

      void Player::setCanNormalPlay(bool canNormal){
          can_normal_play=canNormal;
      }

      bool Player::getCanNormalPlay(){
          return can_normal_play;
      }

      void Player::setWin(bool win){
          is_win=win;
      }

      bool Player::getWin(){
          return is_win;
      }

      void Player::setConnected(bool isConnected){
          is_connected=isConnected;
      }

      bool Player::getConnected(){
          return is_connected;
      }

      int Player::getMysteries(){
          int count=0;
          for(card_pointer card : played_cards){
             count+=card->getMysteryPoints();
          }
          return count;
      }

      bool Player::chooseFaceUp(GameState& gameState){

      }

      card_pointer Player::chooseHandCard(GameState& gameState){

      }

      card_pointer Player::chooseTargetCard(GameState& gameState){

      }

      string Player::chooseTargetPlayerID(GameState& gameState){

      }
