#include <QObject>
#include <QDebug>

#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>

#include <QFileInfo>
#include <QFileDevice>
#include <QDateTime>
#include <QSqlError>
#include <QSet>

#include "tableitemdata.h"
#include "helper.h"
#include "sql.h"



Sql::Sql() : db_(QSqlDatabase::addDatabase("QSQLITE"))
{
     db_.setDatabaseName("./db.sqlite3");
     db_.open();

     QSqlQuery query;
     query.exec("create table FileInfo(size, ctime, wtime, directory, name, salient, thumbid)");
     query.exec("alter table FileInfo add duration");
     query.exec("alter table FileInfo add format");
     query.exec("alter table FileInfo add vcodec");
     query.exec("alter table FileInfo add acodec");
     query.exec("alter table FileInfo add vwidth");
     query.exec("alter table FileInfo add vheight");

     for (int i = 0; i < db_.tables().count(); i ++) {
         qDebug() << db_.tables().at(i);
     }
     query.exec("PRAGMA table_info('FileInfo')");
     while(query.next())
     {
         QString col=query.value("name").toString();
         qDebug() << col;
         allColumns_.append(col);
     }
     ok_ = true;
}
Sql::~Sql()
{
    delete pQDeleteFromDirectoryName_;
    delete pQInsert_;
    delete pQGetInfo_;

    db_.close();
}


static QString getUUID(const QString& file)
{
    int i = file.lastIndexOf('.');
    if(i < 0)
        return QString();
    QString ret = file.left(i);
    if(ret.length() < 2)
        return QString();
    ret = ret.left(ret.length()-2);
    return ret;
}
static bool isUUID(const QString& s)
{
    if(s.isEmpty())
        return false;
    if(s.length()!=36)
        return false;
    return true;
}

int Sql::GetMovieFileInfo(const QString& movieFile,
                     bool& exist,
                     qint64& size,
                     QString& directory,
                     QString& name,
                     QString* salient,
                     qint64& ctime,
                     qint64& wtime) const
{
    QFileInfo fi(movieFile);
    exist = fi.exists();
    if(!exist)
        return MOVIEFILE_NOT_FOUND;

    size = fi.size();
    if(size <= 0)
        return FILESIZE_UNDERZERO;

    directory = fi.absolutePath();
    name = fi.fileName();

    ctime = fi.birthTime().toSecsSinceEpoch();
    wtime = fi.lastModified().toSecsSinceEpoch();

    if(salient)
    {
        *salient = createSalient(movieFile, size);
        if(salient->isEmpty())
            return ERROR_CREATE_SALIENT;
    }
    return 0;
}

QSqlQuery* Sql::getDeleteFromDirectoryName()
{
    if(pQDeleteFromDirectoryName_)
        return pQDeleteFromDirectoryName_;

    pQDeleteFromDirectoryName_ = new QSqlQuery(db_);
    if(!pQDeleteFromDirectoryName_->prepare("delete from FileInfo where "
                  "directory=? and name=?"))
    {
        qDebug() << pQDeleteFromDirectoryName_->lastError();
        Q_ASSERT(false);
        return nullptr;
    }
    return pQDeleteFromDirectoryName_;
}

