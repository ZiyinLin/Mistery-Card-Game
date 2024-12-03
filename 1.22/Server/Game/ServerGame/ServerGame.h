#ifndef SERVERGAME_H
#define SERVERGAME_H

#include "Card.h"
#include "GameState.h"
#include "PlayerAction.h"
#include "Player.h"
#include "GameEvent.h"
#include <QString>
#include "TcpServer.h"
#include <QList>
#include <memory>
#include <QTcpSocket>

class ServerGame : public QObject {
    Q_OBJECT
public:
    explicit ServerGame(QObject* parent = nullptr);
    virtual ~ServerGame() = default;

    void dealCards();
    void sendGameState(const GameState& gameState);
    void sendGameState(QTcpSocket* playerSocket, const GameState& gameState);
    void setState(State state);
    void setNextPlayer();
    void setCurrentPlayer(Player* player){current_player=player;}
    void flipHideCard();
    void addPointedCard(std::shared_ptr<Card> card);
    void flipPointedCards();
    void checkWinner();
    void announceWinner(Player* player, const QString& winReason);
    bool allPlayerHandEmpty();
    std::vector<Player*> getPlayers();
    Player* getCurrentPlayer();
    void addPlayer(Player* player) { players.push_back(player); }
    void addPlayerSocket(QTcpSocket* socket){playerSockets.push_back(socket);}
    void processPlayerAction(const PlayerAction& action);
    GameState& getGameState() { return game_state; }
    
    void forceDiscard(Player* player);  // 强制弃牌
    void sendDiscardResult(Player* player);  // 发送弃牌结果
    void sendHandCardsInfo(Player* player);  // 发送手牌信息

    Player* getPlayer(const QString& playerId) {
        for (Player* player : players) {
            if (player->getPlayerID() == playerId) {
                return player;
            }
        }
        return nullptr;
    }
    void handlePointOutRequest(const QString& pointingPlayerId, const QString& targetPlayerId, int cardIndex);
    void sendPointOutPhaseInfo();
    void handlePointOutPhaseEnd();
    void returnToRoom();


public slots:
    void startGameLogic();
    void onPlayerActionReceived(const PlayerAction& action);
    void receivePlayerAction(const QByteArray& data, QTcpSocket* playerSocket);
    void handleDealCard();
    void handleDiscardCard();
    void handlePlayCard();
    void handleShowHide();
    void handlePointOut();
    void handleFlipCard();
    void handleSettle();
    void handleRestart();
    void handleDiscardPhase();
    void processPlayerDiscard(Player* player, int cardId, int count);

signals:
    void gameStateChanged(const QJsonObject& state);
    void gameStarted();
    void gameEnded();
    void turnChanged(const QString& playerName);
    void roundStarted(int roundNumber);
    void roundEnded(int roundNumber);
    void gameReturningToRoom();

private:
    std::vector<std::shared_ptr<Card>> deck;
    std::vector<std::shared_ptr<Card>> pointedCards;
    std::vector<Player*> players;
    GameState game_state;
    QList <QTcpSocket*> playerSockets;
    Player* current_player;
    QMap<QTcpSocket*, QByteArray> receivedActions;
    QMap<QString, bool> playerPointedStatus;  // 记录每个玩家是否已经指认过
    QMap<QString, int> playerMysteryPoints;
    int pointed_count;
    bool processPlayerActionForPlayer(Player* player);
    
    QString getCardTypeName(CardType type);
    int getCardID(CardType type);
    
    QTcpSocket* findPlayerSocket(Player* player) {
        for (QTcpSocket* socket : playerSockets) {
            if (socket->property("username").toString() == player->getPlayerID()) {
                return socket;
            }
        }
        return nullptr;
    }
};


#endif // SERVERGAME_H
