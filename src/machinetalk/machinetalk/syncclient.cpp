/****************************************************************************
**
** This code was generated by a code generator based on imatix/gsl
** Any changes in this code will be lost.
**
****************************************************************************/
#include "syncclient.h"
#include "debughelper.h"

#if defined(Q_OS_IOS)
namespace gpb = google_public::protobuf;
#else
namespace gpb = google::protobuf;
#endif

using namespace nzmqt;

namespace machinetalk {

/** Generic Sync Client implementation */
SyncClient::SyncClient(QObject *parent) :
    QObject(parent),
    QQmlParserStatus(),
    m_componentCompleted(false),
    m_ready(false),
    m_debugName("Sync Client"),
    m_syncChannel(nullptr),
    m_subChannel(nullptr),
    m_pubChannel(nullptr),
    m_state(Down),
    m_previousState(Down),
    m_errorString("")
{
    // initialize sync channel
    m_syncChannel = new machinetalk::RpcClient(this);
    m_syncChannel->setDebugName(m_debugName + " - sync");
    connect(m_syncChannel, &machinetalk::RpcClient::socketUriChanged,
            this, &SyncClient::syncUriChanged);
    connect(m_syncChannel, &machinetalk::RpcClient::stateChanged,
            this, &SyncClient::syncChannelStateChanged);
    connect(m_syncChannel, &machinetalk::RpcClient::socketMessageReceived,
            this, &SyncClient::processSyncChannelMessage);
    // initialize sub channel
    m_subChannel = new machinetalk::Subscribe(this);
    m_subChannel->setDebugName(m_debugName + " - sub");
    connect(m_subChannel, &machinetalk::Subscribe::socketUriChanged,
            this, &SyncClient::subUriChanged);
    connect(m_subChannel, &machinetalk::Subscribe::stateChanged,
            this, &SyncClient::subChannelStateChanged);
    connect(m_subChannel, &machinetalk::Subscribe::socketMessageReceived,
            this, &SyncClient::processSubChannelMessage);
    // initialize pub channel
    m_pubChannel = new machinetalk::Publish(this);
    m_pubChannel->setDebugName(m_debugName + " - pub");
    connect(m_pubChannel, &machinetalk::Publish::socketUriChanged,
            this, &SyncClient::pubUriChanged);

    connect(m_syncChannel, &machinetalk::RpcClient::heartbeatIntervalChanged,
            this, &SyncClient::syncHeartbeatIntervalChanged);

    connect(m_subChannel, &machinetalk::Subscribe::heartbeatIntervalChanged,
            this, &SyncClient::subHeartbeatIntervalChanged);

    connect(m_pubChannel, &machinetalk::Publish::heartbeatIntervalChanged,
            this, &SyncClient::pubHeartbeatIntervalChanged);
    // state machine
    connect(this, &SyncClient::fsmDownStart,
            this, &SyncClient::fsmDownStartEvent);
    connect(this, &SyncClient::fsmTryingSyncStateUp,
            this, &SyncClient::fsmTryingSyncStateUpEvent);
    connect(this, &SyncClient::fsmTryingStop,
            this, &SyncClient::fsmTryingStopEvent);
    connect(this, &SyncClient::fsmSyncingSyncStateTrying,
            this, &SyncClient::fsmSyncingSyncStateTryingEvent);
    connect(this, &SyncClient::fsmSyncingSubStateUp,
            this, &SyncClient::fsmSyncingSubStateUpEvent);
    connect(this, &SyncClient::fsmSyncingStop,
            this, &SyncClient::fsmSyncingStopEvent);
    connect(this, &SyncClient::fsmSyncedSubStateTrying,
            this, &SyncClient::fsmSyncedSubStateTryingEvent);
    connect(this, &SyncClient::fsmSyncedSyncStateTrying,
            this, &SyncClient::fsmSyncedSyncStateTryingEvent);
    connect(this, &SyncClient::fsmSyncedStop,
            this, &SyncClient::fsmSyncedStopEvent);
}

SyncClient::~SyncClient()
{
}

/** Add a topic that should be subscribed **/
void SyncClient::addSubTopic(const QString &name)
{
    m_subChannel->addSocketTopic(name);
}

/** Removes a topic from the list of topics that should be subscribed **/
void SyncClient::removeSubTopic(const QString &name)
{
    m_subChannel->removeSocketTopic(name);
}

/** Clears the the topics that should be subscribed **/
void SyncClient::clearSubTopics()
{
    m_subChannel->clearSocketTopics();
}

void SyncClient::startSyncChannel()
{
    m_syncChannel->setReady(true);
}

void SyncClient::stopSyncChannel()
{
    m_syncChannel->setReady(false);
}

void SyncClient::startSubChannel()
{
    m_subChannel->setReady(true);
}

void SyncClient::stopSubChannel()
{
    m_subChannel->setReady(false);
}

void SyncClient::startPubChannel()
{
    m_pubChannel->setReady(true);
}

void SyncClient::stopPubChannel()
{
    m_pubChannel->setReady(false);
}

/** Processes all message received on sync */
void SyncClient::processSyncChannelMessage(const pb::Container &rx)
{

    emit syncMessageReceived(rx);
}

/** Processes all message received on sub */
void SyncClient::processSubChannelMessage(const QByteArray &topic, const pb::Container &rx)
{

    emit subMessageReceived(topic, rx);
}

void SyncClient::sendSyncMessage(pb::ContainerType type, pb::Container &tx)
{
    m_syncChannel->sendSocketMessage(type, tx);
}

void SyncClient::sendPubMessage(pb::ContainerType type, pb::Container &tx)
{
    m_pubChannel->sendSocketMessage(type, tx);
}

void SyncClient::sendSync()
{
    pb::Container &tx = m_syncTx;
    sendSyncMessage(pb::MT_SYNC, tx);
}

void SyncClient::sendIncrementalUpdate(pb::Container &tx)
{
    sendPubMessage(pb::MT_INCREMENTAL_UPDATE, tx);
}

void SyncClient::fsmDown()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "State DOWN");
#endif
    m_state = Down;
    emit stateChanged(m_state);
}