QString Sql::getAllColumns(bool bBrace, bool bQ)
{
    QString ret;
    ret += " "; //safe space
    if(bBrace)
        ret.append("(");

    for(int i=0 ; i < allColumns_.count(); ++i)
    {
        if(bQ)
        {
            ret.append("?");
        }
        else
        {
            ret.append(allColumns_[i]);
        }
        if( (i+1) != allColumns_.count())
            ret.append(",");
    }

    if(bBrace)
        ret.append(")");

    ret += " "; //safe space
    return ret;
}
QSqlQuery* Sql::getInsertQuery()
{
    if(pQInsert_)
        return pQInsert_;
    pQInsert_=new QSqlQuery(db_);

    QString preparing =
        QString("insert into FileInfo ") +
        // "(size, ctime, wtime, directory, name, salient, thumbid, format, duration) "
        getAllColumns(true,false) +
        "values "+
        getAllColumns(true,true);
        //"values (?,?,?,?,?,?,?,?,?)"))
    if(!pQInsert_->prepare(preparing))
    {
        qDebug() << pQInsert_->lastError();
        Q_ASSERT(false);
        return nullptr;
    }
    return pQInsert_;
}
int Sql::AppendData(const TableItemData& tid)
{
    QString salient = createSalient(tid.getMovieFile(), tid.getSize());

    Q_ASSERT(tid.getImageFiles().count()==5);
    if(tid.getImageFiles().isEmpty())
        return THUMBFILE_NOT_FOUND;

    QString uuid = getUUID(tid.getImageFiles()[0]);
    if(!isUUID(uuid))
        return UUID_FORMAT_ERROR;

    {
        QSqlQuery* pQuery = getDeleteFromDirectoryName();

        int i = 0;

        pQuery->bindValue(i++, tid.getMovieDirectory());
        pQuery->bindValue(i++, tid.getMovieFileName());
        if(!pQuery->exec())
        {
            qDebug() << pQuery->lastError();
            return SQL_EXEC_FAILED;
        }
    }

    QSqlQuery* pQInsert = getInsertQuery();
    int i = 0;
    pQInsert->bindValue(i++, tid.getSize());
    pQInsert->bindValue(i++, tid.getCtime());
    pQInsert->bindValue(i++, tid.getWtime());
    pQInsert->bindValue(i++, tid.getMovieDirectory());
    pQInsert->bindValue(i++, tid.getMovieFileName());
    pQInsert->bindValue(i++, salient);
    pQInsert->bindValue(i++, uuid);
    pQInsert->bindValue(i++, tid.getDuration());
    pQInsert->bindValue(i++, tid.getFormat());
    pQInsert->bindValue(i++, tid.getVcodec());
    pQInsert->bindValue(i++, tid.getAcodec());
    pQInsert->bindValue(i++, tid.getVWidth());
    pQInsert->bindValue(i++, tid.getVHeight());

    if(!pQInsert->exec())
    {
        qDebug() << pQInsert->lastError();
        return SQL_EXEC_FAILED;
    }
    return 0;
}
QSqlQuery* Sql::getGetInfoQuery()
{
    if(pQGetInfo_)
        return pQGetInfo_;
    pQGetInfo_=new QSqlQuery(db_);

    if(!pQGetInfo_->prepare("select * from FileInfo where "
                   "size=? and directory=? and name=? and salient=? and ctime=? and wtime=?"))
    {
        qDebug() << pQGetInfo_->lastError();
        Q_ASSERT(false);
        return nullptr;
    }
    return pQGetInfo_;
}

