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

#include <QListWidgetItem>
#include <QScrollBar>

#include "consts.h"
#include "globals.h"
#include "waitcursor.h"
#include "settings.h"
#include "sql.h"
#include "extension.h"
#include "helper.h"

#include "ui_mainwindow.h"
#include "mainwindow.h"

using namespace Consts;

void MainWindow::showEvent( QShowEvent* event )
{
    QMainWindow::showEvent( event );

    if(initShown)
        return;
    initShown=true;

    ui->directoryWidget->setMaximumSize(10000,10000);
    ui->txtLog->setMaximumSize(10000,10000);
    ui->listTask->setMaximumSize(10000,10000);

    directoryChangedCommon();
    tableSortParameterChanged(sortManager_.GetCurrentSort(), sortManager_.GetCurrentRev());  // tableModel_->GetSortColumn(), tableModel_->GetSortReverse());


    on_tableView_scrollChanged(-1);

    // Alert(this, QString("ScrollIndex:%1-%2").arg(pDoc_->modeIndexRow()).arg(pDoc_->modeIndexColumn()));
    int row = 0;
    int column = 0;
    if(pDoc_ && pDoc_->getLastPos(row,column))
    {
        QModelIndex mi = proxyModel_->index(row,column);
        // Alert(this, QString("ScrollIndex:%1-%2").arg(mi.row()).arg(mi.column()));


        ui->tableView->selectionModel()->select(mi,
                                                QItemSelectionModel::ClearAndSelect);
        proxyModel_->ensureIndex(mi);
        QApplication::processEvents();
        ui->tableView->scrollTo(mi);
    }
}
void MainWindow::resizeEvent(QResizeEvent *event)
{
    resizeDock(ui->dockFolder, ui->directoryWidget->size());
    resizeDock(ui->dockOutput, ui->txtLog->size());
    resizeDock(ui->dockTask, ui->listTask->size());
    QMainWindow::resizeEvent(event);
}

void MainWindow::StoreDocument()
{
    if(!pDoc_)
        return;

    QModelIndexList sels = ui->tableView->selectionModel()->selectedIndexes();
    QModelIndex mi;
    if(!sels.isEmpty())
        mi = sels[0];
    pDoc_->Store(ui->directoryWidget,
                 mi);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    Q_UNUSED(event);

    gPaused = false;
    WaitCursor wc;

    clearAllPool();

    settings_.setValue(KEY_GEOMETRY, saveGeometry());
    settings_.setValue(KEY_WINDOWSTATE, saveState());

    if(!this->isMaximized() && !this->isMinimized())
    {
        settings_.setValue(KEY_SIZE, this->size());
        settings_.setValue(KEY_TREESIZE, ui->directoryWidget->size());
        settings_.setValue(KEY_TXTLOGSIZE, ui->txtLog->size());
        settings_.setValue(KEY_LISTTASKSIZE, ui->listTask->size());
    }
    settings_.setValue(KEY_LASTSELECTEDADDFOLDERDIRECTORY, lastSelectedAddFolderDir_);
    settings_.setValue(KEY_LASTSELECTEDSCANDIRECTORY, lastSelectedScanDir_);
    settings_.setValue(KEY_LASTSELECTEDDOCUMENT, lastSelectedDocumentDir_);

    QStringList findtexts;
    int combosavecount = qMin(cmbFind_->count(), MAX_COMBOFIND_SAVECOUNT);
    for(int i=0; i < combosavecount; ++i)
    {
        findtexts << cmbFind_->itemText(i);
    }
    settings_.setValue(KEY_COMBO_FINDTEXTS, findtexts);

    settings_.setValue(KEY_SHOWMISSING, tbShowNonExistant_->isChecked());


    StoreDocument();

    settings_.setValue(KEY_MAX_GETDIR_THREADCOUNT, optionThreadcountGetDir_);
    settings_.setValue(KEY_MAX_THUMBNAIL_THREADCOUNT, optionThreadcountThumbnail_);
    settings_.setValue(KEY_THUMBNAIL_COUNT, optionThumbCount_);

    settings_.setValue(KEY_TITLE_TEXT_TEMPLATE, tableModel_->GetTitleTextTemplate());
    settings_.setValue(KEY_INFO_TEXT_TEMPLATE, tableModel_->GetInfoTextTemplate());
    settings_.setValue(KEY_IMAGECACHETYPE, (int)tableModel_->GetImageCache());

    Extension::Save(settings_);

    for(int i=0 ; i < externalTools_.count(); ++i)
    {
        externalTools_[i].Save(i, settings_);
    }
    settings_.setValue(KEY_EXTERNALTOOLS_COUNT, externalTools_.count());

    // recents
    settings_.setValue(KEY_RECENT_OPENDOCUMENTS,recents_);

	// sort
	settings_.setValue(KEY_SORT, (int)sortManager_.GetCurrentSort());
	settings_.setValue(KEY_SORTREV, sortManager_.GetCurrentRev());

    settings_.sync();
    QMainWindow::closeEvent(event);
}
