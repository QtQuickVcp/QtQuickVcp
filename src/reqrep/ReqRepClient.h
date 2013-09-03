// Copyright 2011-2013 Johann Duscher (a.k.a. Jonny Dee). All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
//    1. Redistributions of source code must retain the above copyright notice, this list of
//       conditions and the following disclaimer.
//
//    2. Redistributions in binary form must reproduce the above copyright notice, this list
//       of conditions and the following disclaimer in the documentation and/or other materials
//       provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY JOHANN DUSCHER ''AS IS'' AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
// ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// The views and conclusions contained in the software and documentation are those of the
// authors and should not be interpreted as representing official policies, either expressed
// or implied, of Johann Duscher.

#ifndef REQREPCLIENT_H
#define REQREPCLIENT_H

#include "common/SampleBase.h"

#include <nzmqt/nzmqt.hpp>

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QTimer>


namespace nzmqt
{

namespace samples
{

class ReqRepClient : public SampleBase
{
    Q_OBJECT
    typedef SampleBase super;

public:
    explicit ReqRepClient(ZMQContext& context, const QString& address, const QString& requestMsg, QObject *parent)
        : super(parent)
        , address_(address), requestMsg_(requestMsg)
    {
        socket_ = context.createSocket(ZMQSocket::TYP_REQ, this);
    }

protected:
    void runImpl()
    {
        connect(socket_, SIGNAL(messageReceived(const QList<QByteArray>&)), SLOT(replyReceived(const QList<QByteArray>&)));
        socket_->connectTo(address_);

        QTimer::singleShot(1000, this, SLOT(sendRequest()));

        waitUntilStopped();
    }

protected slots:
    void sendRequest()
    {
        static quint64 counter = 0;

        QList<QByteArray> request;
        request += QString("REQUEST[%1: %2]").arg(++counter).arg(QDateTime::currentDateTime().toString(Qt::ISODate)).toLocal8Bit();
        request += requestMsg_.toLocal8Bit();
        qDebug() << "ReqRepClient::sendRequest> " << request;
        socket_->sendMessage(request);
    }

    void replyReceived(const QList<QByteArray>& reply)
    {
        qDebug() << "ReqRepClient::replyReceived> " << reply;

        // Start timer again in order to trigger the next sendRequest() call.
        QTimer::singleShot(1000, this, SLOT(sendRequest()));
    }

private:
    QString address_;
    QString requestMsg_;

    ZMQSocket* socket_;
};

}

}

#endif // REQREPCLIENT_H