bool Sql::IsSameFile(const QString& dir,
                const QString& name,
                const qint64& size,
                const QString& salient)
{
    bool exist2;
    qint64 size2;
    QString directory2;
    QString name2;
    QString salient2;
    qint64 ctime2;
    qint64 wtime2;
    int ret = GetMovieFileInfo(
                     pathCombine(dir,name),
                     exist2,
                     size2,
                     directory2,
                     name2,
                     nullptr,
                     ctime2,
                     wtime2);
    if(ret != 0)
        return false;

    if(size != size2)
        return false;

    // need to check salient
    GetMovieFileInfo(
                     pathCombine(dir,name),
                     exist2,
                     size2,
                     directory2,
                     name2,
                     &salient2,
                     ctime2,
                     wtime2);
    if(salient != salient2)
        return false;

    return true;
}
int Sql::filterWithEntry(const QString& movieDir,
                         const QStringList& movieFiles,
                         QStringList& results)
{
    if(movieFiles.isEmpty())
        return 0;

    QSet<QString> sets;
    for(int i=0 ; i < movieFiles.count(); ++i)
        sets.insert(movieFiles[i]);

    QSqlQuery query(db_);
    if(!query.prepare("select size,name,salient from FileInfo where "
                  "directory=?"))
    {
        qDebug() << pQGetInfo_->lastError();
        Q_ASSERT(false);
        return SQL_PREPARE_FAILED;
    }
    query.bindValue(0, movieDir);

    if(!query.exec())
    {
        return SQL_EXEC_FAILED;
    }

    while(query.next())
    {
        qint64 size = query.value("size").toLongLong();
        QString name = query.value("name").toString();
        QString salient = query.value("salient").toString();

        if(sets.contains(name) && IsSameFile(movieDir,name,size,salient))
        {
            sets.remove(name);
        }
    }

    QSet<QString>::iterator it;
    for (it = sets.begin(); it != sets.end(); ++it)
        results.append(*it);
    return 0;
}
int Sql::GetAllEntry(const QString& dir,
                     QStringList& entries,
                     QVector<qint64>& sizes,
                     QVector<qint64>& ctimes,
                     QVector<qint64>& wtimes,
                     QStringList& salients)
{
    QSqlQuery query(db_);
    if(!query.prepare("select name, size, ctime, wtime, salient from FileInfo where "
                  "directory=?"))
    {
        qDebug() << query.lastError();
        Q_ASSERT(false);
        return SQL_PREPARE_FAILED;
    }
    query.bindValue(0, dir);

    if(!query.exec())
    {
        return SQL_EXEC_FAILED;
    }

    while(query.next())
    {
        QString name = query.value("name").toString();
        entries.append(name);
        sizes.append(query.value("size").toLongLong());
        ctimes.append(query.value("ctime").toLongLong());
        wtimes.append(query.value("wtime").toLongLong());
        salients.append(query.value("salient").toString());
    }

    return 0;
}
int Sql::hasThumb(const QString& movieFile)
{
    bool exist;
    qint64 size;
    QString directory;
    QString name;
    QString salient;
    qint64 ctime;
    qint64 wtime;
    int ret = GetMovieFileInfo(
                     movieFile,
                     exist,
                     size,
                     directory,
                     name,
                     &salient,
                     ctime,
                     wtime);
    if(ret != 0)
        return ret;

    QSqlQuery* pGetInfo = getGetInfoQuery();
    int i = 0;
    pGetInfo->bindValue(i++, size);
    pGetInfo->bindValue(i++, directory);
    pGetInfo->bindValue(i++, name);
    pGetInfo->bindValue(i++, salient);
    pGetInfo->bindValue(i++, ctime);
    pGetInfo->bindValue(i++, wtime);
    if(!pGetInfo->exec())
    {
        qDebug() << pGetInfo->lastError();
        return SQL_EXEC_FAILED;
    }
    while (pGetInfo->next())
    {
        QString thumbid = pGetInfo->value("thumbid").toString();
        QStringList thumbs;
        for(int i=1 ; i <= 5 ; ++i)
        {
            QString t=thumbid;
            t+="-";
            t+=QString::number(i);
            t+=".png";

            t = pathCombine("thumbs", t);
            if(!QFile(t).exists())
            {
                removeEntry(thumbid);
                return THUMBFILE_NOT_FOUND;
            }
        }

        Q_ASSERT(!pGetInfo->next());
        return THUMB_EXIST;
    }
    return THUMB_NOT_EXIST;
}
int Sql::removeEntry(const QString& thumbid)
{
    if(!isUUID(thumbid))
        return THUMBID_IS_NOT_UUID;

    QSqlQuery query(db_);

    if(!query.prepare("delete from FileInfo where "
                   "thumbid=?"))
    {
        qDebug() << query.lastError();
        return SQL_EXEC_FAILED;
    }
    int i = 0;
    query.bindValue(i++, thumbid);
    if(!query.exec())
    {
        qDebug() << query.lastError();
        return SQL_EXEC_FAILED;
    }
    return 0;
}
QString Sql::getErrorStrig(int thumbRet)
{
    switch(thumbRet)
    {
    case NO_ERROR: return tr("No Error");
    case MOVIEFILE_NOT_FOUND: return tr("Video file not found.");
    case FILESIZE_UNDERZERO: return tr("File size is under 0.");
    case ERROR_CREATE_SALIENT: return tr("Failed to create Salient.");
    case THUMBFILE_NOT_FOUND: return tr("Thumbnail file(s) not found.");
    case UUID_FORMAT_ERROR: return tr("UUID format error.");
    case SQL_EXEC_FAILED: return tr("Sql failed.");
    case THUMB_EXIST: return tr("Thumb exists.");
    case THUMB_NOT_EXIST: return tr("Thumb not exists.");
    }
    Q_ASSERT(false);
    return QString();
}
bool Sql::GetAll(QList<TableItemData*>& v)
{
    QSqlQuery query(db_);
    if(!query.exec("select * from FileInfo"))
        return false;
    while (query.next()) {
        QString directory = query.value("directory").toString();
        QString name = query.value("name").toString();
        QString movieFileFull = pathCombine(directory,name);
         if(!QFile(movieFileFull).exists())
             continue;
        QString thumbid = query.value("thumbid").toString();
        QStringList thumbs;
        for(int i=1 ; i <= 5 ; ++i)
        {
            QString t=thumbid;
            t+="-";
            t+=QString::number(i);
            t+=".png";
            thumbs.append(t);
        }

        qint64 size = query.value("size").toLongLong();
        qint64 ctime = query.value("ctime").toLongLong();
        qint64 wtime = query.value("wtime").toLongLong();
        QString salitnet = query.value("salient").toString();
        double duration  = query.value("duration").toDouble();
        QString format = query.value("format").toString();

        QString vcodec = query.value("vcodec").toString();
        QString acodec = query.value("acodec").toString();

        int vwidth = query.value("vwidth").toInt();
        int vheight = query.value("vheight").toInt();
        TableItemData* pID = new TableItemData(thumbs,
                                               directory,
                                               name,

                                               size,
                                               ctime,
                                               wtime,

                                               0,0,
                                               duration,
                                               format,
                                               vcodec,acodec,
                                               vwidth,vheight);
        v.append(pID);
    }
    return true;
}
