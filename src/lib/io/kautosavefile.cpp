/*  This file is part of the KDE libraries
    Copyright (c) 2006 Jacob R Rideout <kde@jacobrideout.net>
    Copyright (c) 2015 Nick Shaforostoff <shafff@ukr.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kautosavefile.h"

#include <stdio.h> // for FILENAME_MAX

#include <QLatin1Char>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLockFile>
#include <QStandardPaths>
#include "krandom.h"
#include "kcoreaddons_debug.h"

class KAutoSaveFilePrivate
{
public:
    enum {NamePadding=8};

    KAutoSaveFilePrivate()
        : lock(nullptr),
          managedFileNameChanged(false)
    {}

    QString tempFileName();
    QUrl managedFile;
    QLockFile *lock;
    bool managedFileNameChanged;
};

static QStringList findAllStales(const QString &appName)
{
    const QStringList dirs = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    QStringList files;

    for (const QString &dir : dirs) {
        QDir appDir(dir + QLatin1String("/stalefiles/") + appName);
        //qCDebug(KCOREADDONS_DEBUG) << "Looking in" << appDir.absolutePath();
        const auto listFiles = appDir.entryList(QDir::Files);
        for (const QString &file : listFiles) {
            files << (appDir.absolutePath() + QLatin1Char('/') + file);
        }
    }
    return files;
}

QString KAutoSaveFilePrivate::tempFileName()
{
    // Note: we drop any query string and user/pass info
    const QString protocol(managedFile.scheme());
    const QString path(managedFile.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).path());
    QString name(managedFile.fileName());

    // Remove any part of the path to the right if it is longer than the max file size and
    // ensure that the max filesize takes into account the other parts of the tempFileName
    // Subtract 1 for the _ char, 3 for the padding separator, 5 is for the .lock
    int pathLengthLimit = FILENAME_MAX - NamePadding - name.size() - protocol.size() - 9;

    QString junk = KRandom::randomString(NamePadding);
    // tempName = fileName + junk.truncated + protocol + _ + path.truncated + junk
    // This is done so that the separation between the filename and path can be determined
    name += junk.rightRef(3) + protocol + QLatin1Char('_') + path.leftRef(pathLengthLimit) + junk;

    return QString::fromLatin1(QUrl::toPercentEncoding(name).constData());
}

KAutoSaveFile::KAutoSaveFile(const QUrl &filename, QObject *parent)
    : QFile(parent),
      d(new KAutoSaveFilePrivate)
{
    setManagedFile(filename);
}

KAutoSaveFile::KAutoSaveFile(QObject *parent)
    : QFile(parent),
      d(new KAutoSaveFilePrivate)
{

}

KAutoSaveFile::~KAutoSaveFile()
{
    releaseLock();
    delete d->lock;
    delete d;
}

QUrl KAutoSaveFile::managedFile() const
{
    return d->managedFile;
}

void KAutoSaveFile::setManagedFile(const QUrl &filename)
{
    releaseLock();

    d->managedFile = filename;
    d->managedFileNameChanged = true;
}

void KAutoSaveFile::releaseLock()
{
    if (d->lock && d->lock->isLocked()) {
        delete d->lock;
        d->lock = nullptr;
        if (!fileName().isEmpty()) {
            remove();
        }
    }
}

bool KAutoSaveFile::open(OpenMode openmode)
{
    if (d->managedFile.isEmpty()) {
        return false;
    }

    QString tempFile;
    if (d->managedFileNameChanged) {
        QString staleFilesDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) +
                                QLatin1String("/stalefiles/") + QCoreApplication::instance()->applicationName();
        if (!QDir().mkpath(staleFilesDir)) {
            return false;
        }
        tempFile = staleFilesDir + QChar::fromLatin1('/') + d->tempFileName();
    } else {
        tempFile = fileName();
    }

    d->managedFileNameChanged = false;

    setFileName(tempFile);

    if (QFile::open(openmode)) {

        if (!d->lock)
        {
            d->lock = new QLockFile(tempFile + QLatin1String(".lock"));
            d->lock->setStaleLockTime(60 * 1000); // HARDCODE, 1 minute
        }

        if (d->lock->isLocked() || d->lock->tryLock()) {
            return true;
        } else {
            qCWarning(KCOREADDONS_DEBUG)<<"Could not lock file:"<<tempFile;
            close();
        }
    }

    return false;
}

static QUrl extractManagedFilePath(const QString& staleFileName)
{
    const QStringRef sep = staleFileName.rightRef(3);
    int sepPos = staleFileName.indexOf(sep);
    int pathPos = staleFileName.indexOf(QChar::fromLatin1('_'), sepPos);
    QUrl managedFileName;
    //name.setScheme(file.mid(sepPos + 3, pathPos - sep.size() - 3));
    QByteArray encodedPath = staleFileName.midRef(pathPos+1, staleFileName.length()-pathPos-1-KAutoSaveFilePrivate::NamePadding).toLatin1();
    managedFileName.setPath(QUrl::fromPercentEncoding(encodedPath) + QLatin1Char('/') + QFileInfo(QUrl::fromPercentEncoding(staleFileName.leftRef(sepPos).toLatin1())).fileName());
    return managedFileName;
}

QList<KAutoSaveFile *> KAutoSaveFile::staleFiles(const QUrl &filename, const QString &applicationName)
{
    QString appName(applicationName);
    if (appName.isEmpty()) {
        appName = QCoreApplication::instance()->applicationName();
    }

    // get stale files
    const QStringList files = findAllStales(appName);

    QList<KAutoSaveFile *> list;

    // contruct a KAutoSaveFile for stale files corresponding given filename
    for (const QString &file : files) {
        if (file.endsWith(QLatin1String(".lock")) || (!filename.isEmpty() && extractManagedFilePath(file).path()!=filename.path())) {
            continue;
        }

        // sets managedFile
        KAutoSaveFile *asFile = new KAutoSaveFile(filename.isEmpty()?extractManagedFilePath(file):filename);
        asFile->setFileName(file);
        asFile->d->managedFileNameChanged = false; // do not regenerate tempfile name
        list.append(asFile);
    }

    return list;
}

QList<KAutoSaveFile *> KAutoSaveFile::allStaleFiles(const QString &applicationName)
{
    return staleFiles(QUrl(), applicationName);
}

#include "moc_kautosavefile.cpp"
