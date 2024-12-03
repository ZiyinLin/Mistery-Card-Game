#include "UserManager.h"
#include "RoomManager.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QDebug>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QRegularExpression>
#include <QMetaObject>
#include <QDir>

UserManager::UserManager(QObject *parent, RoomManager* rm)
    : QObject(parent), roomManager(rm)
{
    initializeDatabase();
}

UserManager::~UserManager()
{
    if (db.isOpen()) {
        db.close();
    }
    QSqlDatabase::removeDatabase(db.connectionName());
}

void UserManager::initializeDatabase()
{
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, "initializeDatabase", Qt::BlockingQueuedConnection);
        return;
    }

    QString connectionName = QString("Connection-%1").arg((quintptr)QThread::currentThreadId());
    if(QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
    }

    db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    QString dbPath = QDir::currentPath() + "/users.db";
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qDebug() << "Error: Unable to open database at" << dbPath;
        qDebug() << "Database error:" << db.lastError().text();
        return;
    }

    qDebug() << "Database opened successfully at" << dbPath;

    QSqlQuery query(db);
    if (!query.exec("CREATE TABLE IF NOT EXISTS users ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "username TEXT UNIQUE, "
                   "password TEXT)")) {
        qDebug() << "Failed to create table:" << query.lastError().text();
    } else {
        qDebug() << "Table created or already exists";
    }
}

void UserManager::handleLogin(const QJsonObject& jsonObj, QTcpSocket* tcpSocket)
{
    // 确保在主线程中执行
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, "handleLogin",
                                Qt::BlockingQueuedConnection,
                                Q_ARG(QJsonObject, jsonObj),
                                Q_ARG(QTcpSocket*, tcpSocket));
        return;
    }

    QString username = jsonObj.value("username").toString();
    QString password = jsonObj.value("password").toString();
    
    QJsonObject response;
    response.insert("type", "login");
    
    // 检查用户名和密码格式
    QRegularExpression usernameRegex("^[a-zA-Z0-9_]{3,15}$");
    QRegularExpression passwordRegex("^[a-zA-Z0-9@#$%^&+=]{6,}$");
    
    if (!usernameRegex.match(username).hasMatch() || !passwordRegex.match(password).hasMatch()) {
        response.insert("status", "error");
        response.insert("message", "Invalid username or password format");
        emit loginFailed(tcpSocket, response.value("message").toString());
    } else {
        QByteArray hashedPassword = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
        QSqlQuery query(db);
        query.prepare("SELECT password FROM users WHERE username = :username");
        query.bindValue(":username", username);
        
        if (query.exec() && query.next()) {
            QByteArray storedHash = QByteArray::fromHex(query.value(0).toString().toLatin1());
            
            if (storedHash == hashedPassword) {
                // 检查该用户是否已经登录
                QTcpSocket* existingSocket = userSockets.value(username);
                if (existingSocket && existingSocket != tcpSocket) {
                    // 发送强制下线通知给旧客户端
                    QJsonObject kickNotification;
                    kickNotification.insert("type", "force_logout");
                    kickNotification.insert("message", "Your account has been logged in from another location");
                    QJsonDocument doc(kickNotification);
                    existingSocket->write(doc.toJson(QJsonDocument::Compact) + "\n");
                    
                    // 从在线用户列表中移除旧连接
                    userSockets.remove(username);
                    // 断开旧连接
                    existingSocket->disconnectFromHost();
                }
                
                // 添加新连接到在线用户列表
                userSockets[username] = tcpSocket;
                tcpSocket->setProperty("username", username);
                
                response.insert("status", "success");
                response.insert("message", "Login successful");
                response.insert("username", username);
                emit loginSuccess(tcpSocket, username);
            } else {
                response.insert("status", "error");
                response.insert("message", "Invalid username or password");
                emit loginFailed(tcpSocket, "Invalid username or password");
            }
        } else {
            response.insert("status", "error");
            response.insert("message", "Invalid username or password");
            emit loginFailed(tcpSocket, "Invalid username or password");
        }
    }
    
    QJsonDocument responseDoc(response);
    tcpSocket->write(responseDoc.toJson(QJsonDocument::Compact) + "\n");
}

