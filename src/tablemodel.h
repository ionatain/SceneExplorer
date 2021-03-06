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

#ifndef IMAGEMODEL_H
#define IMAGEMODEL_H

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <QWidget>
#include <QFile>
#include <QStack>
#include <QElapsedTimer>

#include "globals.h"
#include "tableitemdata.h"
#include "imainwindow.h"

class QTableView;

class TableModel : public QAbstractTableModel
{
    Q_OBJECT

    static const int RowCountPerEntry = 3;
    // static const int ColumnCountImage = 5;
    int columnCountImage_ = 3;
    bool initColumnWidth_ = false;

    QFont fontInfo_;
    QFont fontDetail_;

    mutable QStack<QModelIndex> suspendImageIndexes_;
    mutable int timerID_=0;
    mutable QElapsedTimer imageElapsedTimer_;
    mutable int imageInterval_ = 100;

    void StartImageTimer() ;
    void KillImageTimer();
protected:
    void timerEvent(QTimerEvent *event) override;

public:

    // static QString GetSortColumnName(SORTCOLUMN sc);
    QString GetSortColumnValue(SORTCOLUMNMY sc, TableItemDataPointer item) const;
private:
    QList<TableItemDataPointer> itemDatas_;
    QMap<QString, TableItemDataPointer> mapsFullpathToItem_;
    mutable QMap<QString, QVariant> mapPixmaps_;
    ImageCacheType imagecache_;

    bool bShowMissing_ = false;

    QTableView* parent_;
    IMainWindow* mainWindow_;

    QString ExtractInfoText(TableItemDataPointer item, const QString& str) const;
    QString GetTitleText(TableItemDataPointer item) const;
    QString GetInfoText(TableItemDataPointer item) const;

    // static bool itemDataLessThan(const TableItemDataPointer v1, const TableItemDataPointer v2);

    void ClearData();
    // SORTCOLUMN sortColumn_ = SORT_FILENAME;
    // bool sortReverse_ = false;

    //void SetSortColumn(SORTCOLUMN sc);
    //void SetSortReverse(bool rev);
public:
    //SORTCOLUMN GetSortColumn() const;
    //bool GetSortReverse() const;

    enum TableRole {
        MovieFileFull = Qt::UserRole + 1,
        ID,
    };
    TableModel(QTableView *parent, IMainWindow* mainWindow);
    void AppendData(TableItemDataPointer pItemData, const bool enableUpdate = true);
    // void AppendDatas(const QList<TableItemData*>&v);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QSize calculateSize(const QModelIndex& index, const QString& str) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex & index) const override ;

    //    void Sort_obsolete(SORTCOLUMN column);
    //    void Sort_obsolete(SORTCOLUMN column, bool rev);
    //    bool RenameEntries(const QString& dir,
    //                       const QStringList& renameOlds,
    //                       const QStringList& renameNews);
    bool RenameEntry(const QString& dbDir,
                     const QString& dbFile,
                     const QString& newdir,
                     const QString& newfile);
    void ResetData(const QList<TableItemDataPointer>& all);
    int GetItemCount() const {
        return itemDatas_.count();
    }

    void SetShowMissing(bool bToggle)
    {
        bShowMissing_ = bToggle;
    }
    bool IsShowMissing() const {
        return bShowMissing_;
    }
    void UpdateOpenCountAndLastAccess(const QString& movieFile,
                                      const int* opencount,
                                      const qint64* lastaccess);
    void RemoveItem(const QString& movieFile);
    QModelIndex GetIndex(const QString& movieFile) const;
    QFont GetInfoFont() {
        return fontInfo_;
    }
    void SetInfoFont(const QFont& font) {
        fontInfo_=font;
    }

    QFont GetDetailFont() {
        return fontDetail_;
    }
    void SetDetailFont(const QFont& font) {
        fontDetail_=font;
    }

    ImageCacheType GetImageCache() const {
        return imagecache_;
    }
    QString titleTemplate_, infoTemplate_;
    QString GetTitleTextTemplate() const {
        return titleTemplate_;
    }
    QString GetInfoTextTemplate() {
        return infoTemplate_;
    }
    void SetTitleTextTemplate(const QString& s) {
        titleTemplate_=s;
    }
    void SetInfoTextTemplate(const QString& s) {
        infoTemplate_=s;
    }

