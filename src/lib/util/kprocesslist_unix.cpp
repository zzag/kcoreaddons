/**************************************************************************
**
** This file is part of the KDE Frameworks
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (c) 2019 David Hallas <david@davidhallas.dk>
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "kprocesslist.h"

#include <QProcess>
#include <QDir>

using namespace KProcessList;

namespace {

bool isUnixProcessId(const QString &procname)
{
    for (int i = 0; i != procname.size(); ++i) {
        if (!procname.at(i).isDigit())
            return false;
    }
    return true;
}

// Determine UNIX processes by running ps
KProcessInfoList unixProcessListPS()
{
    KProcessInfoList rc;
    QProcess psProcess;
    const QStringList args {
        QStringLiteral("-e"),
        QStringLiteral("-o"),
#ifdef Q_OS_MAC
    // command goes last, otherwise it is cut off
        QStringLiteral("pid state user comm command"),
#else
        QStringLiteral("pid,state,user,comm,cmd"),
#endif
    };
    psProcess.start(QStringLiteral("ps"), args);
    if (!psProcess.waitForStarted())
        return rc;
    psProcess.waitForFinished();
    QByteArray output = psProcess.readAllStandardOutput();
    // Split "457 S+   /Users/foo.app"
    const QStringList lines = QString::fromLocal8Bit(output).split(QLatin1Char('\n'));
    const int lineCount = lines.size();
    const QChar blank = QLatin1Char(' ');
    for (int l = 1; l < lineCount; l++) { // Skip header
        const QString line = lines.at(l).simplified();
        // we can't just split on blank as the process name might
        // contain them
        const int endOfPid = line.indexOf(blank);
        const int endOfState = line.indexOf(blank, endOfPid+1);
        const int endOfUser = line.indexOf(blank, endOfState+1);
        const int endOfName = line.indexOf(blank, endOfUser+1);

        if (endOfPid >= 0 && endOfState >= 0 && endOfUser >= 0) {
            qint64 pid = line.leftRef(endOfPid).toUInt();
            QString user = line.mid(endOfState+1, endOfUser-endOfState-1);
            QString name = line.mid(endOfUser+1, endOfName-endOfUser-1);
            QString command = line.right(line.size()-endOfName-1);
            rc.push_back(KProcessInfo(pid, command, name, user));
        }
    }

    return rc;
}

} // unnamed namespace

// Determine UNIX processes by reading "/proc". Default to ps if
// it does not exist
KProcessInfoList KProcessList::processInfoList()
{
    const QDir procDir(QStringLiteral("/proc/"));
#ifdef Q_OS_FREEBSD
    QString statusFileName(QStringLiteral("/status"));
#else
    QString statusFileName(QStringLiteral("/stat"));
#endif
    if (!procDir.exists())
        return unixProcessListPS();
    KProcessInfoList rc;
    const QStringList procIds = procDir.entryList();
    if (procIds.isEmpty())
        return rc;
    for (const QString &procId : procIds) {
        if (!isUnixProcessId(procId))
            continue;
        QString filename = QStringLiteral("/proc/");
        filename += procId;
        filename += statusFileName;
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly))
            continue;     // process may have exited

        const QStringList data = QString::fromLocal8Bit(file.readAll()).split(QLatin1Char(' '));
        qint64 pid = procId.toUInt();
        QString name = data.at(1);
        if (name.startsWith(QLatin1Char('(')) && name.endsWith(QLatin1Char(')'))) {
            name.chop(1);
            name.remove(0, 1);
        }
        // State is element 2
        // PPID is element 3
        QString user = QFileInfo(file).owner();
        file.close();

        QString command = name;

        QFile cmdFile(QLatin1String("/proc/") + procId + QLatin1String("/cmdline"));
        if (cmdFile.open(QFile::ReadOnly)) {
            QByteArray cmd = cmdFile.readAll();

            if (!cmd.isEmpty()) {
                // extract non-truncated name from cmdline
                int zeroIndex = cmd.indexOf('\0');
                int processNameStart = cmd.lastIndexOf('/', zeroIndex);
                if (processNameStart == -1) {
                    processNameStart = 0;
                } else {
                    processNameStart++;
                }
                name = QString::fromLocal8Bit(cmd.mid(processNameStart, zeroIndex - processNameStart));

                cmd.replace('\0', ' ');
                command = QString::fromLocal8Bit(cmd).trimmed();
            }
        }
        cmdFile.close();

        rc.push_back(KProcessInfo(pid, command, name, user));
    }
    return rc;
}
