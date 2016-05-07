#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTime>

#include "piano.h"

void onTick();


QTcpServer server;
QTimer timer;



void event(PianoKeyEvent event);
void onNewConnection();
void syncTask();

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    Piano piano;

    QObject::connect(&piano, &Piano::keyStateChanged, event);
    QObject::connect(&server, &QTcpServer::newConnection, onNewConnection);

    piano.connectToDevice("/dev/midi3");

    if (!server.listen(QHostAddress::Any, 1289))
        std::cout << "listen error" << std::endl;

    timer.setInterval(1000);
    QObject::connect(&timer, &QTimer::timeout, syncTask);
    timer.start();
    piano.playNote(0x48);

    return a.exec();
}

QTcpSocket * clientConnection = nullptr;

void onNewConnection()
{
    std::cout << "new connection" << std::endl;
    if (clientConnection)
        delete clientConnection;
    clientConnection = server.nextPendingConnection();
//    std::cout << clientConnection->
//    clientConnection->write("connection accepted");
    event({ET_Time, 0, QTime::currentTime()});
}

void syncTask()
{
    event({ET_Time, 0, QTime::currentTime()});
}

void event(PianoKeyEvent event)
{
    std::stringstream ss;
    ss << std::hex;
    switch(event.type)
    {
    case ET_KeyDown:
        ss << event.key << " d ";
        break;
    case ET_KeyUp:
        ss << event.key << " u ";
        break;
    case ET_Time:
        ss << "00 t ";
    }

    QString times = event.time.toString("HH mm ss zzz");
    ss << times.toStdString();

    std::cout << ss.str() << std::endl;
    if (clientConnection) {
        clientConnection->write(ss.str().c_str());
        clientConnection->flush();
    }
}
