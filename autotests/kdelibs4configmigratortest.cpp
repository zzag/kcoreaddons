/*
 *  Copyright 2014 Montel Laurent <montel@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation;
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

// test object
#include "kdelibs4configmigrator.h"
// Qt
#include <QObject>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>
#include <QStandardPaths>

class Kdelibs4ConfigMigratorTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void shouldNotMigrateIfKde4HomeDirDoesntExist();
    void shouldMigrateIfKde4HomeDirExist();
    void shouldMigrateConfigFiles();
    void shouldMigrateUiFiles();
};

void Kdelibs4ConfigMigratorTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    const QString configHome = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QDir(configHome).removeRecursively();
    const QString dataHome = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QDir(dataHome).removeRecursively();
}

void Kdelibs4ConfigMigratorTest::shouldNotMigrateIfKde4HomeDirDoesntExist()
{
    qputenv("KDEHOME", "");
    Kdelibs4ConfigMigrator migration(QLatin1String("foo"));
    QCOMPARE(migration.migrate(), false);
}

void Kdelibs4ConfigMigratorTest::shouldMigrateIfKde4HomeDirExist()
{
    QTemporaryDir kdehomeDir;
    QVERIFY(kdehomeDir.isValid());
    const QString kdehome = kdehomeDir.path();
    qputenv("KDEHOME", QFile::encodeName(kdehome));
    Kdelibs4ConfigMigrator migration(QLatin1String("foo"));
    QCOMPARE(migration.migrate(), true);
}

void Kdelibs4ConfigMigratorTest::shouldMigrateConfigFiles()
{
    QTemporaryDir kdehomeDir;
    const QString kdehome = kdehomeDir.path();
    qputenv("KDEHOME", QFile::encodeName(kdehome));

    //Generate kde4 config dir
    const QString configPath = kdehome + QLatin1Char('/') + QLatin1String("share/config/");
    QDir().mkpath(configPath);
    QVERIFY(QDir(configPath).exists());

    QStringList listConfig;
    listConfig << QLatin1String("foorc") << QLatin1String("foo1rc");
    for (const QString &config : qAsConst(listConfig)) {
        QFile fooConfigFile(QLatin1String(KDELIBS4CONFIGMIGRATOR_DATA_DIR) + QLatin1Char('/') + config);
        QVERIFY(fooConfigFile.exists());
        const QString storedConfigFilePath = configPath + QLatin1Char('/') + config;
        QVERIFY(QFile::copy(fooConfigFile.fileName(), storedConfigFilePath));
        QCOMPARE(QStandardPaths::locate(QStandardPaths::ConfigLocation, config), QString());
    }

    Kdelibs4ConfigMigrator migration(QLatin1String("foo"));
    migration.setConfigFiles(QStringList() << listConfig);
    QVERIFY(migration.migrate());

    for (const QString &config : qAsConst(listConfig)) {
        const QString migratedConfigFile = QStandardPaths::locate(QStandardPaths::ConfigLocation, config);
        QVERIFY(!migratedConfigFile.isEmpty());
        QVERIFY(QFile(migratedConfigFile).exists());
        QFile::remove(migratedConfigFile);
    }
}

void Kdelibs4ConfigMigratorTest::shouldMigrateUiFiles()
{
    QTemporaryDir kdehomeDir;
    const QString kdehome = kdehomeDir.path();
    qputenv("KDEHOME", QFile::encodeName(kdehome));

    const QString appName = QLatin1String("foo");

    //Generate kde4 data dir
    const QString dataPath = kdehome + QLatin1Char('/') + QLatin1String("share/apps/");
    QDir().mkpath(dataPath);
    QVERIFY(QDir(dataPath).exists());

    const QString xdgDatahome = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);

    QStringList listUi;
    listUi << QLatin1String("appuirc") << QLatin1String("appui1rc");
    for (const QString &uifile : qAsConst(listUi)) {
        QFile fooConfigFile(QLatin1String(KDELIBS4CONFIGMIGRATOR_DATA_DIR) + QLatin1Char('/') + uifile);
        QVERIFY(fooConfigFile.exists());
        QDir().mkpath(dataPath + QLatin1Char('/') + appName);
        const QString storedConfigFilePath = dataPath + QLatin1Char('/') + appName + QLatin1Char('/') + uifile;
        QVERIFY(QFile::copy(fooConfigFile.fileName(), storedConfigFilePath));

        const QString xdgUiFile = xdgDatahome + QLatin1String("/kxmlgui5/") + appName + QLatin1Char('/') + uifile;
        QVERIFY(!QFile::exists(xdgUiFile));
    }

    Kdelibs4ConfigMigrator migration(appName);
    migration.setUiFiles(QStringList() << listUi);
    QVERIFY(migration.migrate());

    for (const QString &uifile : qAsConst(listUi)) {
        const QString xdgUiFile = xdgDatahome + QLatin1String("/kxmlgui5/") + appName + QLatin1Char('/') + uifile;
        QVERIFY(QFile(xdgUiFile).exists());
        QFile::remove(xdgUiFile);
    }
}

QTEST_MAIN(Kdelibs4ConfigMigratorTest)

#include "kdelibs4configmigratortest.moc"

