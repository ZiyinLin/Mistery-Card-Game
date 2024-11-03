#include <QCoreApplication>
#include <QDir>
#include <TcpServer.h>
#include <QTextStream>
#include <QTimer>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QDir::setCurrent(a.applicationDirPath());

    TcpServer* server = new TcpServer;
    server->StartListen(1000);
    QTextStream cin(stdin);
    QTimer timer;
    timer.start(100);
    return a.exec();
}
