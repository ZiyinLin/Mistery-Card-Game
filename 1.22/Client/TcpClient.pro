QT += core network
QT -= gui

# 确保使用正确的 Qt 版本
QT_VERSION = $$[QT_VERSION]
message(Qt version: $$QT_VERSION)

CONFIG += c++17 console
CONFIG -= app_bundle

# 添加包含路径
INCLUDEPATH += \
    $$PWD \
    $$PWD/Network \
    $$PWD/Game/ClientGame \
    $$[QT_INSTALL_HEADERS]

# 添加源文件
SOURCES += \
    $$PWD/Network/CClientSocket.cpp \
    $$PWD/Network/InputHandler.cpp \
    $$PWD/Game/ClientGame/ClientGame.cpp \
    $$PWD/Game/ClientGame/UI.cpp \
    $$PWD/ClientMain.cpp

# 添加头文件
HEADERS += \
    $$PWD/Network/CClientSocket.h \
    $$PWD/Network/InputHandler.h \
    $$PWD/Game/ClientGame/ClientGame.h \
    $$PWD/Game/ClientGame/UI.h

# 包含公共配置
include(../common.pri)

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
