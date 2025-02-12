/*
 * Copyright 2014 Alex Merry <alex.merry@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <QtTest>
#include <QFileInfo>

#include <kpluginloader.h>
#include <kpluginmetadata.h>

class LibraryPathRestorer {
public:
    explicit LibraryPathRestorer(const QStringList &paths) : mPaths(paths) {}
    ~LibraryPathRestorer() { QCoreApplication::setLibraryPaths(mPaths); }
private:
    QStringList mPaths;
};

class KPluginLoaderTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testFindPlugin_missing()
    {
        const QString location = KPluginLoader::findPlugin(QStringLiteral("idonotexist"));
        QVERIFY2(location.isEmpty(), qPrintable(location));
    }

    void testFindPlugin()
    {
        const QString location = KPluginLoader::findPlugin(QStringLiteral("jsonplugin"));
        QVERIFY2(!location.isEmpty(), qPrintable(location));
    }

    void testPluginVersion()
    {
        KPluginLoader vplugin(QStringLiteral("versionedplugin"));
        QCOMPARE(vplugin.pluginVersion(), quint32(5));

        KPluginLoader vplugin2(QStringLiteral("versionedplugin"));
        QCOMPARE(vplugin2.pluginVersion(), quint32(5));

        KPluginLoader uplugin(QStringLiteral("unversionedplugin"));
        QCOMPARE(uplugin.pluginVersion(), quint32(-1));

        KPluginLoader jplugin(KPluginName(QStringLiteral("jsonplugin")));
        QCOMPARE(jplugin.pluginVersion(), quint32(-1));

        KPluginLoader eplugin(KPluginName::fromErrorString(QStringLiteral("there was an error")));
        QCOMPARE(eplugin.pluginVersion(), quint32(-1));

        KPluginLoader noplugin(QStringLiteral("idonotexist"));
        QCOMPARE(noplugin.pluginVersion(), quint32(-1));
    }

    void testPluginName()
    {
        KPluginLoader vplugin(QStringLiteral("versionedplugin"));
        QCOMPARE(vplugin.pluginName(), QString::fromLatin1("versionedplugin"));

        KPluginLoader jplugin(KPluginName(QStringLiteral("jsonplugin")));
        QCOMPARE(jplugin.pluginName(), QString::fromLatin1("jsonplugin"));

        KPluginLoader eplugin(KPluginName::fromErrorString(QStringLiteral("there was an error")));
        QVERIFY2(eplugin.pluginName().isEmpty(), qPrintable(eplugin.pluginName()));

        KPluginLoader noplugin(QStringLiteral("idonotexist"));
        QCOMPARE(noplugin.pluginName(), QString::fromLatin1("idonotexist"));
    }

    void testFactory()
    {
        KPluginLoader vplugin(QStringLiteral("versionedplugin"));
        QVERIFY(vplugin.factory());

        KPluginLoader jplugin(KPluginName(QStringLiteral("jsonplugin")));
        QVERIFY(jplugin.factory());

        KPluginLoader eplugin(KPluginName::fromErrorString(QStringLiteral("there was an error")));
        QVERIFY(!eplugin.factory());

        KPluginLoader noplugin(QStringLiteral("idonotexist"));
        QVERIFY(!noplugin.factory());
    }

    void testErrorString()
    {
        KPluginLoader eplugin(KPluginName::fromErrorString(QStringLiteral("there was an error")));
        QCOMPARE(eplugin.errorString(), QString::fromLatin1("there was an error"));
    }

    void testFileName()
    {
        KPluginLoader vplugin(QStringLiteral("versionedplugin"));
        QCOMPARE(QFileInfo(vplugin.fileName()).canonicalFilePath(),
                 QFileInfo(QStringLiteral(VERSIONEDPLUGIN_FILE)).canonicalFilePath());

        KPluginLoader jplugin(KPluginName(QStringLiteral("jsonplugin")));
        QCOMPARE(QFileInfo(jplugin.fileName()).canonicalFilePath(),
                 QFileInfo(QStringLiteral(JSONPLUGIN_FILE)).canonicalFilePath());

        KPluginLoader eplugin(KPluginName::fromErrorString(QStringLiteral("there was an error")));
        QVERIFY2(eplugin.fileName().isEmpty(), qPrintable(eplugin.fileName()));

        KPluginLoader noplugin(QStringLiteral("idonotexist"));
        QVERIFY2(noplugin.fileName().isEmpty(), qPrintable(noplugin.fileName()));
    }

    void testInstance()
    {
        KPluginLoader vplugin(QStringLiteral("versionedplugin"));
        QVERIFY(vplugin.instance());

        KPluginLoader jplugin(KPluginName(QStringLiteral("jsonplugin")));
        QVERIFY(jplugin.instance());

        KPluginLoader eplugin(KPluginName::fromErrorString(QStringLiteral("there was an error")));
        QVERIFY(!eplugin.instance());

        KPluginLoader noplugin(QStringLiteral("idonotexist"));
        QVERIFY(!noplugin.instance());
    }

    void testIsLoaded()
    {
        KPluginLoader vplugin(QStringLiteral("versionedplugin"));
        QVERIFY(!vplugin.isLoaded());
        QVERIFY(vplugin.load());
        QVERIFY(vplugin.isLoaded());

        KPluginLoader jplugin(KPluginName(QStringLiteral("jsonplugin")));
        QVERIFY(!jplugin.isLoaded());
        QVERIFY(jplugin.load());
        QVERIFY(jplugin.isLoaded());

        KPluginLoader aplugin(QStringLiteral("alwaysunloadplugin"));
        QVERIFY(!aplugin.isLoaded());
        QVERIFY(aplugin.load());
        QVERIFY(aplugin.isLoaded());
        if (aplugin.unload()) {
            QVERIFY(!aplugin.isLoaded());
        } else {
            qDebug() << "Could not unload alwaysunloadplugin:" << aplugin.errorString();
        }

        KPluginLoader eplugin(KPluginName::fromErrorString(QStringLiteral("there was an error")));
        QVERIFY(!eplugin.isLoaded());
        QVERIFY(!eplugin.load());
        QVERIFY(!eplugin.isLoaded());

        KPluginLoader noplugin(QStringLiteral("idonotexist"));
        QVERIFY(!noplugin.isLoaded());
        QVERIFY(!noplugin.load());
        QVERIFY(!noplugin.isLoaded());
    }

    void testLoad()
    {
        KPluginLoader vplugin(QStringLiteral("versionedplugin"));
        QVERIFY(vplugin.load());

        KPluginLoader jplugin(KPluginName(QStringLiteral("jsonplugin")));
        QVERIFY(jplugin.load());

        KPluginLoader eplugin(KPluginName::fromErrorString(QStringLiteral("there was an error")));
        QVERIFY(!eplugin.load());

        KPluginLoader noplugin(QStringLiteral("idonotexist"));
        QVERIFY(!noplugin.load());
    }

    void testLoadHints()
    {
        KPluginLoader aplugin(QStringLiteral("alwaysunloadplugin"));
        aplugin.setLoadHints(QLibrary::ResolveAllSymbolsHint);
        QCOMPARE(aplugin.loadHints(), QLibrary::ResolveAllSymbolsHint);
    }

    void testMetaData()
    {
        KPluginLoader aplugin(QStringLiteral("alwaysunloadplugin"));
        QJsonObject ametadata = aplugin.metaData();
        QVERIFY(!ametadata.isEmpty());
        QVERIFY(ametadata.keys().contains(QLatin1String("IID")));
        QJsonValue ametadata_metadata = ametadata.value(QStringLiteral("MetaData"));
        QVERIFY(ametadata_metadata.toObject().isEmpty());
        QVERIFY(!aplugin.isLoaded()); // didn't load anything

        KPluginLoader jplugin(KPluginName(QStringLiteral("jsonplugin")));
        QJsonObject jmetadata = jplugin.metaData();
        QVERIFY(!jmetadata.isEmpty());
        QJsonValue jmetadata_metadata = jmetadata.value(QStringLiteral("MetaData"));
        QVERIFY(jmetadata_metadata.isObject());
        QJsonObject jmetadata_obj = jmetadata_metadata.toObject();
        QVERIFY(!jmetadata_obj.isEmpty());
        QJsonValue comment = jmetadata_obj.value(QStringLiteral("KPlugin")).toObject().value(QStringLiteral("Description"));
        QVERIFY(comment.isString());
        QCOMPARE(comment.toString(), QString::fromLatin1("This is a plugin"));

        KPluginLoader eplugin(KPluginName::fromErrorString(QStringLiteral("there was an error")));
        QVERIFY(eplugin.metaData().isEmpty());

        KPluginLoader noplugin(QStringLiteral("idonotexist"));
        QVERIFY(noplugin.metaData().isEmpty());
    }

    void testUnload()
    {
        KPluginLoader aplugin(QStringLiteral("alwaysunloadplugin"));
        QVERIFY(aplugin.load());
        // may need QEXPECT_FAIL on some platforms...
        QVERIFY(aplugin.unload());
    }

    void testInstantiatePlugins()
    {
        const QString plugin1Path = KPluginLoader::findPlugin(QStringLiteral("jsonplugin"));
        QVERIFY2(!plugin1Path.isEmpty(), qPrintable(plugin1Path));
        const QString plugin2Path = KPluginLoader::findPlugin(QStringLiteral("unversionedplugin"));
        QVERIFY2(!plugin2Path.isEmpty(), qPrintable(plugin2Path));
        const QString plugin3Path = KPluginLoader::findPlugin(QStringLiteral("jsonplugin2"));
        QVERIFY2(!plugin3Path.isEmpty(), qPrintable(plugin3Path));

        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY2(QFile::copy(plugin1Path, dir.absoluteFilePath(QFileInfo(plugin1Path).fileName())),
                 qPrintable(dir.absoluteFilePath(QFileInfo(plugin1Path).fileName())));
        QVERIFY2(QFile::copy(plugin2Path, dir.absoluteFilePath(QFileInfo(plugin2Path).fileName())),
                 qPrintable(dir.absoluteFilePath(QFileInfo(plugin2Path).fileName())));
        QVERIFY2(QFile::copy(plugin3Path, dir.absoluteFilePath(QFileInfo(plugin3Path).fileName())),
                 qPrintable(dir.absoluteFilePath(QFileInfo(plugin3Path).fileName())));

        // only jsonplugin, since unversionedplugin has no json metadata
        QList<QObject*> plugins = KPluginLoader::instantiatePlugins(temp.path());
        QCOMPARE(plugins.size(), 2);
        QStringList classNames = QStringList() << QString::fromLatin1(plugins[0]->metaObject()->className())
                << QString::fromLatin1(plugins[1]->metaObject()->className());
        classNames.sort();
        QCOMPARE(classNames[0], QStringLiteral("jsonplugin2"));
        QCOMPARE(classNames[1], QStringLiteral("jsonpluginfa"));
        qDeleteAll(plugins);

        //try filter
        plugins = KPluginLoader::instantiatePlugins(temp.path(), [](const KPluginMetaData & md) {
            return md.pluginId() == QLatin1String("jsonplugin");
        });
        QCOMPARE(plugins.size(), 1);
        QCOMPARE(plugins[0]->metaObject()->className(), "jsonpluginfa");
        qDeleteAll(plugins);

        plugins = KPluginLoader::instantiatePlugins(temp.path(), [](const KPluginMetaData & md) {
            return md.pluginId() == QLatin1String("unversionedplugin");
        });
        QCOMPARE(plugins.size(), 0);

        plugins = KPluginLoader::instantiatePlugins(temp.path(), [](const KPluginMetaData & md) {
            return md.pluginId() == QLatin1String("foobar"); // ID does not match file name, is set in JSON
        });
        QCOMPARE(plugins.size(), 1);
        QCOMPARE(plugins[0]->metaObject()->className(), "jsonplugin2");
        qDeleteAll(plugins);

        // check that parent gets set
        plugins = KPluginLoader::instantiatePlugins(temp.path(), [](const KPluginMetaData&) { return true; }, this);
        QCOMPARE(plugins.size(), 2);
        QCOMPARE(plugins[0]->parent(), this);
        QCOMPARE(plugins[1]->parent(), this);
        qDeleteAll(plugins);

        const QString subDirName = dir.dirName();
        QVERIFY(dir.cdUp()); // should now point to /tmp on Linux
        LibraryPathRestorer restorer(QCoreApplication::libraryPaths());
        // instantiate using relative path
        // make sure library path is set up correctly
        QCoreApplication::setLibraryPaths(QStringList() << dir.absolutePath());
        QVERIFY(!QDir::isAbsolutePath(subDirName));
        plugins = KPluginLoader::instantiatePlugins(subDirName);
        QCOMPARE(plugins.size(), 2);
        classNames = QStringList() << QString::fromLatin1(plugins[0]->metaObject()->className())
                << QString::fromLatin1(plugins[1]->metaObject()->className());
        classNames.sort();
        QCOMPARE(classNames[0], QStringLiteral("jsonplugin2"));
        QCOMPARE(classNames[1], QStringLiteral("jsonpluginfa"));
        qDeleteAll(plugins);
    }

    void testFindPlugins()
    {
        const QString plugin1Path = KPluginLoader::findPlugin(QStringLiteral("jsonplugin"));
        QVERIFY2(!plugin1Path.isEmpty(), qPrintable(plugin1Path));
        const QString plugin2Path = KPluginLoader::findPlugin(QStringLiteral("unversionedplugin"));
        QVERIFY2(!plugin2Path.isEmpty(), qPrintable(plugin2Path));
        const QString plugin3Path = KPluginLoader::findPlugin(QStringLiteral("jsonplugin2"));
        QVERIFY2(!plugin3Path.isEmpty(), qPrintable(plugin3Path));

        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkdir(QStringLiteral("kpluginmetadatatest")));
        QVERIFY(dir.cd(QStringLiteral("kpluginmetadatatest")));
        QVERIFY2(QFile::copy(plugin1Path, dir.absoluteFilePath(QFileInfo(plugin1Path).fileName())),
                 qPrintable(dir.absoluteFilePath(QFileInfo(plugin1Path).fileName())));
        QVERIFY2(QFile::copy(plugin2Path, dir.absoluteFilePath(QFileInfo(plugin2Path).fileName())),
                 qPrintable(dir.absoluteFilePath(QFileInfo(plugin2Path).fileName())));
        QVERIFY2(QFile::copy(plugin3Path, dir.absoluteFilePath(QFileInfo(plugin3Path).fileName())),
                 qPrintable(dir.absoluteFilePath(QFileInfo(plugin3Path).fileName())));
        LibraryPathRestorer restorer(QCoreApplication::libraryPaths());
        // we only want plugins from our temporary dir
        QCoreApplication::setLibraryPaths(QStringList() << temp.path());

        auto sortPlugins = [](const KPluginMetaData &a, const KPluginMetaData &b) {
            return a.pluginId() < b.pluginId();
        };
        // it should find jsonplugin and jsonplugin2 since unversionedplugin does not have any meta data
        auto plugins = KPluginLoader::findPlugins(QStringLiteral("kpluginmetadatatest"));
        std::sort(plugins.begin(), plugins.end(), sortPlugins);
        QCOMPARE(plugins.size(), 2);
        QCOMPARE(plugins[0].pluginId(), QStringLiteral("foobar")); // ID is not the filename, it is set in the JSON metadata
        QCOMPARE(plugins[0].description(), QStringLiteral("This is another plugin"));
        QCOMPARE(plugins[1].pluginId(), QStringLiteral("jsonplugin"));
        QCOMPARE(plugins[1].description(), QStringLiteral("This is a plugin"));

        // filter accepts none
        plugins = KPluginLoader::findPlugins(QStringLiteral("kpluginmetadatatest"), [](const KPluginMetaData &) { return false; });
        std::sort(plugins.begin(), plugins.end(), sortPlugins);
        QCOMPARE(plugins.size(), 0);

        // filter accepts all
        plugins = KPluginLoader::findPlugins(QStringLiteral("kpluginmetadatatest"), [](const KPluginMetaData &) { return true; });
        std::sort(plugins.begin(), plugins.end(), sortPlugins);
        QCOMPARE(plugins.size(), 2);
        QCOMPARE(plugins[0].description(), QStringLiteral("This is another plugin"));
        QCOMPARE(plugins[1].description(), QStringLiteral("This is a plugin"));

        // invalid std::function as filter
        plugins = KPluginLoader::findPlugins(QStringLiteral("kpluginmetadatatest"));
        std::sort(plugins.begin(), plugins.end(), sortPlugins);
        QCOMPARE(plugins.size(), 2);
        QCOMPARE(plugins[0].description(), QStringLiteral("This is another plugin"));
        QCOMPARE(plugins[1].description(), QStringLiteral("This is a plugin"));

        // by plugin id
        plugins = KPluginLoader::findPluginsById(dir.absolutePath(), QStringLiteral("foobar"));
        QCOMPARE(plugins.size(), 1);
        QCOMPARE(plugins[0].description(), QStringLiteral("This is another plugin"));

        // by plugin invalid id
        plugins = KPluginLoader::findPluginsById(dir.absolutePath(), QStringLiteral("invalidid"));
        QCOMPARE(plugins.size(), 0);

        // absolute path, no filter
        plugins = KPluginLoader::findPlugins(dir.absolutePath());
        std::sort(plugins.begin(), plugins.end(), sortPlugins);
        QCOMPARE(plugins.size(), 2);
        QCOMPARE(plugins[0].description(), QStringLiteral("This is another plugin"));
        QCOMPARE(plugins[1].description(), QStringLiteral("This is a plugin"));
    }

    void testForEachPlugin()
    {
        const QString jsonPluginSrc = KPluginLoader::findPlugin(QStringLiteral("jsonplugin"));
        QVERIFY2(!jsonPluginSrc.isEmpty(), qPrintable(jsonPluginSrc));
        const QString unversionedPluginSrc = KPluginLoader::findPlugin(QStringLiteral("unversionedplugin"));
        QVERIFY2(!unversionedPluginSrc.isEmpty(), qPrintable(unversionedPluginSrc));
        const QString jsonPlugin2Src = KPluginLoader::findPlugin(QStringLiteral("jsonplugin2"));
        QVERIFY2(!jsonPlugin2Src.isEmpty(), qPrintable(jsonPlugin2Src));

        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        QDir dir(temp.path());
        QVERIFY(dir.mkdir(QStringLiteral("for-each-plugin")));
        QVERIFY(dir.cd(QStringLiteral("for-each-plugin")));
        const QString jsonPluginDest = dir.absoluteFilePath(QFileInfo(jsonPluginSrc).fileName());
        QVERIFY2(QFile::copy(jsonPluginSrc, jsonPluginDest), qPrintable(jsonPluginDest));
        const QString unversionedPluginDest = dir.absoluteFilePath(QFileInfo(unversionedPluginSrc).fileName());
        QVERIFY2(QFile::copy(unversionedPluginSrc, unversionedPluginDest), qPrintable(unversionedPluginDest));
        // copy jsonplugin2 to a "for-each-plugin" subdirectory in a different directory
        QTemporaryDir temp2;
        QVERIFY(temp2.isValid());
        QDir dir2(temp2.path());
        QVERIFY(dir2.mkdir(QStringLiteral("for-each-plugin")));
        QVERIFY(dir2.cd(QStringLiteral("for-each-plugin")));
        const QString jsonPlugin2Dest = dir2.absoluteFilePath(QFileInfo(jsonPlugin2Src).fileName());
        QVERIFY2(QFile::copy(jsonPlugin2Src, jsonPlugin2Dest), qPrintable(jsonPlugin2Dest));

        QStringList foundPlugins;
        QStringList expectedPlugins;
        const auto addToFoundPlugins = [&](const QString &path) {
            QVERIFY(!path.isEmpty());
            foundPlugins.append(path);
        };

        // test finding with absolute path
        expectedPlugins = QStringList() << jsonPluginDest << unversionedPluginDest;
        expectedPlugins.sort();
        KPluginLoader::forEachPlugin(dir.path(), addToFoundPlugins);
        foundPlugins.sort();
        QCOMPARE(foundPlugins, expectedPlugins);


        expectedPlugins = QStringList() << jsonPlugin2Dest;
        expectedPlugins.sort();
        foundPlugins.clear();
        KPluginLoader::forEachPlugin(dir2.path(), addToFoundPlugins);
        foundPlugins.sort();
        QCOMPARE(foundPlugins, expectedPlugins);

        // now test relative paths

        LibraryPathRestorer restorer(QCoreApplication::libraryPaths());
        QCoreApplication::setLibraryPaths(QStringList() << temp.path());
        expectedPlugins = QStringList() << jsonPluginDest << unversionedPluginDest;
        expectedPlugins.sort();
        foundPlugins.clear();
        KPluginLoader::forEachPlugin(QStringLiteral("for-each-plugin"), addToFoundPlugins);
        foundPlugins.sort();
        QCOMPARE(foundPlugins, expectedPlugins);

        QCoreApplication::setLibraryPaths(QStringList() << temp2.path());
        expectedPlugins = QStringList() << jsonPlugin2Dest;
        expectedPlugins.sort();
        foundPlugins.clear();
        KPluginLoader::forEachPlugin(QStringLiteral("for-each-plugin"), addToFoundPlugins);
        foundPlugins.sort();
        QCOMPARE(foundPlugins, expectedPlugins);

        QCoreApplication::setLibraryPaths(QStringList() << temp.path() << temp2.path());
        expectedPlugins = QStringList() << jsonPluginDest << unversionedPluginDest << jsonPlugin2Dest;
        expectedPlugins.sort();
        foundPlugins.clear();
        KPluginLoader::forEachPlugin(QStringLiteral("for-each-plugin"), addToFoundPlugins);
        foundPlugins.sort();
        QCOMPARE(foundPlugins, expectedPlugins);
    }
};

QTEST_MAIN(KPluginLoaderTest)

#include "kpluginloadertest.moc"
