QT += core network sql
QT -= gui

CONFIG += c++17 console
CONFIG -= app_bundle

# 设置项目根目录
ROOT_DIR = $$PWD/..

# 添加 Qt 头文件路径
INCLUDEPATH += \
    $$[QT_INSTALL_HEADERS] \
    $$[QT_INSTALL_HEADERS]/QtCore \
    $$[QT_INSTALL_HEADERS]/QtNetwork \
    $$[QT_INSTALL_HEADERS]/QtSql

# 添加项目头文件路径
INCLUDEPATH += \
    $$ROOT_DIR \
    $$ROOT_DIR/Common \
    $$ROOT_DIR/Common/Card \
    $$ROOT_DIR/Common/Player \
    $$ROOT_DIR/Common/GameState \
    $$ROOT_DIR/Common/GameEvent \
    $$ROOT_DIR/Common/PlayerAction \
    $$PWD \
    $$PWD/Handler \
    $$PWD/Network \
    $$PWD/Game/ServerGame

# 添加所有源文件
SOURCES += \
    Network/TcpServer.cpp \
    Network/UserManager.cpp \
    Network/RoomManager.cpp \
    Network/GameRoom.cpp \
    Network/GameRoomWorker.cpp \
    Handler/RequestHandler.cpp \
    Game/ServerGame/ServerGame.cpp \
    ServerMain.cpp

# 添加所有头文件
HEADERS += \
    Network/TcpServer.h \
    Network/UserManager.h \
    Network/RoomManager.h \
    Network/GameRoom.h \
    Network/GameRoomWorker.h \
    Handler/RequestHandler.h \
    Game/ServerGame/ServerGame.h

# 包含公共配置
include(../common.pri)

# 添加编译器警告
DEFINES += QT_DEPRECATED_WARNINGS

# 设置目标名称
TARGET = TcpServer

# 简化构建目录结构
CONFIG(debug, debug|release) {
    DESTDIR = debug
    OBJECTS_DIR = debug/obj
    MOC_DIR = debug/moc
} else {
    DESTDIR = release
    OBJECTS_DIR = release/obj
    MOC_DIR = release/moc
}

# 部署规则
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target 