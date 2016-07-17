/****************************************************************************
**
** Copyright (C) 2014 Alexander Rössler
** License: LGPL version 2.1
**
** This file is part of QtQuickVcp.
**
** All rights reserved. This program and the accompanying materials
** are made available under the terms of the GNU Lesser General Public License
** (LGPL) version 2.1 which accompanies this distribution, and is available at
** http://www.gnu.org/licenses/lgpl-2.1.html
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** Lesser General Public License for more details.
**
** Contributors:
** Alexander Rössler @ The Cool Tool GmbH <mail DOT aroessler AT gmail DOT com>
**
****************************************************************************/

#include "qpreviewclient.h"
#include "debughelper.h"

QPreviewClient::QPreviewClient(QObject *parent) :
    AbstractServiceImplementation(parent),
    m_statusUri(""),
    m_previewUri(""),
    m_connectionState(Disconnected),
    m_connected(false),
    m_error(NoError),
    m_errorString(""),
    m_model(NULL),
    m_interpreterState(InterpreterStateUnset),
    m_interpreterNote(""),
    m_units(CanonUnitsInch),
    m_convertFactor(1.0),
    m_context(NULL),
    m_statusSocket(NULL),
    m_previewSocket(NULL),
    m_previewUpdated(false)
{
    m_previewStatus.fileName = "test.ngc";
    m_previewStatus.lineNumber = 0;
}

void QPreviewClient::setUnits(QPreviewClient::CanonUnits arg)
{
    if (m_units == arg)
        return;

    switch (arg) {
    case CanonUnitsInch:
        m_convertFactor = 1.0;
        break;
    case CanonUnitsMm:
        m_convertFactor = 25.4;
        break;
    case CanonUnitsCm:
        m_convertFactor = 2.54;
        break;
    default:
        m_convertFactor = 1.0;
    }

    m_units = arg;
    emit unitsChanged(arg);
}

void QPreviewClient::start()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, "preview", "start")
        #endif
            updateState(Connecting);

    if (connectSockets())
    {
        m_statusSocket->subscribeTo("status");
        m_previewSocket->subscribeTo("preview");
        updateState(Connected); //TODO: add something like a ping
    }
}

void QPreviewClient::stop()
{
#ifdef QT_DEBUG
    DEBUG_TAG(1, "preview", "stop")
#endif

    cleanup();
    updateState(Disconnected);  // clears the error
}

void QPreviewClient::cleanup()
{
    disconnectSockets();
}

void QPreviewClient::updateState(QPreviewClient::State state)
{
    updateState(state, NoError, "");
}

void QPreviewClient::updateState(QPreviewClient::State state, QPreviewClient::ConnectionError error, QString errorString)
{
    if (state != m_connectionState)
    {
        if (m_connected != (state == Connected)) {
            m_connected = (state == Connected);
            emit connectedChanged(m_connected);
        }

        m_connectionState = state;
        emit connectionStateChanged(m_connectionState);
    }

    updateError(error, errorString);
}

void QPreviewClient::updateError(QPreviewClient::ConnectionError error, QString errorString)
{
    if (m_errorString != errorString)
    {
        m_errorString = errorString;
        emit errorStringChanged(m_errorString);
    }

    if (m_error != error)
    {
        if (error != NoError)
        {
            cleanup();
        }
        m_error = error;
        emit errorChanged(m_error);
    }
}

double QPreviewClient::convertValue(double value)
{
    return value * m_convertFactor;
}

void QPreviewClient::convertPos(pb::Position *position)
{
    position->set_x(convertValue(position->x()));
    position->set_y(convertValue(position->y()));
    position->set_z(convertValue(position->z()));
    position->set_a(convertValue(position->a()));
    position->set_b(convertValue(position->b()));
    position->set_c(convertValue(position->c()));
    position->set_u(convertValue(position->u()));
    position->set_v(convertValue(position->v()));
    position->set_w(convertValue(position->w()));
}

/** Processes all message received on the status 0MQ socket */
void QPreviewClient::statusMessageReceived(QList<QByteArray> messageList)
{
    QByteArray topic;

    topic = messageList.at(0);
    m_rx.ParseFromArray(messageList.at(1).data(), messageList.at(1).size());

    #ifdef QT_DEBUG
        std::string s;
        gpb::TextFormat::PrintToString(m_rx, &s);
        DEBUG_TAG(3, "preview", "status update" << topic << QString::fromStdString(s))
    #endif

    if (m_rx.type() == pb::MT_INTERP_STAT)
    {
        QStringList notes;

        m_interpreterState = (InterpreterState)m_rx.interp_state();
        for (int i = 0; i< m_rx.note_size(); ++i)
        {
            notes.append(QString::fromStdString(m_rx.note(i)));
        }
        m_interpreterNote = notes.join("\n");

        emit interpreterNoteChanged(m_interpreterNote);
        emit interpreterStateChanged(m_interpreterState);

        if ((m_interpreterState == InterpreterIdle)
                && m_previewUpdated
                && m_model)
        {
            m_model->endUpdate();
        }
    }
}

