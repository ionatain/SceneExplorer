//SceneExplorer
//Exploring video files by viewer thumbnails
//
//Copyright (C) 2018  Ambiesoft
//
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program.  If not, see <https://www.gnu.org/licenses/>.

// #define _WIN32_WINNT 0x0501 // XP and above
#include <windows.h>
#include <Shlobj.h>

#include <QApplication>
#include <QDir>
//#include <QMessageBox>
//#include <QProcessEnvironment>
//#include <QFileInfo>
//#include <QDesktopServices>
#include <QWidget>
#include <QThread>
#include <QStandardPaths>
#include <QProcessEnvironment>
#include <QProcess>
#include <QDebug>

#include "../../lsMisc/stdQt/stdQt.h"

#include "../../lsMisc/GetLastErrorString.h"
#include "../../profile/cpp/Profile/include/ambiesoft.profile.h"
#include "../../lsMisc/stdosd/SetPrority.h"
#include "../../lsMisc/OpenCommon.h"

#include "errorinfoexception.h"
#include "helper.h"
#include "osd.h"

using namespace Ambiesoft::stdosd::Process;
using namespace Ambiesoft;
using namespace AmbiesoftQt;


// https://stackoverflow.com/a/3546503
bool showInGraphicalShell(QWidget *parent, const QString &pathIn)
{
    Q_UNUSED(parent);

    QString explorer;
    QString path = QProcessEnvironment::systemEnvironment().value("PATH");
    QStringList paths = path.split(';');
    for (int i = 0; i < paths.length(); ++i)
    {
        QString s = pathCombine(paths[i], "explorer.exe");

        if (QFile(s).exists())
        {
            explorer = s;
            break;
        }
    }
    if (explorer.isEmpty()) {
        return false;
    }
    QString param;
    if (!QFileInfo(pathIn).isDir())
    {
        QStringList args;
        args << "/select,";
        args << QDir::toNativeSeparators(pathIn);
        args << ",/n";
        QProcess::startDetached(explorer, args);
    }
    else
    {
        QStringList args;
        args << QDir::toNativeSeparators(pathIn);
        QProcess::startDetached(explorer, args);
    }

    return true;
}

// https://stackoverflow.com/a/17974223
void MoveToTrashImpl( QString file ){
    QFileInfo fileinfo( file );
    if( !fileinfo.exists() )
        throw ErrorInfoException( QObject::tr("File doesnt exists, cant move to trash" ));
    WCHAR from[ MAX_PATH ];
    memset( from, 0, sizeof( from ));
    int l = fileinfo.absoluteFilePath().toWCharArray( from );
    Q_ASSERT( 0 <= l && l < MAX_PATH );
    from[ l ] = '\0';
    SHFILEOPSTRUCT fileop;
    memset( &fileop, 0, sizeof( fileop ) );
    fileop.wFunc = FO_DELETE;
    fileop.pFrom = from;
    fileop.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
    int rv = SHFileOperation( &fileop );
    if( 0 != rv ){
        qDebug() << rv << QString::number( rv ).toInt( nullptr, 8 );
        throw ErrorInfoException( QObject::tr("move to trash failed" ));
    }
}


QString GetDefaultFFprobe()
{
    return pathCombine(GetAppDir(), "ffmpeg/bin/ffprobe.exe");
}
QString GetDefaultFFmpeg()
{
    return pathCombine(GetAppDir(), "ffmpeg/bin/ffmpeg.exe");
}

static QString getSpecialFolder(int id)
{
    wchar_t path[MAX_PATH] = {0};
    if(S_OK != SHGetFolderPath(
                nullptr,
                id,
                nullptr,
                SHGFP_TYPE_CURRENT,
                path))

    {
        Alert(nullptr, QObject::tr("Failed to get folder location."));
        return QString();
    }


    //    PWSTR pOut=NULL;
    //    if(S_OK != SHGetKnownFolderPath(id,0,NULL,&pOut))
    //    {
    //        Alert(nullptr, QObject::tr("Failed to get Roaming folder. Default folder will be used."));
    //        return QString();
    //    }

    QString dir = QString::fromUtf16((const ushort*)path);
    //    CoTaskMemFree(pOut);

    dir = QDir(dir).absolutePath();

    QDir(dir).mkdir("Ambiesoft");
    dir = pathCombine(dir, "Ambiesoft");

    QDir(dir).mkdir("SceneExplorer");
    dir = pathCombine(dir, "SceneExplorer");

    return dir;
}
QString getInifile(bool& bExit)
{
    QFileInfo trfi(QCoreApplication::applicationFilePath());
    std::string folini = pathCombine(trfi.absolutePath(), "folder.ini").toStdString();
    int intval = -1;
    Profile::GetInt("Main", "PathType", -1, intval, folini);

    QString dir;
    switch (intval)
    {
    case 0:
        dir = trfi.absolutePath();
        break;

    case 1:
        dir = getSpecialFolder(CSIDL_LOCAL_APPDATA);
        break;

    default:
    case 2:
        dir = getSpecialFolder(CSIDL_APPDATA);
        break;

    case 3:
    {
        std::string t;
        Profile::GetString("Main", "folder", "", t, folini);
        dir = t.c_str();
        if(dir.isEmpty())
        {
            Alert(nullptr, QObject::tr("Folder settings does not have sufficient data. Lanuch FolderConfig.exe and reconfigure."));
            bExit=true;
            return QString();
        }
        else
        {
            if(!QDir(dir).exists())
            {
                if(!YesNo(nullptr, QObject::tr("\"%1\" does not exist. Do you want to create it?").arg(dir)))
                {
                    bExit=true;
                    return QString();
                }
                QDir(dir).mkpath(".");
                if(!QDir(dir).exists())
                {
                    Alert(nullptr, QObject::tr("Failed to create directory."));
                    bExit=true;
                    return QString();
                }
            }
        }
    }
        break;
    } // switch

    if (dir.isEmpty())
        return QString();

    return pathCombine(dir, "SceneExplorer.ini");
}

