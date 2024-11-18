#include "ServerGame.h"
#include "Player.h"
#include <QCoreApplication>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    ServerGame serverGame;
    serverGame.startServer();

    // 初始化玩家
    std::vector<Player*> players = serverGame.getPlayers();
    serverGame.setState(SEND_CARD);

    // 发牌阶段
    serverGame.broadcastGameState();
    serverGame.dealCards();


    // 弃牌阶段
    serverGame.setState(DISCARD);
    serverGame.broadcastGameState();
    for (Player* player : players) {
        while (player->getHandCards().size() > 6) {
            // 等待客户端发送弃牌动作
            PlayerAction action = serverGame.receivePlayerAction();
            // 确保动作是弃牌动作
            if (action.getActionType() == DISCARD_CARD && action.getPlayerId() == player->getPlayerID()) {
                // 从玩家手牌中移除弃置的牌
                card_pointer cardToDiscard = action.getCard();
                player->removeHandCard(cardToDiscard);
                player->addDiscardedCard(cardToDiscard);

                // 广播弃牌动作给所有客户端
                serverGame.broadcastAction(action);
            }
        }
    }


    // 出牌阶段
    serverGame.setState(PLAY);
    while (!serverGame.allPlayerHandEmpty()) {
        Player* currentPlayer = serverGame.getCurrentPlayer();
        // 等待客户端发送出牌动作
        PlayerAction action = serverGame.receivePlayerAction();
        serverGame.broadcastAction(action);
        serverGame.setNextPlayer();
    }

    // 亮匿阶段
    serverGame.setState(SHOW_HIDE);
    serverGame.broadcastGameState();
    serverGame.revealHideCard();


    // 指认阶段
    serverGame.setState(POINT_OUT);
    serverGame.broadcastGameState();
    for (Player* player : players) {
        // 等待客户端发送指认动作
        PlayerAction action = serverGame.receivePlayerAction();
        if(action.getActionType() == POINT_OUT_CARD && action.getPlayerId() == player->getPlayerID()){
            card_pointer pointedcard = action.getCard();
            serverGame.addPointedCard(pointedcard);
        }
    }


    // 翻牌阶段
    serverGame.setState(FLIP_CARD);
    serverGame.broadcastGameState();
    serverGame.flipPointedCards();
    serverGame.checkWinner();

    // 结算阶段
    serverGame.setState(SETTLE);
    serverGame.broadcastGameState();
    serverGame.announceWinner();

    return app.exec();
}
