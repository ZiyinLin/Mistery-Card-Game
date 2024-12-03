#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <QString>
#include <QJsonObject>
#include <QQueue>
#include <vector>
#include <QObject>
#include "../GameEvent/GameEvent.h"

// Forward declarations
class Player;
class Card;
class PlayerAction;

enum State {
    INIT,
    SEND_CARD,
    DISCARD,
    PLAY,
    SHOW_HIDE,
    POINT_OUT,
    FLIP_CARD,
    SETTLE,
    RESTART
};

class GameState : public QObject {
    Q_OBJECT

public:
    explicit GameState(QObject* parent = nullptr);
    
    void setState(State stateType);
    State getState();
    void appendPlayer(Player* player);
    std::vector<Player*> getAllPlayers();
    void setEvents(QQueue<GameEvent> events);
    QQueue<GameEvent> getEvents();
    Player* getCurrentPlayer();
    Player* getNextPlayer(Player* player);
    Player* getPlayer(const QString playerID);
    void setCanNormalPlay(const QString playerID, bool normal);
    void sendErrorMessage(const QString playerID, QString error);
    QJsonObject toJson() const;
    static GameState* fromJson(const QJsonObject& json);

    // 清理事件列表的方法
    void clearEvents() { events.clear(); }
    
    // 可选：添加一个检查事件列表是否为空的方法
    bool hasEvents() const { return !events.isEmpty(); }

signals:
    void errorMessage(const QJsonObject& json);

private:
    State state_type;
    std::vector<Player*> players;
    QQueue<GameEvent> events;
};

#endif // GAMESTATE_H
