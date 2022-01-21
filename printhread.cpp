#include "printhread.h"
#include "printer.h"

#include <QDebug>

PrintThread::PrintThread(QObject *parent) : QThread(parent)
{

}

PrintThread::~PrintThread()
{
    mMutex.lock();
    mQuit = true;
    mCond.wakeOne();
    mMutex.unlock();
    wait();
}

void PrintThread::setup(Printer *printer)
{
    mPrinter = printer;
}

void PrintThread::stop()
{
    mMutex.lock();
    mRunning = false;
    mMutex.unlock();
}

void PrintThread::execute_command(std::stringstream &ss)
{
    const QMutexLocker locker(&mMutex);
    if(mQueue.size() != 0) // if the queue is not empty
    {
        emit error("command queue was not empty when new commands were attempted");
    }
    else // start or wake the thread
    {
        // Auto execute code here
        // emit response("Testing...\n");

        std::string buffer;
        //while(ss >> buffer) // spaces split into different objects
        while(std::getline(ss, buffer)) // Reads whole line (includes spaces)
        {
            //buffer.erase(std::remove(buffer.begin(), buffer.end(), '\n'), buffer.end());
            mQueue.push(buffer);
        }

        if (!isRunning())
        {
            start(); // start a new thread if one has not been created before
        }
        else
        {
            mCond.wakeOne();   // else wake the thread
        }
    }
}

void PrintThread::clear_queue()
{
    mMutex.lock();
    while(mQueue.size() > 0)
    {
        mQueue.pop();
    }
    mMutex.unlock();
}

void PrintThread::run()
{
    while (!mQuit) {

        while(mQueue.size() > 0)
        {
            if(!mRunning) // If the queue is externally stopped
            {
                clear_queue();
                // Code to run on stop
                emit response("Print Stopped\n");
            }
            else
            {
                // === Code to run on each queue item ===
                //emit response(QString::fromStdString(mQueue.front()));
                //qDebug() << QString::fromStdString(mQueue.front());
                std::string commandString = mQueue.front();
                std::string delimeterChar = ",";
                size_t pos{0};
                std::string commandType;
                pos = commandString.find(delimeterChar);

                // I only need this if there is a chance there won't be a comma in an input
                if(pos != std::string::npos)
                {
                    commandType = commandString.substr(0, pos);
                    commandString.erase(0, pos + delimeterChar.length());
                }else
                {
                    commandType = commandString;
                    commandString = "";
                }

                //emit response(QString::fromStdString("The type is: " + commandType));
                //emit response(QString::fromStdString(commandString));

                ParserStatus parserStatus{mPrinter->parse_command(commandType, commandString)};

                switch (parserStatus)
                {
                case ParserStatus::NoError:
                    emit response(QString::fromStdString(commandType) + QString::fromStdString(": ") + QString::fromStdString(commandString));

                    msleep(150);

                    mQueue.pop(); // remove command from the queue
                    if(mQueue.size() == 0)
                    {
                        // code to run when the queue completes normally
                        emit response("Finished Queue\n");
                    }

                    break;
                case ParserStatus::CommandTypeNotFound:
                    emit response(QString::fromStdString("Command Not Found!: \"") + QString::fromStdString(commandType) + QString::fromStdString("\"\nStopping Print..."));
                    stop();

                    break;
                case ParserStatus::InvalidCommand:
                    emit response("Invalid Command!\nStopping Print...");
                    stop();

                    break;
                default:
                    break;
                }


            }
        }

        emit ended();
        mMutex.lock();
        mCond.wait(&mMutex); // wait until thread is woken again by transaction call
        // Once the thread is woken again
        mRunning = true;
        mMutex.unlock();
    }
}