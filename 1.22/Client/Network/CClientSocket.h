/******************************************************************************
                  Copyright (C), 2011-2021, GST
 ******************************************************************************
  File Name   : CClientSocket.h
  Version       : new file
  Author       :Nexito
  Create Date : 2015/5/19
  Last Modify :
  Description :Server Communication
      Input :New Socket Connection
      Output:Communication Data For CEvent
  Modify Date:
  1.date   : 2015/5/19
    author : Nexito
    content: new file
******************************************************************************/

#ifndef CCLIENTSOCKET_H
#define CCLIENTSOCKET_H
#include <QObject>
#include <QString>
#include <QTcpSocket>
#include <QMap>
#include <QHash>
#include <QBasicTimer>
#include <QTimer>
#include <QJsonObject>
#include "GameState.h"
#include "../Common/Card/Card.h"

// 添加前向声明
class ClientGame;  // 添加这一行

class CClientSocket:public QTcpSocket
{
    Q_OBJECT
signals:
    void loginSuccess();
    void loginFailed(const QString& message);
    void registrationSuccess();
    void registrationFailed(const QString& message);
    void roomCreated(const QString& roomCode);
    void roomCreationFailed(const QString& message);
    void roomJoined(const QString& roomCode);
    void roomJoinFailed(const QString& message);
    void playerReadyStateChanged(const QString& player, bool ready);
    void spectateSuccess(const QString& roomCode);
    void spectateFailed(const QString& message);
    void forceLogout(const QString& message);
    void playerKicked(const QString& player);
    void kickFailed(const QString& message);
    void roomLeft();
    void ownershipTransferred(const QString& newOwner);
    void transferFailed(const QString& message);
    void connectionLost();
    void gameStateReceived(GameState& gameState);
    void gameStateChanged(const QString& state);
    void discardPhaseComplete();         // 玩家手动弃牌达到6张
    void discardPhaseFinalComplete();    // 服务器发送最终统计结果
public slots:
    void sendLoginRequest(const QString& username, const QString& password);
    void sendStartGameRequest();
    void sendRegisterRequest(const QString& username, const QString& password);
    void sendCreateRoomRequest(int capacity);
    void sendJoinRoomRequest(const QString& roomCode);
    void sendReadyRequest(bool ready);
    void sendSpectateRequest(const QString& roomCode);
    void sendChatMessage(const QString& message);

public:
    CClientSocket	(QObject *parent = NULL, QString strIP="127.0.0.1", qint16 iPort=1000);
    ~CClientSocket	();
    void	connectToServer	 (QString ip, quint16 port );
    void    sendDataToServer(QString str);
    void	close			();
    bool    socketIsConnected();
protected:
    void   timerEvent(QTimerEvent *event);
private slots:
    void onConnected	();
    void onReadyRead	();
    void onDisConnected	();
    void sendHeartbeat();
    void sendGameAction(const QJsonObject& action);
private:
    QString strServerIP;
    quint16 iServerPort;
    int          iConnSeconds;
    int          iTimerId;
    QTimer* heartbeatTimer;
    bool isLoggedIn = false;
    bool discardResultShown = false;
    QJsonObject lastRoomStatus;
    bool isRoomStatusChanged(const QJsonObject& newStatus);
    void processJsonObject(const QJsonObject &jsonObj);
    void startHeartbeat();
    void stopHeartbeat();
    ClientGame* m_clientGame;
    QString getCardTypeName(CardType type);
    int getCardID(CardType type);
};

#endif // CCLIENTSOCKET_H
