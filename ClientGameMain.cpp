#include "ClientGame.h"
#include "Player.h"
#include <QCoreApplication>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    ClientGame clientGame("serverUri");
    clientGame.connectToServer();

    // 初始化玩家
    Player* player = clientGame.getPlayer();
    GameState& gameState = clientGame.getGameState();

    while (true) {
        // 更新游戏状态
        clientGame.updateGameState();

        switch (gameState.getState()) {
            case SEND_CARD:
                clientGame.updateHandCards();
                break;

            case DISCARD_CARD:
                while (player->getHandCards().size() > 6) {
                    // 选择弃牌并发送给服务器
                    PlayerAction action(DISCARD_CARD, player->getPlayerID(), player->chooseHandCard(gameState));
                    clientGame.sendAction(action);
                }
                break;

            case PLAY_CARD:
                if (clientGame.getMyId() == gameState.getCurrentPlayer()->getPlayerID()) {
                    bool validPlay = false;
                    while (!validPlay) {
                        card_pointer card = player->chooseHandCard(gameState);
                        if(!clientGame.isValidInput(player,card)){
                            gameState.sendErrorMessage(player->getPlayerID(),"你未拥有该牌！");
                            continue;
                        }
                        //明置出牌的合法性在play函数中检查，若不合法该牌的validPlay为false，循环继续
                        card->play(player, gameState, player->chooseFaceUp(gameState));
                        validPlay = card->getValidPlay();
                        if(validPlay){
                            if(card->getType()==SOLVE || card->getType()==READ ||card->getType()==STEAL){
                                PlayerAction action(PLAY_CARD, player->getPlayerID(), card,player->chooseTargetCard(gameState));
                            }else if(card->getType()==TRADE){
                                PlayerAction action(PLAY_CARD, player->getPlayerID(), card,player->chooseTargetPlayerID(gameState));
                            }else{
                                PlayerAction action(PLAY_CARD, player->getPlayerID(), card);
                            }
                            clientGame.sendAction(action);
                        }
                    }
                }else{
                    clientGame.receiveUpdates();
                }
                break;

            case SHOW_HIDE:
                clientGame.receiveUpdates();
                break;

            case POINT_OUT:
                // 选择指认并发送给服务器
                PlayerAction action(POINT_OUT_CARD, player->getPlayerID(), player->chooseTargetCard(gameState));
                clientGame.sendAction(action);
                break;

            case FLIP_CARD:
                clientGame.receiveUpdates();
                break;

            case SETTLE:
                clientGame.receiveUpdates();
                clientGame.displayWinner();
                break;

            default:
                break;
        }
    }

    return app.exec();
}
