/* Private Header for class of KDirWatchPrivate
 *
 * this separate header file is needed for MOC processing
 * because KDirWatchPrivate has signals and slots
 *
 * This file is part of the KDE libraries
 * Copyright (C) 1998 Sven Radej <sven@lisa.exp.univie.ac.at>
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2007 Flavio Castelli <flavio.castelli@gmail.com>
 * Copyright (C) 2008 Jarosław Staniek <staniek@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KDIRWATCH_P_H
#define KDIRWATCH_P_H

#include <io/config-kdirwatch.h>
#include "kdirwatch.h"

#ifndef QT_NO_FILESYSTEMWATCHER
#define HAVE_QFILESYSTEMWATCHER 1
#else
#define HAVE_QFILESYSTEMWATCHER 0
#endif

#include <QList>
#include <QSet>
#include <QMap>
#include <QObject>
#include <QString>
#include <QTimer>
class QSocketNotifier;

#if HAVE_FAM
#include <limits.h>
#include <fam.h>
#endif

#include <sys/types.h> // time_t, ino_t
#include <ctime>

#define invalid_ctime (static_cast<time_t>(-1))

#if HAVE_QFILESYSTEMWATCHER
#include <QFileSystemWatcher>
#endif // HAVE_QFILESYSTEMWATCHER

/* KDirWatchPrivate is a singleton and does the watching
 * for every KDirWatch instance in the application.
 */
class KDirWatchPrivate : public QObject
{
    Q_OBJECT
public:

    enum entryStatus { Normal = 0, NonExistent };
    enum entryMode { UnknownMode = 0, StatMode, INotifyMode, FAMMode, QFSWatchMode };
    enum { NoChange = 0, Changed = 1, Created = 2, Deleted = 4 };

    struct Client {
        Client(KDirWatch *inst, KDirWatch::WatchModes watchModes)
            : instance(inst),
            count(1),
            watchingStopped(inst->isStopped()),
            pending(NoChange),
            m_watchModes(watchModes)
        {}

        // The compiler needs a copy ctor for Client when Entry is inserted into m_mapEntries
        // (even though the vector of clients is empty at that point, so no performance penalty there)
        //Client(const Client &) = delete;
        //Client &operator=(const Client &) = delete;
        //Client(Client &&) = default;
        //Client &operator=(Client &&) = default;

        KDirWatch *instance;
        int count;
        // did the instance stop watching
        bool watchingStopped;
        // events blocked when stopped
        int pending;
        KDirWatch::WatchModes m_watchModes;
    };

    class Entry
    {
    public:
        ~Entry();
        // instances interested in events
        std::vector<Client> m_clients;
        // nonexistent entries of this directory
        QList<Entry *> m_entries;
        QString path;

        // the last observed modification time
        time_t m_ctime;
        // last observed inode
        ino_t m_ino;
        // the last observed link count
        int m_nlink;
        entryStatus m_status;
        entryMode m_mode;
        int msecLeft, freq;
        bool isDir;

        QString parentDirectory() const;
        void addClient(KDirWatch *, KDirWatch::WatchModes);
        void removeClient(KDirWatch *);
        int clientCount() const;
        bool isValid()
        {
            return !m_clients.empty() || !m_entries.empty();
        }

        Entry *findSubEntry(const QString &path) const
        {
            for (Entry *sub_entry : qAsConst(m_entries)) {
                if (sub_entry->path == path) {
                    return sub_entry;
                }
            }
            return nullptr;
        }

        bool dirty;
        void propagate_dirty();

        QList<const Client *> clientsForFileOrDir(const QString &tpath, bool *isDir) const;
        QList<const Client *> inotifyClientsForFileOrDir(bool isDir) const;

#if HAVE_FAM
        FAMRequest fr;
        bool m_famReportedSeen;
#endif

#if HAVE_SYS_INOTIFY_H
        int wd;
        // Creation and Deletion of files happens infrequently, so
        // can safely be reported as they occur.  File changes i.e. those that emit "dirty()" can
        // happen many times per second, though, so maintain a list of files in this directory
        // that can be emitted and flushed at the next slotRescan(...).
        // This will be unused if the Entry is not a directory.
        QList<QString> m_pendingFileChanges;
#endif
    };

    typedef QMap<QString, Entry> EntryMap;

    KDirWatchPrivate();
    ~KDirWatchPrivate();

    void resetList(KDirWatch *instance, bool skippedToo);
    void useFreq(Entry *e, int newFreq);
    void addEntry(KDirWatch *instance, const QString &_path, Entry *sub_entry,
                  bool isDir, KDirWatch::WatchModes watchModes = KDirWatch::WatchDirOnly);
    void removeEntry(KDirWatch *instance, const QString &path, Entry *sub_entry);
    void removeEntry(KDirWatch *instance, Entry *e, Entry *sub_entry);
    bool stopEntryScan(KDirWatch *instance, Entry *e);
    bool restartEntryScan(KDirWatch *instance, Entry *e, bool notify);
    void stopScan(KDirWatch *instance);
    void startScan(KDirWatch *instance, bool notify, bool skippedToo);

    void removeEntries(KDirWatch *instance);
    void statistics();

    void addWatch(Entry *entry);
    void removeWatch(Entry *entry);
    Entry *entry(const QString &_path);
    int scanEntry(Entry *e);
    void emitEvent(Entry *e, int event, const QString &fileName = QString());

    static bool isNoisyFile(const char *filename);

public Q_SLOTS:
    void slotRescan();
    void famEventReceived(); // for FAM
    void inotifyEventReceived(); // for inotify
    void slotRemoveDelayed();
    void fswEventReceived(const QString &path);  // for QFileSystemWatcher

public:
    QTimer timer;
    EntryMap m_mapEntries;

    KDirWatch::Method m_preferredMethod, m_nfsPreferredMethod;
    int freq;
    int statEntries;
    int m_nfsPollInterval, m_PollInterval;
    bool useStat(Entry *e);

    // removeList is allowed to contain any entry at most once
    QSet<Entry *> removeList;
    bool delayRemove;

    bool rescan_all;
    QTimer rescan_timer;

#if HAVE_FAM
    QSocketNotifier *sn;
    FAMConnection fc;
    bool use_fam;

    void checkFAMEvent(FAMEvent *fe);
    bool useFAM(Entry *e);
#endif

#if HAVE_SYS_INOTIFY_H
    QSocketNotifier *mSn;
    bool supports_inotify;
    int m_inotify_fd;
    QHash<int, Entry *> m_inotify_wd_to_entry;

    bool useINotify(Entry *e);
#endif
#if HAVE_QFILESYSTEMWATCHER
    QFileSystemWatcher *fsWatcher;
    bool useQFSWatch(Entry *e);
#endif

    bool _isStopped;
};

QDebug operator<<(QDebug debug, const KDirWatchPrivate::Entry &entry);

#endif // KDIRWATCH_P_H

