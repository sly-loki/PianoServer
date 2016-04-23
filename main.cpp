#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <QFile>
#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTime>

void onTick();

QFile f("/dev/midi3");
QTcpServer server;
QTimer timer;

enum EventType
{
    ET_KeyDown,
    ET_KeyUp,
    ET_Time
};

struct PianoKeyEvent
{
    EventType type;
    unsigned int key;
    QTime time;
};

class ReadingThread : public QThread
{
    Q_OBJECT
signals:
    void newEvent(PianoKeyEvent event);

protected:
    virtual void run() override
    {
        if (!f.open(QIODevice::ReadOnly | QIODevice::Unbuffered))
        {
            std::cout << "Error opening file" << std::endl;
        }
        while (true)
        {
            char data[50];
            f.read(data, 1);
            if ((data[0] & 0xf0) == 0x90)
            {
                f.read(data, 2);
                EventType type = (data[1])?ET_KeyDown:ET_KeyUp;
//                std::cout << "emit event";
                emit newEvent({type, data[0], QTime::currentTime()});
            }
        }
    }
};

void event(PianoKeyEvent event);
void onNewConnection();
void syncTask();

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    ReadingThread t;

    QObject::connect(&t, SIGNAL(finished()), &a, SLOT(quit()));
    QObject::connect(&t, &ReadingThread::newEvent, event);
    t.start();
    QObject::connect(&server, &QTcpServer::newConnection, onNewConnection);
    if (!server.listen(QHostAddress::Any, 1289))
        std::cout << "listen error" << std::endl;
    timer.setInterval(1000);
    QObject::connect(&timer, &QTimer::timeout, syncTask);
    timer.start();
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

#include "main.moc"