// https://stackoverflow.com/a/45282192
bool isLegalFilePath(QString filename, QString* pError)
{
    // Windows filenames are not case sensitive.
    filename = filename.toUpper();

    QString illegal=GetIllegalFilenameCharacters();

    foreach (const QChar& c, filename)
    {
        // Check for control characters
        if (c.toLatin1() > 0 && c.toLatin1() < 32)
        {
            if(pError)
            {
                *pError = QObject::tr("Filename could not have control characters.");
            }
            return false;
        }
        // Check for illegal characters
        if (illegal.contains(c))
        {
            if(pError)
            {
                *pError = QObject::tr("Filename could not have these characters.");
                *pError += "\n\n";
                *pError += GetIllegalFilenameCharacters();
            }
            return false;
        }
    }

    // Check for device names in filenames
    static QStringList devices;

    if (!devices.count())
        devices << "CON" << "PRN" << "AUX" << "NUL" << "COM0" << "COM1" << "COM2"
                << "COM3" << "COM4" << "COM5" << "COM6" << "COM7" << "COM8" << "COM9" << "LPT0"
                << "LPT1" << "LPT2" << "LPT3" << "LPT4" << "LPT5" << "LPT6" << "LPT7" << "LPT8"
                << "LPT9";

    const QFileInfo fi(filename);
    const QString basename = fi.baseName();

    foreach (const QString& s, devices)
    {
        if (basename == s)
        {
            if(pError)
            {
                *pError = QObject::tr("Filename must not be device names.");
            }
            return false;
        }
    }
    // Check for trailing periods or spaces
    if (filename.right(1)==".")
    {
        if(pError)
        {
            *pError = QObject::tr("Filename should not end with period.");
        }
        return false;
    }
    if( filename.right(1)==" ")
    {
        if(pError)
        {
            *pError = QObject::tr("Filename must not end with space character.");
        }
        return false;
    }

    if (filename.right(1)=="\\")
    {
        if(pError)
        {
            *pError = QObject::tr("Filename must not end with backslash character.");
        }
        return false;
    }
    if (filename.right(1)=="/")
    {
        if(pError)
        {
            *pError = QObject::tr("Filename must not end with slash character.");
        }
        return false;
    }
    // Check for pathnames that are too long (disregarding raw pathnames)
    if (filename.length()>260)
    {
        if(pError)
        {
            *pError = QObject::tr("Filename is too long.");
        }
        return false;
    }

    //    // Exclude raw device names
    //    if (filename.left(4)=="\\\\.\\")
    //        return false;


    return true;
}
QString GetIllegalFilenameCharacters()
{
    return "<>:\"|?*";
}





bool setProcessPriority(const qint64& pid, QThread::Priority priority, QStringList& errors)
{
    CPUPRIORITY cpuPriority = CPU_NONE;
    IOPRIORITY ioPriority = IO_NONE;
    MEMORYPRIORITY memPriority = MEMORY_NONE;

    switch (priority)
    {
    case QThread::HighestPriority:
        cpuPriority = CPU_HIGH;
        ioPriority = IO_HIGH;
        memPriority = MEMORY_HIGH;
        break;
    case QThread::HighPriority:
        cpuPriority = CPU_ABOVENORMAL;
        ioPriority = IO_ABOVENORMAL;
        memPriority = MEMORY_ABOVENORMAL;
        break;
    case QThread::NormalPriority:
        cpuPriority = CPU_NORMAL;
        ioPriority = IO_NORMAL;
        memPriority = MEMORY_NORMAL;
        break;
    case QThread::LowPriority:
        cpuPriority = CPU_BELOWNORMAL;
        ioPriority = IO_BELOWNORMAL;
        memPriority = MEMORY_BELOWNORMAL;
        break;
    case QThread::LowestPriority:
    case QThread::IdlePriority:
        cpuPriority = CPU_IDLE;
        ioPriority = IO_IDLE;
        memPriority = MEMORY_IDLE;
        break;
    default:
        Q_ASSERT(false);
        return false;
    }

    int ret = SetProirity(pid,
                          cpuPriority,
                          ioPriority,
                          memPriority);

    if (ret != 0)
    {
        std::wstring lastError = GetLastErrorString(static_cast<DWORD>(ret));
        errors << QString::fromStdWString(lastError);
    }
    return ret==0;
}


bool myRename(const QString& oldfull, const QString& newfull, QString& error)
{
    if(!MoveFileW(oldfull.toStdWString().c_str(), newfull.toStdWString().c_str()))
    {
        error = QString::fromStdWString(GetLastErrorStringW(GetLastError()));
        return false;
    }
    return true;
}

QString GetUserDocumentDirectory()
{
    QString result = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if(!result.isEmpty())
        return result;

    result = getSpecialFolder(CSIDL_MYDOCUMENTS);
    if(!result.isEmpty())
        return result;

    return QDir::homePath();
}

bool StartProcessDetached(const QString& exe, const QString& arg)
{
    return !!OpenCommon(nullptr,
               exe.toStdWString().c_str(),
               arg.toStdWString().c_str());
}
