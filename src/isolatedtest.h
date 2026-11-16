#pragma once

#ifdef NOTES_PORTABLE
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QString>
#include <QUuid>

// All persistent application data belongs to the directory containing Notes.exe.
inline QString portableRoot()
{
    return QCoreApplication::applicationDirPath();
}

inline QString portableSettings()
{
    return QDir(portableRoot()).filePath(QStringLiteral("Settings/Settings.ini"));
}

inline QString portableDatabase()
{
    return QDir(portableRoot()).filePath(QStringLiteral("Data/notes.db"));
}

inline bool validatePortablePaths()
{
    const QString root = QFileInfo(portableRoot()).canonicalFilePath();
    if (root.isEmpty()) return false;
    for (const QString &name : { QStringLiteral("Data"), QStringLiteral("Settings") }) {
        const QString path = QDir(root).filePath(name);
        if (QFileInfo(path).isSymLink() || !QDir().mkpath(path)
            || QFileInfo(path).canonicalFilePath() != QDir::cleanPath(root + "/" + name))
            return false;
    }
    for (const QString &file : { portableSettings(), portableDatabase(), portableDatabase() + "-wal", portableDatabase() + "-shm" }) {
        if (QFileInfo(file).isSymLink())
            return false;
    }
    return true;
}

// A unique backup is made before a legacy migration; failure stops the migration.
inline bool backupPortableFiles(const QStringList &files)
{
    const QString base = QDir(portableRoot()).filePath(QStringLiteral("Data/Backups"));
    if (QFileInfo(base).isSymLink() || !QDir().mkpath(base)) return false;
    const QString target = QDir(base).filePath(QUuid::createUuid().toString(QUuid::WithoutBraces));
    if (!QDir().mkpath(target)) return false;
    for (const QString &file : files) {
        if (QFileInfo(file).isSymLink()) return false;
        if (QFile::exists(file) && !QFile::copy(file, QDir(target).filePath(QFileInfo(file).fileName()))) return false;
    }
    return true;
}
#endif
