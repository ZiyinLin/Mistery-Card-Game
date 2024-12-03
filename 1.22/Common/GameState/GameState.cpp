#include "GameState.h"
#include <QMetaObject>

// Qt headers
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QQueue>

// Project headers
#include "../Player/Player.h"
#include "../Card/Card.h"
#include "../GameEvent/GameEvent.h"
using namespace std;
using card_pointer = std::shared_ptr<Card>;
void GameState::setState(State stateType) {
    state_type = stateType;
}

State GameState::getState() {
    return state_type;
}

void GameState::appendPlayer(Player* player) {
    players.push_back(player);
}

std::vector<Player*> GameState::getAllPlayers() {
    return players;
}

void GameState::setEvents(QQueue<GameEvent> events) {
    this->events = events;
}

QQueue<GameEvent> GameState::getEvents() {
    return events;
}

void GameState::setCanNormalPlay(const QString playerID, bool normal) {
    for(auto player : players) {
        if(player->getPlayerID() == playerID) {
            player->setCanNormalPlay(normal);
            break;
        }
    }
}

void GameState::sendErrorMessage(const QString playerID, QString error) {
    QJsonObject json;
    json["type"]="error_message";
    json["player_id"]=playerID;
    json["error"]=error;
    emit errorMessage(json);
}

QJsonObject GameState::toJson() const {
    QJsonObject json;
    json["type"] = "game_state";
    json["state"] = static_cast<int>(state_type);
    
    QJsonArray playersArray;
    for(auto player : players) {
        playersArray.append(player->toJson());
    }
    json["players"] = playersArray;
    
    QJsonArray eventsArray;
    QQueue<GameEvent> eventsCopy = events;
    while(!eventsCopy.isEmpty()) {
        GameEvent event = eventsCopy.dequeue();
        QJsonObject eventObj;
        eventObj["type"] = event.getEventType();
        eventObj["data"] = event.getEventData();
        eventsArray.append(eventObj);
    }
    json["events"] = eventsArray;
    
    return json;
}

GameState* GameState::fromJson(const QJsonObject& json) {
    GameState* state = new GameState();
    state->setState(static_cast<State>(json["state"].toInt()));
    
    QJsonArray playersArray = json["players"].toArray();
    for(const auto& playerValue : playersArray) {
        if(playerValue.isObject()) {
            QJsonObject playerObj = playerValue.toObject();
            Player* player = Player::fromJson(playerObj);
            state->appendPlayer(player);
        }
    }
    
    QJsonArray eventsArray = json["events"].toArray();
    QQueue<GameEvent> newEvents;
    for(const auto& eventValue : eventsArray) {
        if(eventValue.isObject()) {
            QJsonObject eventObj = eventValue.toObject();
            GameEvent event;
            event.setEventType(eventObj["type"].toString());
            event.setEventData(eventObj["data"].toObject());
            newEvents.enqueue(event);
        }
    }
    state->setEvents(newEvents);
    
    return state;
}

Player* GameState::getCurrentPlayer() {
    return players.empty() ? nullptr : players[0];
}

Player* GameState::getNextPlayer(Player* player) {
    if(players.empty()) return nullptr;
    
    auto it = std::find(players.begin(), players.end(), player);
    if(it == players.end() || ++it == players.end()) {
        return players.front();
    }
    return *it;
}

Player* GameState::getPlayer(const QString playerID) {
    for(auto player : players) {
        if(player->getPlayerID() == playerID) {
            return player;
        }
    }
    return nullptr;
}

GameState::GameState(QObject* parent) : QObject(parent), state_type(INIT) {
    players.clear();
    events.clear();
}





