#include "piano.h"
#include <iostream>
#include <vector>

class ReadingThread;

Piano::Piano(QObject *parent)
    : QObject(parent)
    , rThread(nullptr)
{
    timer.setInterval(700);
    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), this, SLOT(needTurnOffNote()));
}

Piano::~Piano()
{
    delete rThread;
}

int Piano::connectToDevice(const QString devFileName)
{
    if (!QFile::exists(devFileName)) {
        std::cout << "Error: file not exists" << std::endl;
        return -1;
    }

    deviceFile.setFileName(devFileName);

    if (!deviceFile.open(QIODevice::ReadWrite | QIODevice::Unbuffered)) {
        std::cout << "Error opening file" << std::endl;
        return -1;
    }

    delete rThread;
    rThread = new ReadingThread(deviceFile);
    connect(rThread, SIGNAL(newEvent(PianoKeyEvent)), this, SIGNAL(keyStateChanged(PianoKeyEvent)));
    rThread->start();

    return 0;
}

void Piano::disconnectFromDevice()
{
    delete rThread;
    rThread = nullptr;
    if (deviceFile.isOpen())
        deviceFile.close();
}

void Piano::playNote(Note note)
{
    if (!rThread)
        return;

    char data[3];
    data[0] = 0x90;
    data[1] = (char)note;
    data[2] = 0x3f;
    lastNote = note;
    rThread->writeData(data, 3);
    timer.start();
}

void Piano::needTurnOffNote()
{
    if (!rThread)
        return;

    char data[3];
    data[0] = 0x90;
    data[1] = (char)lastNote;
    data[2] = 0;
    rThread->writeData(data, 3);
}

ReadingThread::ReadingThread(QFile &f)
    : file(f)
{
    if (!f.isOpen()) {
        std::cout << "error: file is not open" << std::endl;
    }
}

void ReadingThread::writeData(char data[], size_t count)
{
    DataToWrite *d = new DataToWrite(count);
    for (size_t i = 0; i < count; i++)
        d->data[i] = data[i];

    queueMutex.lock();
    writeQueue.push_back(*d);
    queueMutex.unlock();
}

void ReadingThread::run()
{
    while (true)
    {
        char data[50];
        file.read(data, 1);
        if ((data[0] & 0xf0) == 0x90) {
            file.read(data, 2);
            EventType type = (data[1])?ET_KeyDown:ET_KeyUp;
            emit newEvent({type, data[0], QTime::currentTime()});
        }

        if (writeQueue.size()) {
//            std::cout << "write data: ";
            queueMutex.lock();
            DataToWrite &d = writeQueue.back();
//            std::cout << std::hex;
//            std::cout << (int)d.data[0] << " " << (int)d.data[1] << " " << (int)d.data[2] << std::endl;
            file.write(d.data, d.size);
            writeQueue.pop_back();
            queueMutex.unlock();
        }
    }
}
