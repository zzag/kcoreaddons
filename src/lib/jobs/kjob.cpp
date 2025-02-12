/*  This file is part of the KDE project
    Copyright (C) 2000 Stephan Kulow <coolo@kde.org>
                       David Faure <faure@kde.org>
    Copyright (C) 2006 Kevin Ottens <ervin@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/

#include "kjob.h"
#include "kjob_p.h"

#include "kjobuidelegate.h"

#include <QEventLoop>
#include <QTimer>

KJobPrivate::KJobPrivate()
    : q_ptr(nullptr), uiDelegate(nullptr), error(KJob::NoError),
      progressUnit(KJob::Bytes), percentage(0),
      speedTimer(nullptr), eventLoop(nullptr),
      capabilities(KJob::NoCapabilities),
      suspended(false), isAutoDelete(true), isFinished(false)
{
}

KJobPrivate::~KJobPrivate()
{
}

KJob::KJob(QObject *parent)
    : QObject(parent), d_ptr(new KJobPrivate)
{
    d_ptr->q_ptr = this;
}

KJob::KJob(KJobPrivate &dd, QObject *parent)
    : QObject(parent), d_ptr(&dd)
{
    d_ptr->q_ptr = this;
}

KJob::~KJob()
{
    if (!d_ptr->isFinished) {
        emit finished(this, QPrivateSignal());
    }

    delete d_ptr->speedTimer;
    delete d_ptr->uiDelegate;
    delete d_ptr;
}

void KJob::setUiDelegate(KJobUiDelegate *delegate)
{
    Q_D(KJob);
    if (delegate == nullptr || delegate->setJob(this)) {
        delete d->uiDelegate;
        d->uiDelegate = delegate;

        if (d->uiDelegate) {
            d->uiDelegate->connectJob(this);
        }
    }
}

KJobUiDelegate *KJob::uiDelegate() const
{
    return d_func()->uiDelegate;
}

KJob::Capabilities KJob::capabilities() const
{
    return d_func()->capabilities;
}

bool KJob::isSuspended() const
{
    return d_func()->suspended;
}

void KJob::finishJob(bool emitResult)
{
    Q_D(KJob);
    d->isFinished = true;

    if (d->eventLoop) {
        d->eventLoop->quit();
    }

    // If we are displaying a progress dialog, remove it first.
    emit finished(this, QPrivateSignal());

    if (emitResult) {
        emit result(this, QPrivateSignal());
    }

    if (isAutoDelete()) {
        deleteLater();
    }
}

bool KJob::kill(KillVerbosity verbosity)
{
    if (doKill()) {
        setError(KilledJobError);

        finishJob(verbosity != Quietly);
        return true;
    } else {
        return false;
    }
}

bool KJob::suspend()
{
    Q_D(KJob);
    if (!d->suspended) {
        if (doSuspend()) {
            d->suspended = true;
            emit suspended(this, QPrivateSignal());

            return true;
        }
    }

    return false;
}

bool KJob::resume()
{
    Q_D(KJob);
    if (d->suspended) {
        if (doResume()) {
            d->suspended = false;
            emit resumed(this, QPrivateSignal());

            return true;
        }
    }

    return false;
}

bool KJob::doKill()
{
    return false;
}

bool KJob::doSuspend()
{
    return false;
}

bool KJob::doResume()
{
    return false;
}

void KJob::setCapabilities(KJob::Capabilities capabilities)
{
    Q_D(KJob);
    d->capabilities = capabilities;
}

bool KJob::exec()
{
    Q_D(KJob);
    // Usually this job would delete itself, via deleteLater() just after
    // emitting result() (unless configured otherwise). Since we use an event
    // loop below, that event loop will process the deletion event and we'll
    // have been deleted when exec() returns. This crashes, so temporarily
    // suspend autodeletion and manually do it afterwards.
    const bool wasAutoDelete = isAutoDelete();
    setAutoDelete(false);

    Q_ASSERT(! d->eventLoop);

    QEventLoop loop(this);
    d->eventLoop = &loop;

    start();
    if (!d->isFinished) {
        d->eventLoop->exec(QEventLoop::ExcludeUserInputEvents);
    }
    d->eventLoop = nullptr;

    if (wasAutoDelete) {
        deleteLater();
    }
    return (d->error == NoError);
}

int KJob::error() const
{
    return d_func()->error;
}

QString KJob::errorText() const
{
    return d_func()->errorText;
}

QString KJob::errorString() const
{
    return d_func()->errorText;
}

qulonglong KJob::processedAmount(Unit unit) const
{
    return d_func()->processedAmount[unit];
}

qulonglong KJob::totalAmount(Unit unit) const
{
    return d_func()->totalAmount[unit];
}

unsigned long KJob::percent() const
{
    return d_func()->percentage;
}

void KJob::setError(int errorCode)
{
    Q_D(KJob);
    d->error = errorCode;
}

void KJob::setErrorText(const QString &errorText)
{
    Q_D(KJob);
    d->errorText = errorText;
}

void KJob::setProcessedAmount(Unit unit, qulonglong amount)
{
    Q_D(KJob);
    bool should_emit = (d->processedAmount[unit] != amount);

    d->processedAmount[unit] = amount;

    if (should_emit) {
        emit processedAmount(this, unit, amount);
        if (unit == d->progressUnit) {
            emit processedSize(this, amount);
            emitPercent(d->processedAmount[unit], d->totalAmount[unit]);
        }
    }
}

void KJob::setTotalAmount(Unit unit, qulonglong amount)
{
    Q_D(KJob);
    bool should_emit = (d->totalAmount[unit] != amount);

    d->totalAmount[unit] = amount;

    if (should_emit) {
        emit totalAmount(this, unit, amount);
        if (unit == d->progressUnit) {
            emit totalSize(this, amount);
            emitPercent(d->processedAmount[unit], d->totalAmount[unit]);
        }
    }
}

void KJob::setPercent(unsigned long percentage)
{
    Q_D(KJob);
    if (d->percentage != percentage) {
        d->percentage = percentage;
        emit percent(this, percentage);
    }
}

void KJob::emitResult()
{
    finishJob(true);
}

void KJob::emitPercent(qulonglong processedAmount, qulonglong totalAmount)
{
    Q_D(KJob);
    // calculate percents
    if (totalAmount) {
        unsigned long oldPercentage = d->percentage;
        d->percentage = 100.0 * processedAmount / totalAmount;
        if (d->percentage != oldPercentage) {
            emit percent(this, d->percentage);
        }
    }
}

void KJob::emitSpeed(unsigned long value)
{
    Q_D(KJob);
    if (!d->speedTimer) {
        d->speedTimer = new QTimer(this);
        connect(d->speedTimer, SIGNAL(timeout()), SLOT(_k_speedTimeout()));
    }

    emit speed(this, value);
    d->speedTimer->start(5000);   // 5 seconds interval should be enough
}

void KJobPrivate::_k_speedTimeout()
{
    Q_Q(KJob);
    // send 0 and stop the timer
    // timer will be restarted only when we receive another speed event
    emit q->speed(q, 0);
    speedTimer->stop();
}

bool KJob::isAutoDelete() const
{
    Q_D(const KJob);
    return d->isAutoDelete;
}

void KJob::setAutoDelete(bool autodelete)
{
    Q_D(KJob);
    d->isAutoDelete = autodelete;
}

#include "moc_kjob.cpp"
