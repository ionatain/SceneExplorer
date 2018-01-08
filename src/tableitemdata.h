#ifndef LISTITEMDATA_H
#define LISTITEMDATA_H

#include <QStringList>

class TableItemData
{
    QStringList files_;
    QString movieDirectory_;
    QString movieFilename_;

    int width_;
    int height_;
    qint64 size_=-1;
    qint64 ctime_=-1;
    qint64 wtime_=-1;
    int thumbwidth_=-1;
    int thumbheight_=-1;
    double duration_=-1;
    QString format_;
    QString vcodec_;
    QString acodec_;
    int vWidth_,vHeight_;
public:
    TableItemData(const QStringList& files,
                  const QString& movieDirectory,
                  const QString& movieFileName,

                  const qint64& size,
                  const qint64& ctime,
                  const qint64& wtime,

                  int thumbwidth,
                  int thumbheight,
                  const double& duration,
                  const QString& format,
                  const QString& vcodec,
                  const QString& acodec,
                  int vWidth,int vHeight);

    QStringList getImageFiles() const {
        return files_;
    }
    int getWidth() const {
        return width_;
    }
    int getHeight() const {
        return height_;
    }
    QString getMovieDirectory() const
    {
        return movieDirectory_;
    }
    QString getMovieFileName() const
    {
        return movieFilename_;
    }
    QString getMovieFile() const ;
    QString getFormat() const {
        return format_;
    }
    double getDuration() const {
        return duration_;
    }
    QString getVcodec() const {
        return vcodec_;
    }
    QString getAcodec() const {
        return acodec_;
    }
    int getVWidth() const {
        return vWidth_;
    }
    int getVHeight() const {
        return vHeight_;
    }
    qint64 getSize() const;
    qint64 getCtime() const;
    qint64 getWtime() const;
};

#endif // LISTITEMDATA_H
