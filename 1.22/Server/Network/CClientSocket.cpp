void CClientSocket::processJsonObject(const QJsonObject &jsonObj) {
    QString type = jsonObj.value("type").toString();
    
    static bool dealPhaseEnded = false;
    
    if (type == "game_state") {
        State currentState = static_cast<State>(jsonObj.value("state").toInt());
        QString myUsername = property("username").toString();
        
        // 发牌阶段结束时的总结
        if (currentState == DISCARD && !dealPhaseEnded) {
            dealPhaseEnded = true;
            
            // 从服务器发来的玩家数据中获取手牌信息
            QJsonArray players = jsonObj.value("players").toArray();
            for (const auto& playerValue : players) {
                QJsonObject playerObj = playerValue.toObject();
                // 找到当前玩家
                if (playerObj.value("player_id").toString() == myUsername) {
                    QJsonArray handCards = playerObj.value("hand_cards").toArray();
                    
                    // 输出手牌总结信息
                    qDebug() << "\n=== Your Hand After Dealing Phase ===";
                    qDebug() << "Total cards in hand:" << handCards.size();
                    
                    if (!handCards.isEmpty()) {
                        qDebug() << "Cards breakdown:";
                        // 统计每种卡牌的数量
                        QMap<CardType, int> cardCounts;
                        for (const auto& cardValue : handCards) {
                            QJsonObject cardObj = cardValue.toObject();
                            CardType cardType = static_cast<CardType>(cardObj.value("card_type").toInt());
                            cardCounts[cardType]++;
                        }
                        
                        // 输出每种卡牌的数量
                        for (auto it = cardCounts.begin(); it != cardCounts.end(); ++it) {
                            qDebug() << "-" << getCardTypeName(it.key()) << ":" << it.value();
                        }
                    }
                    break;
                }
            }
        }
    }
} 