void UserManager::handleRegistration(const QJsonObject& jsonObj, QTcpSocket* tcpSocket)
{
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, "handleRegistration",
                                Qt::BlockingQueuedConnection,
                                Q_ARG(QJsonObject, jsonObj),
                                Q_ARG(QTcpSocket*, tcpSocket));
        return;
    }

    db.transaction();
    try {
        QString username = jsonObj.value("username").toString().trimmed();
        QString password = jsonObj.value("password").toString().trimmed();

        QRegularExpression usernameRegex("^[a-zA-Z0-9_]{3,15}$");
        QRegularExpression passwordRegex("^[a-zA-Z0-9@#$%^&+=]{6,}$");

        QJsonObject response;
        response.insert("type", "registration");

        if (!usernameRegex.match(username).hasMatch()) {
            response.insert("status", "error");
            response.insert("message", "Username must be 3-15 characters and contain only letters, numbers, and underscores");
            emit registrationFailed(tcpSocket, response.value("message").toString());
        } else if (!passwordRegex.match(password).hasMatch()) {
            response.insert("status", "error");
            response.insert("message", "Password must be at least 6 characters and can contain letters, numbers, and special characters");
            emit registrationFailed(tcpSocket, response.value("message").toString());
        } else {
            QByteArray hashedPassword = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
            QSqlQuery query(db);
            query.prepare("INSERT INTO users (username, password) VALUES (:username, :password)");
            query.bindValue(":username", username);
            query.bindValue(":password", hashedPassword.toHex());

            if (query.exec()) {
                response.insert("status", "success");
                response.insert("message", "Registration successful");
                response.insert("username", username);
                
                tcpSocket->setProperty("username", username);
                
                userSockets[username] = tcpSocket;
                
                emit registrationSuccess(tcpSocket, username);
            } else {
                response.insert("status", "error");
                response.insert("message", "Username already exists");
                emit registrationFailed(tcpSocket, "Username already exists");
            }
        }

        QJsonDocument responseDoc(response);
        tcpSocket->write(responseDoc.toJson(QJsonDocument::Compact) + "\n");
        db.commit();
    } catch (...) {
        db.rollback();
        // 处理错误
    }
}

void UserManager::removeUserSocket(QTcpSocket* socket)
{
    QString username = socket->property("username").toString();
    if (!username.isEmpty()) {
        userSockets.remove(username);
    }
}

QTcpSocket* UserManager::getUserSocket(const QString& username)
{
    return userSockets.value(username);
}

void UserManager::handleLoginRequest(const QJsonObject& request, QTcpSocket* socket)
{
    QString username = request.value("username").toString();
    QString password = request.value("password").toString();
    
    QJsonObject response;
    response.insert("type", "login");
    
    if (validateUser(username, password)) {
        // 检查该用户是否已经登录
        QTcpSocket* existingSocket = userSockets.value(username);
        if (existingSocket && existingSocket != socket) {
            // 发送强制下线通知给旧客户端
            QJsonObject kickNotification;
            kickNotification.insert("type", "force_logout");
            kickNotification.insert("message", "Your account has been logged in from another location");
            QJsonDocument doc(kickNotification);
            existingSocket->write(doc.toJson(QJsonDocument::Compact) + "\n");
            
            // 从在线用户列表中移除旧连接
            userSockets.remove(username);
            // 可选：断开旧连接
            existingSocket->disconnectFromHost();
        }
        
        // 添加新连接到在线用户列表
        userSockets[username] = socket;
        socket->setProperty("username", username);
        
        response.insert("status", "success");
        response.insert("message", "Login successful");
    } else {
        response.insert("status", "error");
        response.insert("message", "Invalid username or password");
    }
    
    QJsonDocument doc(response);
    socket->write(doc.toJson(QJsonDocument::Compact) + "\n");
}

bool UserManager::validateUser(const QString& username, const QString& password)
{
    QSqlQuery query(db);
    query.prepare("SELECT password FROM users WHERE username = :username");
    query.bindValue(":username", username);
    
    if (query.exec() && query.next()) {
        // 获取存储的哈希密码
        QByteArray storedHash = QByteArray::fromHex(query.value(0).toString().toLatin1());
        
        // 计算输入密码的哈希值
        QByteArray inputHash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
        
        // 比较哈希值
        return storedHash == inputHash;
    }
    
    return false;  // 用户不存在或查询失败
} 