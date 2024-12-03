#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#include <QThread>
#include <QTextStream>
#include <QEventLoop>
#include <QTimer>
#include "CClientSocket.h"
#include "../../Common/PlayerAction/PlayerAction.h"

class InputHandler : public QThread {
    Q_OBJECT
public:
    explicit InputHandler(CClientSocket* socket);
    void run() override;

signals:
    void inputReceived(const QString &input);
    void authenticationRequested(const QString &action, const QString &username, const QString &password);
    void createRoomRequested(int capacity);
    void joinRoomRequested(const QString &roomCode);
    void cardNumberSelected(int number);
    void faceUpSelected(bool faceUp);

public slots:
    void processUserInput(const QString& input);
    void handleError(const QString& message);
    void returnToMainMenu();
    void onGameStateChanged(const QString& state);

private slots:
    void onLoginSuccess();
    void onRegistrationSuccess();
    void onLoginFailed(const QString& message);
    void onRegistrationFailed(const QString& message);

private:
    enum class State {
        INIT,
        WAITING_USERNAME,
        WAITING_PASSWORD,
        WAITING_CHOICE,
        WAITING_ROOM_CODE,
        IN_ROOM,
        WAITING_DEAL,
        DISCARD,
        PLAY,
        POINT
    };

    CClientSocket* m_socket;
    QEventLoop waitForAuthentication;
    void handleRoomCreation();
    void processInput(const QString& input);
    void showMainMenu();

    State currentState = State::INIT;
    bool dealingPhaseComplete = false;
};

#endif // INPUTHANDLER_H