void SyncClient::fsmDownStartEvent()
{
    if (m_state == Down)
    {
#ifdef QT_DEBUG
        DEBUG_TAG(1, m_debugName, "Event START");
#endif
        // handle state change
        emit fsmDownExited(QPrivateSignal());
        fsmTrying();
        emit fsmTryingEntered(QPrivateSignal());
        // execute actions
        startSyncChannel();
        startPubChannel();
     }
}

void SyncClient::fsmTrying()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "State TRYING");
#endif
    m_state = Trying;
    emit stateChanged(m_state);
}

void SyncClient::fsmTryingSyncStateUpEvent()
{
    if (m_state == Trying)
    {
#ifdef QT_DEBUG
        DEBUG_TAG(1, m_debugName, "Event SYNC STATE UP");
#endif
        // handle state change
        emit fsmTryingExited(QPrivateSignal());
        fsmSyncing();
        emit fsmSyncingEntered(QPrivateSignal());
        // execute actions
        sendSync();
        startSubChannel();
     }
}

void SyncClient::fsmTryingStopEvent()
{
    if (m_state == Trying)
    {
#ifdef QT_DEBUG
        DEBUG_TAG(1, m_debugName, "Event STOP");
#endif
        // handle state change
        emit fsmTryingExited(QPrivateSignal());
        fsmDown();
        emit fsmDownEntered(QPrivateSignal());
        // execute actions
        stopSyncChannel();
        stopSubChannel();
        stopPubChannel();
     }
}

void SyncClient::fsmSyncing()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "State SYNCING");
#endif
    m_state = Syncing;
    emit stateChanged(m_state);
}

void SyncClient::fsmSyncingSyncStateTryingEvent()
{
    if (m_state == Syncing)
    {
#ifdef QT_DEBUG
        DEBUG_TAG(1, m_debugName, "Event SYNC STATE TRYING");
#endif
        // handle state change
        emit fsmSyncingExited(QPrivateSignal());
        fsmTrying();
        emit fsmTryingEntered(QPrivateSignal());
        // execute actions
        stopSubChannel();
     }
}

