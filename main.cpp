#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include <cstdio>
#include <QFile>
#include <QThread>

void onTick();

QFile f("/dev/midi3");

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
                emit newEvent({type, data[0]});
            }
        }
    }
};

void event(PianoKeyEvent event);

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    ReadingThread t;

    QObject::connect(&t, SIGNAL(finished()), &a, SLOT(quit()));
    QObject::connect(&t, &ReadingThread::newEvent, event);
    t.start();

    return a.exec();
}

void event(PianoKeyEvent event)
{
    switch(event.type)
    {
    case ET_KeyDown:
        std::cout << event.key << " down" << std::endl;
        break;
    case ET_KeyUp:
        std::cout << event.key << " up" << std::endl;
        break;
    }
}

#include "main.moc"
