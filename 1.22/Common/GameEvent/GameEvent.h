#ifndef GAMEEVENT_H
#define GAMEEVENT_H

#include <QString>
#include <QJsonObject>
#include <QMetaType>

class GameEvent {
public:
    GameEvent() = default;
    GameEvent(const GameEvent&) = default;
    GameEvent& operator=(const GameEvent&) = default;
    ~GameEvent() = default;

    void setEventType(QString type);
    QString getEventType() const;
    void setEventData(const QJsonObject& json);
    QJsonObject getEventData() const;

private:
    QString event_type;
    QJsonObject event_data;
};

Q_DECLARE_METATYPE(GameEvent)

#endif // GAMEEVENT_H
