void RequestHandler::run()
{
    // ... 其他代码保持不变 ...
    
    else if (type == "player_ready") {
        // 使用新的方法查找玩家所在的房间
        GameRoom* room = roomManager->findPlayerRoom(socket);
        if (room) {
            room->handlePlayerReady(socket);
        }
    }
    
    // ... 其他代码保持不变 ...
} 