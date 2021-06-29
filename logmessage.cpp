#include "logmessage.h"

LogMessage::LogMessage(QObject *parentObj)
    : QObject(parentObj),
      mMsgWin(nullptr),
      mErrorWin(nullptr),
      debugOutput(false),
      traceOutput(false),
      context(""),
      msgHash()
{

}

LogMessage::~LogMessage()
{
    try
    {
        mMsgWin = nullptr;
        mErrorWin = nullptr;
        msgHash.clear();
    }
    catch (...)
    {
        // catch all exception
    }
}

void LogMessage::setMsgWindow(QTextEdit *te)
{
    mMsgWin = te;
}

void LogMessage::setErrorWindow(QTextEdit *te)
{
    mErrorWin = te;
}

void LogMessage::setDebugFlag(Qt::CheckState flag)
{
    traceOutput = (flag == Qt::Checked);
    debugOutput = (flag == Qt::PartiallyChecked) || traceOutput;
}

void LogMessage::setContext(QString contextString)
{
    context = contextString;
}

//! show message string
void LogMessage::showMsg(QString vMsgStr, LogLevel ll)
{
    if (LogLevel::DBG > ll || debugOutput )
    {
        QTextEdit *logWin = mMsgWin;
        if (nullptr != mErrorWin && LogLevel::MSG != ll)
        {
            logWin = mErrorWin;
        }

        if (logWin != nullptr)
        {
            QString logStr("");
            switch (ll)
            {
            case LogLevel::DBG:   logStr = "DEBUG"; break;
            case LogLevel::ERR:   logStr = "ERROR"; break;
            case LogLevel::WARN:  logStr = "WARN "; break;
            case LogLevel::MSG:   logStr = "INFO "; break;
            case LogLevel::TRACE: logStr = "TRACE"; break;
            default: break;
            }

            QString showStr = QString("%1:%2 %3")
                    .arg(logStr, context.isEmpty() ? QString("") : " [" + context + "]", vMsgStr);

            if (!msgHash.contains(showStr))
            {
                msgHash.insert(showStr, 1);
                logWin->append(showStr);
            }
        }
    }
}


