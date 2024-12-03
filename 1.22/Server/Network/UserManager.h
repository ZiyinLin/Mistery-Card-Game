#ifndef USERMANAGER_H
#define USERMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QtSql/QSqlDatabase>
#include <QHash>
#include <QJsonObject>
#include <QThread>
#include <QCoreApplication>

class RoomManager;

class UserManager : public QObject {
    Q_OBJECT
public:
    explicit UserManager(QObject *parent = nullptr, RoomManager* rm = nullptr);
    ~UserManager();
    
    QTcpSocket* getUserSocket(const QString& username);

public slots:
    void handleLogin(const QJsonObject& jsonObj, QTcpSocket* tcpSocket);
    void handleRegistration(const QJsonObject& jsonObj, QTcpSocket* tcpSocket);
    void removeUserSocket(QTcpSocket* socket);
    void handleLoginRequest(const QJsonObject& request, QTcpSocket* socket);

signals:
    void loginSuccess(QTcpSocket* socket, const QString& username);
    void loginFailed(QTcpSocket* socket, const QString& message);
    void registrationSuccess(QTcpSocket* socket, const QString& username);
    void registrationFailed(QTcpSocket* socket, const QString& message);

private:
    void initializeDatabase();
    bool validateUser(const QString& username, const QString& password);
    QSqlDatabase db;
    QHash<QString, QTcpSocket*> userSockets;
    RoomManager* roomManager;
};

#endif // USERMANAGER_H 