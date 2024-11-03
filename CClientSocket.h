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
public slots:
    void sendLoginRequest(const QString& username, const QString& password);
    void sendStartGameRequest();
    void sendRegisterRequest(const QString& username, const QString& password);
    void sendCreateRoomRequest(int capacity);
    void sendJoinRoomRequest(const QString& roomCode);
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
private:
    QString strServerIP;
    quint16 iServerPort;
    int          iConnSeconds;
    int          iTimerId;
    void processJsonObject(const QJsonObject &jsonObj);

};

#endif // CCLIENTSOCKET_H
