#include <QCoreApplication>
#include <CClientSocket.h>
#include <QTextStream>
#include <QRegularExpression>

const QString RED_TEXT_START = "\033[31m";
const QString RED_TEXT_END = "\033[0m";

void handleRoomCreation(QTextStream &qtin, CClientSocket *m_pClientSocket) {
    qDebug() << "Do you want to create a game room? (yes/no):";
    QString createRoom = qtin.readLine().trimmed().toLower();
    if (createRoom == "yes") {
        qDebug() << "Enter room capacity (3-8):";
        int capacity = qtin.readLine().toInt();
        if (capacity >= 3 && capacity <= 8) {
            m_pClientSocket->sendCreateRoomRequest(capacity);
        } else {
            qDebug() << "Invalid capacity. Must be between 3 and 8.";
        }
    } else if (createRoom == "no") {
        qDebug() << "Do you want to join a game room? (yes/no):";
        QString joinRoom = qtin.readLine().trimmed().toLower();
        if (joinRoom == "yes") {
            qDebug() << "Enter room code:";
            QString roomCode = qtin.readLine().trimmed();
            m_pClientSocket->sendJoinRoomRequest(roomCode);
        }
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    CClientSocket* m_pClientSocket = new CClientSocket();
    m_pClientSocket->connectToServer("10.32.28.117", 1000);
    QTextStream qtin(stdin);
    QString username, password, action;
    bool inputValid = false;
    QRegularExpression usernameRegex("^[a-zA-Z0-9_]{3,15}$");
    QRegularExpression passwordRegex("^[a-zA-Z0-9@#$%^&+=]{6,}$");

    // 处理登录成功
    QObject::connect(m_pClientSocket, &CClientSocket::loginSuccess, [&qtin, m_pClientSocket](){
        handleRoomCreation(qtin, m_pClientSocket);
    });

    // 处理登录失败
    QObject::connect(m_pClientSocket, &CClientSocket::loginFailed, [](const QString& message){
        qDebug() << "Login failed with message:" << message;
        // 这里可以添加更多的逻辑，比如重新提示用户登录
    });

    // 处理注册成功
    QObject::connect(m_pClientSocket, &CClientSocket::registrationSuccess, [&qtin, m_pClientSocket](){
        handleRoomCreation(qtin, m_pClientSocket);
    });

    // 处理注册失败
    QObject::connect(m_pClientSocket, &CClientSocket::registrationFailed, [](const QString& message){
        qDebug() << "Registration failed with message:" << message;
        // 这里可以添加更多的逻辑，比如重新提示用户注册
    });

    // 处理创建房间成功
    QObject::connect(m_pClientSocket, &CClientSocket::roomCreated, [](const QString& roomCode){
        qDebug() << "Room created with code:" << roomCode;
        // 这里可以添加更多的逻辑，比如更新 UI 或者存储房间码
    });

    // 处理创建房间失败
    QObject::connect(m_pClientSocket, &CClientSocket::roomCreationFailed, [](const QString& message){
        qDebug() << "Failed to create room with message:" << message;
        // 这里可以添加更多的逻辑，比如提示用户房间创建失败的原因
    });

    // 处理加入房间成功
    QObject::connect(m_pClientSocket, &CClientSocket::roomJoined, [](const QString& roomCode){
        qDebug() << "Joined room with code:" << roomCode;
        // 这里可以添加更多的逻辑，比如更新 UI 或者存储房间码
    });

    // 处理加入房间失败
    QObject::connect(m_pClientSocket, &CClientSocket::roomJoinFailed, [](const QString& message){
        qDebug() << "Failed to join room with message:" << message;
        // 这里可以添加更多的逻辑，比如提示用户房间加入失败的原因
    });

    do {
        qDebug() << "Do you want to register or login? (register/login):";
        action = qtin.readLine().trimmed().toLower();
        if (action == "register") {
            qDebug().noquote() << "Please note the following requirements for registration:";
            qDebug().noquote() << "- Username must be 3 to 15 characters long and can only contain letters, numbers, and underscores.";
            qDebug().noquote() << "- Password must be at least 6 characters long and can contain letters, numbers, and special characters @#$%^&+=.";
        }

        if (action != "register" && action != "login") {
            qDebug().noquote() << RED_TEXT_START + "Invalid action. Please type 'register' or 'login'." + RED_TEXT_END;
            continue;
        }

        qDebug() << "Please enter username:";
        username = qtin.readLine().trimmed();
        if (!usernameRegex.match(username).hasMatch()) {
            qDebug().noquote() << RED_TEXT_START + "Username must be 3 to 15 characters long and can only contain letters, numbers, and underscores." + RED_TEXT_END;
            continue;
        }
        qDebug() << "Please enter password:";
        password = qtin.readLine().trimmed();
        if (!passwordRegex.match(password).hasMatch()) {
            qDebug().noquote() << RED_TEXT_START + "Password must be at least 6 characters long and can contain letters, numbers, and special characters @#$%^&+=." + RED_TEXT_END;
            continue;
        }
        inputValid = true;
    } while (!inputValid);

    if (action == "register") {
        m_pClientSocket->sendRegisterRequest(username, password);
    } else if (action == "login") {
        m_pClientSocket->sendLoginRequest(username, password);
    }

    return a.exec();
}
