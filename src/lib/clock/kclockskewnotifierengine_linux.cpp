/*
    Copyright (C) 2019 Vlad Zahorodnii <vladzzag@gmail.com>

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

#include "kclockskewnotifierengine_linux_p.h"
#include "kcoreaddons_debug.h"

#include <QSocketNotifier>

#include <fcntl.h>
#include <sys/timerfd.h>
#include <unistd.h>

KLinuxClockSkewNotifierEngine *KLinuxClockSkewNotifierEngine::create(QObject *parent)
{
    const int fd = timerfd_create(CLOCK_REALTIME, O_CLOEXEC | O_NONBLOCK);
    if (fd == -1) {
        qCWarning(KCOREADDONS_DEBUG, "Couldn't create clock skew notifier engine: %s", strerror(errno));
        return nullptr;
    }

    const itimerspec spec = {};
    const int ret = timerfd_settime(fd, TFD_TIMER_ABSTIME | TFD_TIMER_CANCEL_ON_SET, &spec, nullptr);
    if (ret == -1) {
        qCWarning(KCOREADDONS_DEBUG, "Couldn't create clock skew notifier engine: %s", strerror(errno));
        close(fd);
        return nullptr;
    }

    return new KLinuxClockSkewNotifierEngine(fd, parent);
}

KLinuxClockSkewNotifierEngine::KLinuxClockSkewNotifierEngine(int fd, QObject *parent)
    : KClockSkewNotifierEngine(parent)
    , m_fd(fd)
{
    const QSocketNotifier *notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
    connect(notifier, &QSocketNotifier::activated, this, &KLinuxClockSkewNotifierEngine::handleTimerCancelled);
}

KLinuxClockSkewNotifierEngine::~KLinuxClockSkewNotifierEngine()
{
    close(m_fd);
}

void KLinuxClockSkewNotifierEngine::handleTimerCancelled()
{
    uint64_t expirationCount;
    read(m_fd, &expirationCount, sizeof(expirationCount));

    emit skewed();
}
