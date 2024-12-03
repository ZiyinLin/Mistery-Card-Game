#ifndef GAMEROOM_H
#define GAMEROOM_H

#include <QObject>
#include <QTcpSocket>
#include <QList>
#include <QTimer>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QIODevice>
#include <memory>
#include "../../Server/Game/ServerGame/ServerGame.h"
#include "../../Common/PlayerAction/PlayerAction.h"
#include <QThread>
#include <QCoreApplication>

// 前向声明
class ServerGame;
class PlayerAction;

// 重命名枚举以避免冲突
enum class RoomState {
    WAITING,    // 等待玩家加入
    STARTING,   // 游戏即将开始
    PLAYING,    // 游戏进行中
    PAUSED,     // 游戏暂停
    ENDED       // 游戏结束
};

class GameRoom : public QObject {
    Q_OBJECT
public:
    explicit GameRoom(QObject *parent = nullptr, int capacity = 4, QString roomCode = QString());
    virtual ~GameRoom();
    
    // 基本房间操作
    int getCapacity() const;
    bool isFull() const;
    void addPlayer(QTcpSocket* playerSocket);
    bool removePlayer(QTcpSocket* playerSocket);
    bool kickPlayer(QTcpSocket* playerSocket);
    QTcpSocket* getOwner() const;
    QList<QTcpSocket*> getPlayers() const;
    QList<QTcpSocket*> getSpectators() const;
    QString getRoomCode() const;
    QStringList getPlayerNames() const;
    void updateRoomStatus();

    // 游戏状态管理
    bool isGameStarted() const { return m_gameStarted; }
    int getCurrentRound() const { return roundNumber; }
    QTcpSocket* getCurrentPlayer() const { return players.value(currentPlayerIndex); }
    
    // 游戏操作
    bool startGame();
    void endGame();
    void pauseGame();
    void resumeGame();

    // 游戏状态保存和加载
    void saveGameState();
    bool loadGameProgress(const QString& filename);
    void saveGameProgress();
    bool saveGameState(const QString& filename);
    bool loadGameState(const QString& filename);

    bool areAllPlayersReady() const;
    bool isPlayerReady(QTcpSocket* player) const;

    ServerGame* getGameInstance() { return gameInstance.get(); }

    void broadcastMessage(const QByteArray& message) {
        for (auto socket : players) {
            if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                socket->write(message);
                socket->flush();
            }
        }
    }

public slots:
    void broadcastToRoom(const QJsonObject& message);
    void setPlayerReady(QTcpSocket* player, bool ready);
    void startGameLogic();
    void handlePlayerReady(QTcpSocket* playerSocket);
    void handlePlayerAction(const QJsonObject& actionJson, QTcpSocket* socket);
    void checkGameStart();
    void setOwner(QTcpSocket* ownerSocket);
    void addSpectator(QTcpSocket* spectatorSocket);
    void removeSpectator(QTcpSocket* socket);

signals:
    void gameStartedSignal();
    void gameEndedSignal();
    void turnChanged(const QString& playerName);
    void playerJoined(const QString& playerName);
    void playerLeft(const QString& playerName);
    void playerKicked(const QString& playerName);
    void turnTimeout(const QString& playerName);
    void gameStateChanged(RoomState newState);
    void roundStarted(int roundNumber);
    void roundEnded(int roundNumber);
    void scoreUpdated(const QString& playerName, int newScore);
    void playerActionReceived(const PlayerAction& action);

private slots:
    void handleTurnTimeout();
    void nextTurn();
    void onGameStateUpdated(const QJsonObject& json);

private:
    // 房间基本信息
    int roomCapacity;
    QString roomCode;
    QList<QTcpSocket*> players;
    QList<QTcpSocket*> spectators;
    QTcpSocket* owner;
    QDateTime creationTime;
    QDateTime lastActivityTime;
    
    // 游戏状态
    bool m_gameStarted;
    int currentPlayerIndex;
    int roundNumber;
    int turnTimeLimit;
    
    // 定时器
    QTimer* turnTimer;
    QTimer* statusUpdateTimer;
    const int STATUS_UPDATE_INTERVAL = 1000;
    
    // 工具函数
    bool isPlayerTurn(QTcpSocket* player) const;
    void updateLastActivity();

    QHash<QTcpSocket*, bool> playerReadyStates;

    struct GameStats {
        int totalGamesPlayed = 0;
        int totalRoundsPlayed = 0;
        QDateTime longestGameDuration;
        QDateTime shortestGameDuration;
        QHash<QString, int> playerWins;
    };
    GameStats stats;
    
    void updateGameStats();
    QJsonObject getGameStats() const;
    void setGameState(RoomState newState);
    void startStatusUpdates();
    void stopStatusUpdates();

    std::unique_ptr<ServerGame> gameInstance;
};

#endif // GAMEROOM_H
