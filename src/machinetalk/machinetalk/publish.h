/****************************************************************************
**
** This code was generated by a code generator based on imatix/gsl
** Any changes in this code will be lost.
**
****************************************************************************/
#ifndef PUBLISH_H
#define PUBLISH_H
#include <QObject>
#include <QStateMachine>
#include <nzmqt/nzmqt.hpp>
#include <machinetalk/protobuf/message.pb.h>
#include <google/protobuf/text_format.h>

namespace machinetalk {

class Publish : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool ready READ ready WRITE setReady NOTIFY readyChanged)
    Q_PROPERTY(QString socketUri READ socketUri WRITE setSocketUri NOTIFY socketUriChanged)
    Q_PROPERTY(QString debugName READ debugName WRITE setDebugName NOTIFY debugNameChanged)
    Q_PROPERTY(State connectionState READ state NOTIFY stateChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorStringChanged)
    Q_PROPERTY(int heartbeatInterval READ heartbeatInterval WRITE setHeartbeatInterval NOTIFY heartbeatIntervalChanged)
    Q_ENUMS(State)

public:
    explicit Publish(QObject *parent = 0);
    ~Publish();

    enum State {
        Down = 0,
        Up = 1,
    };

    QString socketUri() const
    {
        return m_socketUri;
    }

    QString debugName() const
    {
        return m_debugName;
    }

    State state() const
    {
        return m_state;
    }

    QString errorString() const
    {
        return m_errorString;
    }

    int heartbeatInterval() const
    {
        return m_heartbeatInterval;
    }

    bool ready() const
    {
        return m_ready;
    }

public slots:

    void setSocketUri(QString uri)
    {
        if (m_socketUri == uri)
            return;

        m_socketUri = uri;
        emit socketUriChanged(uri);
    }

    void setDebugName(QString debugName)
    {
        if (m_debugName == debugName)
            return;

        m_debugName = debugName;
        emit debugNameChanged(debugName);
    }

    void setHeartbeatInterval(int interval)
    {
        if (m_heartbeatInterval == interval)
            return;

        m_heartbeatInterval = interval;
        emit heartbeatIntervalChanged(interval);
    }

    void setReady(bool ready)
    {
        if (m_ready == ready)
            return;

        m_ready = ready;
        emit readyChanged(ready);

        if (m_ready)
        {
            start();
        }
        else
        {
            stop();
        }
    }


    void sendSocketMessage(pb::ContainerType type, pb::Container *tx);
    void sendFullUpdate(pb::Container *tx);
    void sendIncrementalUpdate(pb::Container *tx);

protected:
    void start(); // start trigger
    void stop(); // stop trigger

private:
    bool m_ready;
    QString m_debugName;

    QString m_socketUri;
    nzmqt::PollingZMQContext *m_context;
    nzmqt::ZMQSocket *m_socket;

    State         m_state;
    State         m_previousState;
    QStateMachine *m_fsm;
    QString       m_errorString;

    QTimer     *m_heartbeatTimer;
    int         m_heartbeatInterval;
    // more efficient to reuse a protobuf Messages
    pb::Container m_socketRx;
    pb::Container m_socketTx;

private slots:

    void heartbeatTimerTick();
    void resetHeartbeatTimer();
    void startHeartbeatTimer();
    void stopHeartbeatTimer();

    bool startSocket();
    void stopSocket();

    void processSocketMessage(const QList<QByteArray> &messageList);
    void socketError(int errorNum, const QString& errorMsg);

    void sendPing();

    void fsmDownEntered();
    void fsmDownStartEvent();
    void fsmUpEntered();
    void fsmUpStopEvent();
    void fsmUpHeartbeatTickEvent();


signals:

    void socketUriChanged(QString uri);
    void socketMessageReceived(const pb::Container &rx);
    void debugNameChanged(QString debugName);
    void stateChanged(Publish::State state);
    void errorStringChanged(QString errorString);
    void heartbeatIntervalChanged(int interval);
    void readyChanged(bool ready);
    // fsm
    void fsmDownStart();
    void fsmDownStartQueued();
    void fsmUpStop();
    void fsmUpStopQueued();
    void fsmUpHeartbeatTick();
    void fsmUpHeartbeatTickQueued();
};
}; // namespace machinetalk
#endif //PUBLISH_H