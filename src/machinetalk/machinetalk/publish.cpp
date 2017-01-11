/****************************************************************************
**
** This code was generated by a code generator based on imatix/gsl
** Any changes in this code will be lost.
**
****************************************************************************/
#include "publish.h"
#include "debughelper.h"

#if defined(Q_OS_IOS)
namespace gpb = google_public::protobuf;
#else
namespace gpb = google::protobuf;
#endif

using namespace nzmqt;

namespace machinetalk {

/** Generic Publish implementation */
Publish::Publish(QObject *parent) :
    QObject(parent),
    m_ready(false),
    m_debugName("Publish"),
    m_socketUri(""),
    m_context(nullptr),
    m_socket(nullptr),
    m_state(Down),
    m_previousState(Down),
    m_errorString("")
    ,m_heartbeatTimer(new QTimer(this)),
    m_heartbeatInterval(2500),
{

    m_heartbeatTimer->setSingleShot(true);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &Publish::heartbeatTimerTick);
    // state machine
    connect(this, &Publish::fsmDownStart,
            this, &Publish::fsmDownStartEvent);
    connect(this, &Publish::fsmUpStop,
            this, &Publish::fsmUpStopEvent);
    connect(this, &Publish::fsmUpHeartbeatTick,
            this, &Publish::fsmUpHeartbeatTickEvent);

    m_context = new PollingZMQContext(this, 1);
    connect(m_context, &PollingZMQContext::pollError,
            this, &Publish::socketError);
    m_context->start();
}

Publish::~Publish()
{
    stopSocket();

    if (m_context != nullptr)
    {
        m_context->stop();
        m_context->deleteLater();
        m_context = nullptr;
    }
}

/** Connects the 0MQ sockets */
bool Publish::startSocket()
{
    m_socket = m_context->createSocket(ZMQSocket::TYP_XPUB, this);
    m_socket->setLinger(0);

    try {
        m_socket->connectTo(m_socketUri);
    }
    catch (const zmq::error_t &e) {
        QString errorString;
        errorString = QString("Error %1: ").arg(e.num()) + QString(e.what());
        //updateState(SocketError, errorString); TODO
        return false;
    }

    connect(m_socket, &ZMQSocket::messageReceived,
            this, &Publish::processSocketMessage);


#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "sockets connected" << m_socketUri);
#endif

    return true;
}

/** Disconnects the 0MQ sockets */
void Publish::stopSocket()
{
    if (m_socket != nullptr)
    {
        m_socket->close();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
}


void Publish::resetHeartbeatTimer()
{
    if (m_heartbeatTimer->isActive())
    {
        m_heartbeatTimer->stop();
    }

    if (m_heartbeatInterval > 0)
    {
        m_heartbeatTimer->setInterval(m_heartbeatInterval);
        m_heartbeatTimer->start();
    }
}

void Publish::startHeartbeatTimer()
{
    resetHeartbeatTimer();
}

void Publish::stopHeartbeatTimer()
{
    m_heartbeatTimer->stop();
}

void Publish::heartbeatTimerTick()
{
    if (m_state == Up)
    {
        emit fsmUpHeartbeatTick(QPrivateSignal());
    }
}

/** Processes all message received on socket */
void Publish::processSocketMessage(const QList<QByteArray> &messageList)
{
    pb::Container &rx = m_socketRx;
    rx.ParseFromArray(messageList.at(0).data(), messageList.at(0).size());

#ifdef QT_DEBUG
    std::string s;
    gpb::TextFormat::PrintToString(rx, &s);
    DEBUG_TAG(3, m_debugName, "server message" << QString::fromStdString(s));
#endif

    emit socketMessageReceived(rx);
}

void Publish::sendSocketMessage(pb::ContainerType type, pb::Container &tx)
{
    if (m_socket == nullptr) {  // disallow sending messages when not connected
        return;
    }

    tx.set_type(type);
#ifdef QT_DEBUG
    std::string s;
    gpb::TextFormat::PrintToString(tx, &s);
    DEBUG_TAG(3, m_debugName, "sent message" << QString::fromStdString(s));
#endif
    try {
        m_socket->sendMessage(QByteArray(tx.SerializeAsString().c_str(), tx.ByteSize()));
    }
    catch (const zmq::error_t &e) {
        QString errorString;
        errorString = QString("Error %1: ").arg(e.num()) + QString(e.what());
        //updateState(SocketError, errorString);  TODO
        return;
    }
    tx.Clear();
}

void Publish::sendPing()
{
    pb::Container &tx = m_socketTx;
    sendSocketMessage(pb::MT_PING, tx);
}

void Publish::sendFullUpdate(pb::Container &tx)
{
    sendSocketMessage(pb::MT_FULL_UPDATE, tx);
}

void Publish::sendIncrementalUpdate(pb::Container &tx)
{
    sendSocketMessage(pb::MT_INCREMENTAL_UPDATE, tx);
}

void Publish::socketError(int errorNum, const QString &errorMsg)
{
    QString errorString;
    errorString = QString("Error %1: ").arg(errorNum) + errorMsg;
    //updateState(SocketError, errorString);  TODO
}

void Publish::fsmDown()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "State DOWN");
#endif
    m_state = Down;
    emit stateChanged(m_state);
}

void Publish::fsmDownStartEvent()
{
    if (m_state == Down)
    {
#ifdef QT_DEBUG
        DEBUG_TAG(1, m_debugName, "Event START");
#endif
        // handle state change
        emit fsmDownExited(QPrivateSignal());
        fsmUp();
        emit fsmUpEntered(QPrivateSignal());
        // execute actions
        startSocket();
        startHeartbeatTimer();
     }
}

void Publish::fsmUp()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "State UP");
#endif
    m_state = Up;
    emit stateChanged(m_state);
}

void Publish::fsmUpStopEvent()
{
    if (m_state == Up)
    {
#ifdef QT_DEBUG
        DEBUG_TAG(1, m_debugName, "Event STOP");
#endif
        // handle state change
        emit fsmUpExited(QPrivateSignal());
        fsmDown();
        emit fsmDownEntered(QPrivateSignal());
        // execute actions
        stopHeartbeatTimer();
        stopSocket();
     }
}

void Publish::fsmUpHeartbeatTickEvent()
{
    if (m_state == Up)
    {
#ifdef QT_DEBUG
        DEBUG_TAG(1, m_debugName, "Event HEARTBEAT TICK");
#endif
        // execute actions
        sendPing();
        resetHeartbeatTimer();
     }
}

/** start trigger function */
void Publish::start()
{
    if (m_state == Down) {
        emit fsmDownStart(QPrivateSignal());
    }
}

/** stop trigger function */
void Publish::stop()
{
    if (m_state == Up) {
        emit fsmUpStop(QPrivateSignal());
    }
}
}; // namespace machinetalk
