#ifndef LOGMESSAGE_H
#define LOGMESSAGE_H

#include <QObject>
#include <QTextEdit>

class LogMessage : public QObject
{
public:
    explicit LogMessage(QObject *parentObj = nullptr);
    ~LogMessage();

    void setMsgWindow(QTextEdit *te);
    void setErrorWindow(QTextEdit *te);
    void setDebugFlag(Qt::CheckState flag);
    void setContext(QString contextString);

    bool isDebug() { return debugOutput; }
    bool isTrace() { return traceOutput; }

    enum class LogLevel {ERR, WARN, MSG, DBG, TRACE};

    void traceMsg(QString vMsgStr) { showMsg(vMsgStr, LogLevel::TRACE); }
    void debugMsg(QString vMsgStr) { showMsg(vMsgStr, LogLevel::DBG); }
    void infoMsg(QString vMsgStr) { showMsg(vMsgStr, LogLevel::MSG); }
    void warnMsg(QString vMsgStr) { showMsg(vMsgStr, LogLevel::WARN); }
    void errorMsg(QString vMsgStr) { showMsg(vMsgStr, LogLevel::ERR); }

private:
    void showMsg(QString vMsgStr, LogLevel ll);

    QTextEdit *mMsgWin;
    QTextEdit *mErrorWin;

    bool debugOutput;
    bool traceOutput;

    QString context;
    QHash <QString, int> msgHash;


};

#endif // LOGMESSAGE_H
