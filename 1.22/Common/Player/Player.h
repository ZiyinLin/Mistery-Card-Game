#ifndef PLAYER_H
#define PLAYER_H

#include <QString>
#include <vector>
#include <memory>
#include <QJsonObject>

// 前向声明
class Card;
class GameState;

class Player {
public:
    using card_pointer = std::shared_ptr<Card>;
    
    Player(const QString& id, const QString& nickname):
        player_id(id), player_nickname(nickname), is_connected(false), 
        can_normal_play(true), is_win(false) {}

    QString& getPlayerID();
    QString& getNickname();

    void drawCard(int card);
    void playCard(std::shared_ptr<Card> card, GameState& gameState, bool faceUp);
    void discardCard(std::shared_ptr<Card> card);

    void addHandCard(std::shared_ptr<Card> card);
    void removeHandCard(std::shared_ptr<Card> card);
    void addPlayedCard(std::shared_ptr<Card> card);
    void addPlayedCardAt(std::shared_ptr<Card> card, int index);
    void removePlayedCard(std::shared_ptr<Card> card);
    void addDiscardedCard(std::shared_ptr<Card> card);
    void removeDiscardedCard(std::shared_ptr<Card> card);
    void debugCardStatus();

    std::vector<card_pointer>& getHandCards();
    const std::vector<card_pointer>& getHandCards() const;

    std::vector<std::shared_ptr<Card>> getPlayedCards();
    std::vector<std::shared_ptr<Card>> getDiscardedCards();

    void setCanNormalPlay(bool canNormal);
    bool getCanNormalPlay();

    void setWin(bool win);
    bool getWin();

    void setConnected(bool isConnected);
    bool getConnected();

    int getMysteries();

    bool chooseFaceUp(GameState& gameState);
    card_pointer chooseHandCard(GameState& gameState);
    card_pointer chooseTargetCard(GameState& gameState);
    QString chooseTargetPlayerID(GameState& gameState);

    QJsonObject toJson();
    static Player* fromJson(const QJsonObject& json);
    static std::vector<std::shared_ptr<Card>> discarded_cards;

private:
    QString player_id;
    QString player_nickname;
    std::vector<std::shared_ptr<Card>> hand_cards;
    std::vector<std::shared_ptr<Card>> played_cards;

    bool is_connected;
    bool can_normal_play;
    bool is_win;
};

#endif // PLAYER_H