    void SetImageCache(ImageCacheType ic) {
        imagecache_=ic;
    }
    void SetColumnCountImage(int i) {
        beginResetModel();
        columnCountImage_=i;
        initColumnWidth_=false;
        endResetModel();
    }
    int getActualColumnIndex(int ci) const {
        if(columnCountImage_==5)
            return ci;
        switch(ci)
        {
        case 0:return 0;
        case 1:return 2;
        case 2:return 4;
        }
        Q_ASSERT(false);
        return 0;
    }
signals:
    void itemCountChanged();
    //void sortParameterChanged(SORTCOLUMN sc, bool rev);
};

class FileMissingFilterProxyModel : public QSortFilterProxyModel
{

public:
    FileMissingFilterProxyModel(QWidget*parent) : QSortFilterProxyModel(parent){}

    bool filterAcceptsRow(
            int sourceRow,
            const QModelIndex &sourceParent) const
    {
        if (((TableModel*) sourceModel())->IsShowMissing())
            return true;

        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        QVariant v = sourceModel()->data(index, TableModel::MovieFileFull);
        QString s = v.toString();
        return QFile(s).exists();
    }
    void ensureBottom()
    {
        int allcount = rowCount();
        int startI = allcount-(3*10);
        if (startI < 0)
            return;
        for(int i=startI; i < allcount; ++i)
        {
            sourceModel()->data(sourceModel()->index(i,0),Qt::DecorationRole);
            sourceModel()->data(sourceModel()->index(i,1),Qt::DecorationRole);
            sourceModel()->data(sourceModel()->index(i,2),Qt::DecorationRole);
            sourceModel()->data(sourceModel()->index(i,3),Qt::DecorationRole);
            sourceModel()->data(sourceModel()->index(i,4),Qt::DecorationRole);
        }
    }
    void ensureIndex(const QModelIndex& mi)
    {
        int startI = mi.row();
        startI = startI -(3*10);
        if(startI < 0)
            startI=0;

        int endI = mi.row() + 10;
        if(endI >= rowCount())
            endI = rowCount();
        for(int i=startI; i < endI; ++i)
        {
            sourceModel()->data(sourceModel()->index(i,0),Qt::DecorationRole);
            sourceModel()->data(sourceModel()->index(i,1),Qt::DecorationRole);
            sourceModel()->data(sourceModel()->index(i,2),Qt::DecorationRole);
            sourceModel()->data(sourceModel()->index(i,3),Qt::DecorationRole);
            sourceModel()->data(sourceModel()->index(i,4),Qt::DecorationRole);
        }
    }
    int GetItemCount() const {
        return rowCount()/3;
    }
    //    QString pathInfo(const QModelIndex& mi) const {
    //        int amari = mi.row() % 3;
    //        QModelIndex newI = createIndex(mi.row()-amari, mi.column());
    //        QVariant v=data(newI);
    //        if(!v.isValid())
    //            return QString;
    //        return v.toString();
    //    }
    enum class FIND_INDEX {
        FIND_INDEX_TITLE,
        FIND_INDEX_MOVIE,
        FIND_INDEX_DESCRIPTION,
    };

    QModelIndex findIndex(const QString& selPath, const FIND_INDEX& fi) const {
        QModelIndex sourceIndex = ((TableModel*) sourceModel())->GetIndex(selPath);

        int toAdd=0;
        if(fi==FIND_INDEX::FIND_INDEX_TITLE)
            toAdd=1;
        else if(fi==FIND_INDEX::FIND_INDEX_DESCRIPTION)
            toAdd=2;

        return index(sourceIndex.row() + toAdd, sourceIndex.column());
    }

};
//#include <QItemDelegate>
//class ImageSizeDelegate : public QItemDelegate 
//{
//    Q_OBJECT
//public:
//	ImageSizeDelegate(QWidget * parent) : QItemDelegate(parent){}
////    void paint(QPainter *painter, const QStyleOptionViewItem &option,
////               const QModelIndex &index) const;
//	QSize sizeHint(
//		const QStyleOptionViewItem &option,
//		const QModelIndex &index) const Q_DECL_OVERRIDE
//    {
//        return QSize(100,100);
//    }
//};



#endif // IMAGEMODEL_H
