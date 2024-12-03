#include "UI.h"
#include "Card.h"
#include <QDebug>

UI::UI(QObject* parent) : QObject(parent) {
}

void UI::setGameMessage(const QString& message) {
    qDebug() << "Game Message:" << message;
    // 实现显示游戏消息的逻辑
}

void UI::toggleHandCardSelection(bool enable) {
    qDebug() << "Hand card selection" << (enable ? "enabled" : "disabled");
    emit handCardSelectionChanged(enable);
}

void UI::toggleTableCardSelection(bool enable) {
    qDebug() << "Table card selection" << (enable ? "enabled" : "disabled");
    emit tableCardSelectionChanged(enable);
}

// 修改 displayHandCards 函数
void UI::displayHandCards(const std::vector<std::shared_ptr<Card>>& cards) {
    qDebug() << "\n你的手牌:";
    for (size_t i = 0; i < cards.size(); i++) {
        QString cardType;
        switch(cards[i]->getType()) {
        case CardType::READ: cardType = "Read"; break;
        case CardType::SOLVE: cardType = "Solve"; break;
        case CardType::TRAP: cardType = "Trap"; break;
        case CardType::THINK: cardType = "Think"; break;
        case CardType::TRADE: cardType = "Trade"; break;
        case CardType::STEAL: cardType = "Steal"; break;
        case CardType::INDUCE: cardType = "Induce"; break;
        case CardType::REVERSE: cardType = "Reverse"; break;
        case CardType::COVER: cardType = "Cover"; break;
        case CardType::HIDE: cardType = "Hide"; break;
        case CardType::ANSWER: cardType = "Answer"; break;
        case CardType::FIRST: cardType = "First"; break;
        }
        qDebug().nospace() << (i + 1) << "." << cardType;
    }
}

// 修改 displayTableCards 函数
void UI::displayTableCards(const QString& playerId, const std::vector<std::shared_ptr<Card>>& cards) {
    qDebug() << "\n" << playerId << "的场上牌:";
    for (size_t i = 0; i < cards.size(); i++) {
        QString display;
        if (cards[i]->getFaceUp()) {
            switch(cards[i]->getType()) {
            case CardType::READ: display = "Read"; break;
            case CardType::SOLVE: display = "Solve"; break;
            case CardType::TRAP: display = "Trap"; break;
            case CardType::THINK: display = "Think"; break;
            case CardType::TRADE: display = "Trade"; break;
            case CardType::STEAL: display = "Steal"; break;
            case CardType::INDUCE: display = "Induce"; break;
            case CardType::REVERSE: display = "Reverse"; break;
            case CardType::COVER: display = "Cover"; break;
            case CardType::HIDE: display = "Hide"; break;
            case CardType::ANSWER: display = "Answer"; break;
            case CardType::FIRST: display = "First"; break;
            }
        } else {
            display = "??";
        }
        qDebug().nospace() << (i + 1) << "." << display;
    }
}

// 添加显示提示信息的函数
void UI::displayPrompt(const QString& prompt) {
    qDebug() << "\n" << prompt;
}

// 添加显示明暗置选择的函数
void UI::displayFaceUpChoice() {
    qDebug() << "\n1.明置 2.暗置";
}
