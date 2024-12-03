#ifndef CLIENTGAME_H
#define CLIENTGAME_H

#include "../../../Common/Card/Card.h"
#include "../../../Common/GameState/GameState.h"
#include "../../../Common/PlayerAction/PlayerAction.h"
#include "../../../Common/Player/Player.h"
#include "UI.h"  // 添加 UI 头文件
#include <QString>
#include <QTcpSocket>
#include <QJsonObject>

// 前向声明
class UI;  // 添加 UI 类的前向声明

class ClientGame : public QObject {
    Q_OBJECT
public:
    explicit ClientGame(Player* player);
    virtual ~ClientGame();  // 添加虚析构函数声明
    void sendPlayerAction(PlayerAction& action);  // 发送数据
    bool isValidInput(Player* player, std::shared_ptr<Card> card);  // 检验合法性
    State getState();  // 获取游戏状态
    Player* getPlayer();  // 获取玩家
    QString getMyId();  // 获取玩家ID
    void startGame();

signals:
    void updateMessage(const QString& message);  // 更新提示信息
    void enableCardSelection(bool enable);  // 启用/禁用手牌选择
    void enableTableCardSelection(bool enable);  // 启用/禁用场上牌选择
    void cardSelected(std::shared_ptr<Card> card);  // 玩家选择了手牌
    void tableCardSelected(std::shared_ptr<Card> card);  // 玩家选择了场上卡牌
    void sendPlayerAction(const QJsonObject& action);

private slots:
    void onGameStateReceived(GameState& gameState);

public slots:
    void onCardNumberSelected(int number);  // 处理卡牌选择
    void onFaceUpSelected(bool faceUp);     // 处理明暗置选择

private:
    GameState* my_game_state;
    Player* m_player;
    QString current_player_id;
    bool actionCompleted;
    UI* ui;
    QTcpSocket* tcpSocket;
    void discardCard(std::shared_ptr<Card> card);
    void playCard(std::shared_ptr<Card> card);
    void selectTargetCard(std::shared_ptr<Card> targetCard);
    void selectTargetPlayer(QString targetPlayerID);
    void pointOutCard(std::shared_ptr<Card> card);

    void handleDiscard();
    void handlePlay();
    void handlePointOut();

    void processGameState(GameState& gameState);

    void flipCardUI(Player* player, std::shared_ptr<Card> card);  // 翻牌动画
    void playCardUI(Player* player, std::shared_ptr<Card> card);  // 出牌动画
    void readCardUI(Player* player, std::shared_ptr<Card> card);  // 显示卡牌（打出"读"后看别人牌）
    void discardCardUI(Player* player, std::shared_ptr<Card> card);  // 弃牌动画
    void addHandCardUI(Player* player, std::shared_ptr<Card> card);  // 添加手牌动画
    void takeBackCardUI(Player* player, std::shared_ptr<Card> card);  // 收回卡牌
    void displayWinnerUI(const QString playerID);  // 显示胜利者
};

#endif // CLIENTGAME_H
