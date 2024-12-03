#ifndef REQUESTHANDLER_H
#define REQUESTHANDLER_H

#include <QRunnable>
#include <QTcpSocket>
#include <QJsonObject>
#include <QObject>
#include <QEventLoop>
#include "UserManager.h"
#include "RoomManager.h"
#include "../Network/GameRoom.h"

class RequestHandler : public QObject, public QRunnable {
    Q_OBJECT
public:
    RequestHandler(const QJsonObject& request, QTcpSocket* socket, 
                  UserManager* userManager, RoomManager* roomManager);
    void run() override;

signals:
    void processLoginRequest(const QJsonObject& request, QTcpSocket* socket);
    void processRegistrationRequest(const QJsonObject& request, QTcpSocket* socket);
    void processCreateRoomRequest(const QJsonObject& request, QTcpSocket* socket);
    void processJoinRoomRequest(const QJsonObject& request, QTcpSocket* socket);
    void processKickPlayerRequest(const QJsonObject& request, QTcpSocket* socket);
    void processTransferOwnershipRequest(const QJsonObject& request, QTcpSocket* socket);
    void processSpectateRequest(const QJsonObject& request, QTcpSocket* socket);
    void finished();

private:
    QJsonObject request;
    QTcpSocket* socket;
    UserManager* userManager;
    RoomManager* roomManager;
    QEventLoop eventLoop;
};

#endif // REQUESTHANDLER_H 