/** Processes all message received on the preview 0MQ socket */
void QPreviewClient::previewMessageReceived(QList<QByteArray> messageList)
{
    QByteArray topic;

    topic = messageList.at(0);
    m_rx.ParseFromArray(messageList.at(1).data(), messageList.at(1).size());

    #ifdef QT_DEBUG
        std::string s;
        gpb::TextFormat::PrintToString(m_rx, &s);
        DEBUG_TAG(3, "preview", "preview update" << topic << QString::fromStdString(s))
    #endif

    if (m_rx.type() == pb::MT_PREVIEW)
    {
        if (m_model == NULL)
        {
            return;
        }

        for (int i = 0; i < m_rx.preview_size(); ++i)
        {
            QList<pb::Preview> *previewList;
            pb::Preview preview;

            preview = m_rx.preview(i);

            if (preview.has_line_number())
            {
                m_previewStatus.lineNumber = preview.line_number();
            }

            if (preview.has_filename())
            {
                m_previewStatus.fileName = QString::fromStdString(preview.filename());
            }

            if (preview.has_pos())
            {
                convertPos(preview.mutable_pos());  // messages come always with unit inch
            }

            if (preview.has_first_axis())
            {
                preview.set_first_axis(convertValue(preview.first_axis()));
            }

            if (preview.has_second_axis())
            {
                preview.set_second_axis(convertValue(preview.second_axis()));
            }

            if (preview.has_first_end())
            {
                preview.set_first_end(convertValue(preview.first_end()));
            }

            if (preview.has_second_end())
            {
                preview.set_second_end(convertValue(preview.second_end()));
            }

            if (preview.has_axis_end_point())
            {
                preview.set_axis_end_point(convertValue(preview.axis_end_point()));
            }

            previewList = static_cast<QList<pb::Preview>*>(m_model->data(m_previewStatus.fileName,
                                                                         m_previewStatus.lineNumber,
                                                                         QGCodeProgramModel::PreviewRole).value<void*>());
            if (previewList == NULL)
            {
                previewList = new QList<pb::Preview>();
            }

            previewList->append(preview);

            m_model->setData(m_previewStatus.fileName, m_previewStatus.lineNumber,
                             QVariant::fromValue(static_cast<void*>(previewList)),
                             QGCodeProgramModel::PreviewRole);

            m_previewUpdated = true;
        }
    }
}

void QPreviewClient::pollError(int errorNum, const QString &errorMsg)
{
    QString errorString;
    errorString = QString("Error %1: ").arg(errorNum) + errorMsg;
    updateState(Error, SocketError, errorString);
}

/** Connects the 0MQ sockets */
bool QPreviewClient::connectSockets()
{
    m_context = new PollingZMQContext(this, 1);
    connect(m_context, SIGNAL(pollError(int,QString)),
            this, SLOT(pollError(int,QString)));
    m_context->start();

    m_statusSocket = m_context->createSocket(ZMQSocket::TYP_SUB, this);
    m_statusSocket->setLinger(0);

    m_previewSocket = m_context->createSocket(ZMQSocket::TYP_SUB, this);
    m_previewSocket->setLinger(0);

    try {
        m_statusSocket->connectTo(m_statusUri);
        m_previewSocket->connectTo(m_previewUri);
    }
    catch (const zmq::error_t &e) {
        QString errorString;
        errorString = QString("Error %1: ").arg(e.num()) + QString(e.what());
        updateState(Error, SocketError, errorString);
        return false;
    }

    connect(m_statusSocket, SIGNAL(messageReceived(QList<QByteArray>)),
            this, SLOT(statusMessageReceived(QList<QByteArray>)));
    connect(m_previewSocket, SIGNAL(messageReceived(QList<QByteArray>)),
            this, SLOT(previewMessageReceived(QList<QByteArray>)));

#ifdef QT_DEBUG
    DEBUG_TAG(1, "preview", "sockets connected" << m_statusUri << m_previewUri)
#endif

    return true;
}

/** Disconnects the 0MQ sockets */
void QPreviewClient::disconnectSockets()
{
    if (m_statusSocket != NULL)
    {
        m_statusSocket->close();
        m_statusSocket->deleteLater();
        m_statusSocket = NULL;
    }

    if (m_previewSocket != NULL)
    {
        m_previewSocket->close();
        m_previewSocket->deleteLater();
        m_previewSocket = NULL;
    }

    if (m_context != NULL)
    {
        m_context->stop();
        m_context->deleteLater();
        m_context = NULL;
    }
}
