#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <QFile>
#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>

void onTick();

QFile f("/dev/midi3");
QTcpServer server;

enum EventType
{
    ET_KeyDown,
    ET_KeyUp
};

struct PianoKeyEvent
{
    EventType type;
    unsigned int key;
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
                std::cout << "emit event";
                emit newEvent({type, data[0]});
            }
        }
    }
};

void event(PianoKeyEvent event);
void onNewConnection();

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
}

void event(PianoKeyEvent event)
{
    std::stringstream ss;
    ss << std::hex;
    switch(event.type)
    {
    case ET_KeyDown:
        ss << event.key << " dw";
        break;
    case ET_KeyUp:
        ss << event.key << " up";
        break;
    }

    std::cout << ss.str() << std::endl;
    if (clientConnection) {
        clientConnection->write(ss.str().c_str());
        clientConnection->flush();
    }
}

#include "main.moc"
