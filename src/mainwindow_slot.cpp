#include <QFileInfo>
#include <QDateTime>

#include "taskmodel.h"
#include "tableitemdata.h"

#include "consts.h"
#include "globals.h"
#include "sql.h"
#include "helper.h"

#include "ui_mainwindow.h"
#include "mainwindow.h"

void MainWindow::sayHello(int loopId, int id,
               const QString& movieFile)
{
    if(loopId != gLoopId)
        return;

    taskModel_->SetProgress(id, TaskFFmpeg::Progressing);
    insertLog(TaskKind::FFMpeg, id, QString("%1 \"%2\"").arg(tr("Started"), movieFile));
}
void MainWindow::sayNo(int loopId,
                       int id,
                       const QString& movieFile,
                       const QString& errorReason)
{
    if(loopId != gLoopId)
        return;

    insertLog(TaskKind::FFMpeg, id, QString("%1 (%2) \"%3\"").arg(tr("Not a video"), errorReason, movieFile));
}
void MainWindow::sayGoodby(int loopId,  int id,
                           const QStringList& files,
                           const QString& movieFile,
                           int thumbwidth,
                           int thumbheight,
                           const double& duration,
                           const QString& format,
                           int bitrate,
                           const QString& vcodec,
                           const QString& acodec,
                           int vWidth,int vHeight)
{
	if (loopId != gLoopId)
	{
        for(const QString& thumbfile:files)
        {
            if(thumbfile.count() == Consts::THUMB_FILENAME_LENGTH)
            {
                QFile fi(pathCombine(Consts::FILEPART_THUMBS, thumbfile));
                if(fi.exists())
                {
                    VERIFY(fi.remove());
                }
            }
        }
		return;
	}

    QFileInfo fi(movieFile);

    TableItemDataPointer pTID = TableItemData::Create(
                            files,
                            fi.canonicalPath() + '/',
                            fi.fileName(),
                            fi.size(),
                            fi.birthTime().toSecsSinceEpoch(),
                            fi.lastModified().toSecsSinceEpoch(),
                            thumbwidth,thumbheight,
                            duration,
                            format,
                            bitrate,
                            vcodec,acodec,
                            vWidth,vHeight,

                            0
                );
    if(IsDirSelected(canonicalDir(fi.canonicalPath())))
    {
        tableModel_->AppendData(pTID);
    }

    int sqlError = gpSQL->AppendData(pTID);
    if(sqlError==0)
    {
        insertLog(TaskKind::SQL, id, QString("%1 \"%2\"").arg(tr("Written in Database"), movieFile));
    }
    else
    {
        insertLog(TaskKind::SQL, id, QString("%1 \"%2\"").arg(tr("Failed to write on Database"), movieFile));
    }

    insertLog(TaskKind::FFMpeg, id, QString("%1 \"%2\"").arg(tr("Done"), movieFile));

}
void MainWindow::sayDead(int loopId, int id)
{
    if(loopId != gLoopId)
        return;

    taskModel_->RemoveTask(id);
}
void MainWindow::finished_FFMpeg(int loopId, int id)
{
    if(loopId != gLoopId)
        return;

    Q_UNUSED(id);

    idManager_->IncrementDone(IDKIND_FFmpeg);
    Q_ASSERT(idManager_->Get(IDKIND_FFmpeg) >= idManager_->GetDone(IDKIND_FFmpeg));


    if(idManager_->isAllTaskFinished())
    {
        onTaskEnded();
        insertLog(TaskKind::App, 0, tr("All Tasks finished."));
    }
}