void SyncClient::fsmSyncingSubStateUpEvent()
{
    if (m_state == Syncing)
    {
#ifdef QT_DEBUG
        DEBUG_TAG(1, m_debugName, "Event SUB STATE UP");
#endif
        // handle state change
        emit fsmSyncingExited(QPrivateSignal());
        fsmSynced();
        emit fsmSyncedEntered(QPrivateSignal());
        // execute actions
        synced();
     }
}

void SyncClient::fsmSyncingStopEvent()
{
    if (m_state == Syncing)
    {
#ifdef QT_DEBUG
        DEBUG_TAG(1, m_debugName, "Event STOP");
#endif
        // handle state change
        emit fsmSyncingExited(QPrivateSignal());
        fsmDown();
        emit fsmDownEntered(QPrivateSignal());
        // execute actions
        stopSyncChannel();
        stopSubChannel();
        stopPubChannel();
     }
}

void SyncClient::fsmSynced()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, m_debugName, "State SYNCED");
#endif
    m_state = Synced;
    emit stateChanged(m_state);
}

void SyncClient::fsmSyncedSubStateTryingEvent()
{
    if (m_state == Synced)
    {
#ifdef QT_DEBUG
        DEBUG_TAG(1, m_debugName, "Event SUB STATE TRYING");
#endif
        // handle state change
        emit fsmSyncedExited(QPrivateSignal());
        fsmSyncing();
        emit fsmSyncingEntered(QPrivateSignal());
        // execute actions
        sendSync();
     }
}

void SyncClient::fsmSyncedSyncStateTryingEvent()
{
    if (m_state == Synced)
    {
#ifdef QT_DEBUG
        DEBUG_TAG(1, m_debugName, "Event SYNC STATE TRYING");
#endif
        // handle state change
        emit fsmSyncedExited(QPrivateSignal());
        fsmTrying();
        emit fsmTryingEntered(QPrivateSignal());
        // execute actions
        stopSubChannel();
     }
}

void SyncClient::fsmSyncedStopEvent()
{
    if (m_state == Synced)
    {
#ifdef QT_DEBUG
        DEBUG_TAG(1, m_debugName, "Event STOP");
#endif
        // handle state change
        emit fsmSyncedExited(QPrivateSignal());
        fsmDown();
        emit fsmDownEntered(QPrivateSignal());
        // execute actions
        stopSyncChannel();
        stopSubChannel();
        stopPubChannel();
     }
}

void SyncClient::syncChannelStateChanged(machinetalk::RpcClient::State state)
{

    if (state == machinetalk::RpcClient::Trying)
    {
        if (m_state == Syncing)
        {
            emit fsmSyncingSyncStateTrying(QPrivateSignal());
        }
        if (m_state == Synced)
        {
            emit fsmSyncedSyncStateTrying(QPrivateSignal());
        }
    }

    if (state == machinetalk::RpcClient::Up)
    {
        if (m_state == Trying)
        {
            emit fsmTryingSyncStateUp(QPrivateSignal());
        }
    }
}

void SyncClient::subChannelStateChanged(machinetalk::Subscribe::State state)
{

    if (state == machinetalk::Subscribe::Trying)
    {
        if (m_state == Synced)
        {
            emit fsmSyncedSubStateTrying(QPrivateSignal());
        }
    }

    if (state == machinetalk::Subscribe::Up)
    {
        if (m_state == Syncing)
        {
            emit fsmSyncingSubStateUp(QPrivateSignal());
        }
    }
}

/** start trigger function */
void SyncClient::start()
{
    if (m_state == Down) {
        emit fsmDownStart(QPrivateSignal());
    }
}

/** stop trigger function */
void SyncClient::stop()
{
    if (m_state == Trying) {
        emit fsmTryingStop(QPrivateSignal());
    }
    if (m_state == Syncing) {
        emit fsmSyncingStop(QPrivateSignal());
    }
    if (m_state == Synced) {
        emit fsmSyncedStop(QPrivateSignal());
    }
}
}; // namespace machinetalk
