#include "Card.h"
#include <string>
#include <vector>
#include "Player.h"
#include <algorithm>
using namespace std;

class Player{
    public:

      void drawCard(card_pointer card){

      }

      void playCard(card_pointer card, GameState& gameState, bool faceUp){
           card->setFaceUp(faceUp);
           this->removeHandCard(card);
           this->addPlayedCard(card);
      }

      void discardCard(card_pointer card){
           this->removeHandCard(card);
           this->addDiscardedCard(card);
      }

      void addHandCard(card_pointer card){
           hand_cards.push_back(card);
      }

      void removeHandCard(card_pointer card){
           for(vector<card_pointer>::iterator it=hand_cards.begin();it!=hand_cards.end();){
               if(*it==card){
                  it=hand_cards.erase(it);
                  break;
              }else{
                  ++it;
              }
           }
      }

      void addPlayedCard(card_pointer card){
           played_cards.push_back(card);
      }

      void addPlayedCardAt(card_pointer card, int index){
           played_cards.insert(played_cards.begin()+index, card);
      }

      void removePlayedCard(card_pointer card){
           for(vector<card_pointer>::iterator it=played_cards.begin();it!=played_cards.end();){
               if(*it==card){
                  it=played_cards.erase(it);
                  break;
              }else{
                  ++it;
              }
           }
      }

      void addDiscardedCard(card_pointer card){
            discarded_cards.push_back(card);
      }

      void removeDiscardedCard(card_pointer card){
            for(vector<card_pointer>::iterator it=discarded_cards.begin();it!=discarded_cards.end();){
               if(*it==card){
                  it=discarded_cards.erase(it);
                  break;
              }else{
                  ++it;
              }
           }
      }

      vector<card_pointer> getHandCards(){
          return hand_cards;
      }

      vector<card_pointer> getPlayedCards(){
          return played_cards;
      }

      vector<card_pointer> getDiscardedCards(){
          return discarded_cards;
      }

      void setCanNormalPlay(bool canNormal){
          can_normal_play=canNormal;
      }
      
      bool getCanNormalPlay(){
          return can_normal_play;
      }

      void setWin(bool win){
          is_win=win;
      }

      bool getWin(){
          return is_win;
      }

      void setConnected(bool isConnected){
          is_connected=isConnected;
      }

      bool getConnected(){
          return is_connected;
      }

      int getMysteries(){
          int count=0;
          for(card_pointer card : played_cards){
             count+=card->getMysteryPoints();
          }
          return count;
      }

      bool chooseFaceUp(GameState& gameState){

      }

      card_pointer chooseHandCard(GameState& gameState){

      }

      card_pointer chooseTargetCard(GameState& gameState){

      }

      string chooseTargetPlayerID(GameState& gameState){

      }
    
    private:
       string player_id;
       string player_nickname;
       vector<card_pointer> hand_cards;//手牌
       vector<card_pointer> played_cards;//出牌
       static vector<card_pointer> discarded_cards;//公共的弃牌堆
       bool is_connected;
       bool can_normal_play;
       bool is_win;
};