#ifndef ASYNCSERIALDEVICE_H
#define ASYNCSERIALDEVICE_H

#include <QObject>
#include <QQueue>
#include <QTimer>
#include <QSerialPort>

class AsyncSerialDevice : public QObject
{
    Q_OBJECT
public:
    explicit AsyncSerialDevice(QObject *parent = nullptr);
    bool is_connected() const;

signals:
    void response(const QString &s);
    void error(const QString &s);
    void timeout(const QString &s);

protected:
    void write(const QByteArray &data);
    void write_next();
    void clear_command_queue();

protected:
    QByteArray prevWrite;
    QSerialPort *serialPort {nullptr};
    QTimer *timer {nullptr};
    QByteArray readData;

private:

private:
    QQueue<QByteArray> writeQueue;
    bool isWriteReady {true};
    const int readTimeout_ms {3000};

};

#endif // ASYNCSERIALDEVICE_H
