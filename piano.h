#ifndef PIANO_H
#define PIANO_H

#include <QObject>
#include <QFile>
#include <QTime>
#include <QThread>
#include <QMutex>


typedef int Note;

enum EventType
{
    ET_KeyDown,
    ET_KeyUp,
    ET_Time
};

struct PianoKeyEvent
{
    EventType type;
    Note key;
    QTime time;
};

struct DataToWrite
{
    char *data;
    size_t size;

    DataToWrite(size_t size)
        : size(size)
    {
        data = new char[size];
    }
    ~DataToWrite()
    {
        delete[] data;
    }
};

class ReadingThread : public QThread
{
    Q_OBJECT
    QFile &file;

    std::vector<DataToWrite> writeQueue;
    QMutex queueMutex;
public:
    ReadingThread(QFile &f);

signals:
    void newEvent(PianoKeyEvent event);

public slots:
    void writeData(char data[], size_t count);

protected:
    virtual void run() override;
};

class Piano : public QObject
{
    Q_OBJECT
    QFile deviceFile;
    ReadingThread *rThread;
public:
    explicit Piano(QObject *parent = 0);
    ~Piano();

    int connectToDevice(const QString devFileName);
    void disconnectFromDevice();

    void playNote(Note note);


signals:
    void keyStateChanged(PianoKeyEvent event);

public slots:
};

#endif // PIANO_H
