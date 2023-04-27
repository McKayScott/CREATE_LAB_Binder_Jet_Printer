#include "pcd.h"

#include <QSerialPort>
#include <QDebug>

namespace PCD
{

Controller::Controller(const QString &portName, QObject *parent) :
    AsyncSerialDevice(portName, parent)
{
    name = "Pressure Controller";
    // connect timer for handling timeout errors
    connect(serialPort, &QSerialPort::readyRead, this, &Controller::handle_ready_read);
    connect(timer, &QTimer::timeout, this, &Controller::handle_timeout);
    connect(serialPort, &QSerialPort::errorOccurred, this, &Controller::handle_serial_error);
}

Controller::~Controller()
{
    if (is_connected()) serialPort->close();
}

void Controller::handle_ready_read()
{
    auto inData = serialPort->readAll();
    readData.append(inData);

    if (!inData.contains("\r")) {return;}
    switch (initState)
    {
    case NOT_INITIALIZED:
        if (QString(readData).remove("\r") == initString)
        {
            readData.clear();
            initState = INITIALIZED;
            write_next();
        }
        else
        {
            emit error("Unexpected response from device");
            disconnect_serial();
        }
        break;
    case INITIALIZED:
        // handle response from PCD here
        write_next();
        break;

    default: break;
    }
}

void Controller::handle_timeout()
{
    timer->stop();
    emit error("Serial IO Timeout: No response from pressure controller");
    emit error(readData);
    disconnect_serial();
}

int Controller::connect_to_pressure_controller()
{
    if (is_connected()) // return if already connected
    {
        emit response("Already connected to pressure controller");
        return 0;
    }

    clear_members();

    // attempt to open connection to serial port
    if (!serialPort->open(QIODevice::ReadWrite))
    {
        emit error(QString("Can't open %1 on %2, error code %3")
                   .arg(name, serialPort->portName(), QVariant::fromValue(serialPort->error()).toString()));
        return -1;
    }

    // wait after connection to initialize
    emit response("Connecting to pressure controller");
    QTimer::singleShot(500, this, &Controller::initialize_pressure_controller);
    return 0;
}

void Controller::disconnect_serial()
{
    clear_members();
    if (is_connected())
    {
        emit response("Disconnecting pressure controller");
        serialPort->close();
        timer->stop();
    }
    else emit response("pressure controller is already disconnected");
}

void Controller::update_set_point(double setPoint_PSIG)
{
    QString command =  QString("%1s%2\r").arg((char)UNIT_ID).arg(setPoint_PSIG);
    write(command.toUtf8());
}

void Controller::purge()
{
    write(QString("%1\r").arg((char)PURGE_ON).toUtf8());
}

void Controller::stop_purge()
{
    write(QString("%1\r").arg((char)PURGE_OFF).toUtf8());
}

void Controller::initialize_pressure_controller()
{
    write(QString("%1\r").arg((char)INIT).toUtf8());
}

void Controller::clear_members()
{
    initState = InitState::NOT_INITIALIZED;
    clear_command_queue();
}

void Controller::handle_serial_error(
        QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError)
    {
        emit error(QObject::tr("Read error on port %1, error: %2")
                            .arg(serialPort->portName(),
                                 serialPort->errorString()));
        disconnect_serial();
    }

    if (serialPortError == QSerialPort::OpenError)
    {
        emit error(QObject::tr("Error opening port %1, error: %2")
                            .arg(serialPort->portName(),
                                 serialPort->errorString()));
        disconnect_serial();
    }

    if (serialPortError == QSerialPort::WriteError)
    {
        emit error(QObject::tr("Error writing to port %1, error: %2")
                            .arg(serialPort->portName(),
                                 serialPort->errorString()));
        disconnect_serial();
    }
}

}

# include "moc_pcd.cpp"
