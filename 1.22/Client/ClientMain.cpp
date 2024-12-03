#include <QCoreApplication>
#include "Network/CClientSocket.h"
#include "Network/InputHandler.h"
#include <QTextStream>
#include <QRegularExpression>
#include <QTimer>
#include <QThread>

const QString RED_TEXT_START = "\033[31m";
const QString RED_TEXT_END = "\033[0m";

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    
    // 设置全局编码为 UTF-8
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    #endif
    
    CClientSocket* m_pClientSocket = new CClientSocket();
    m_pClientSocket->connectToServer("127.0.0.1", 1000);
    
    InputHandler *inputHandler = new InputHandler(m_pClientSocket);
    
    // 连接信号槽
    QObject::connect(inputHandler, &InputHandler::authenticationRequested,
                    [m_pClientSocket](const QString &action, const QString &username, const QString &password) {
        if (action == "register") {
            m_pClientSocket->sendRegisterRequest(username, password);
        } else if (action == "login") {
            m_pClientSocket->sendLoginRequest(username, password);
        }
    });

    QObject::connect(inputHandler, &InputHandler::createRoomRequested,
                    m_pClientSocket, &CClientSocket::sendCreateRoomRequest);

    QObject::connect(inputHandler, &InputHandler::joinRoomRequested,
                    m_pClientSocket, &CClientSocket::sendJoinRoomRequest);

    QObject::connect(inputHandler, &InputHandler::inputReceived, 
                    [m_pClientSocket](const QString &input) {
        m_pClientSocket->sendDataToServer(input);
    });

    // 当应用程序退出时清理资源
    QObject::connect(&a, &QCoreApplication::aboutToQuit, [inputHandler]() {
        inputHandler->requestInterruption();
        inputHandler->wait();
        delete inputHandler;
    });

    inputHandler->start();
    return a.exec();
}
