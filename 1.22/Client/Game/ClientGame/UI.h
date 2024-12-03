#ifndef UI_H
#define UI_H

#include <QObject>
#include <QString>
#include "Card.h"

class UI : public QObject {
    Q_OBJECT
public:
    explicit UI(QObject* parent = nullptr);
    virtual ~UI() = default;
    void displayHandCards(const std::vector<std::shared_ptr<Card>>& cards);

public slots:
    void setGameMessage(const QString& message);
    void toggleHandCardSelection(bool enable);
    void toggleTableCardSelection(bool enable);
    void displayTableCards(const QString& playerId, const std::vector<std::shared_ptr<Card>>& cards);
    void displayPrompt(const QString& prompt);
    void displayFaceUpChoice();

signals:
    void handCardSelectionChanged(bool enabled);
    void tableCardSelectionChanged(bool enabled);

private:
         // 添加任何需要的私有成员
};

#endif // UI_H
