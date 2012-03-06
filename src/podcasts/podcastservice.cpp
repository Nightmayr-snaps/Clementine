/* This file is part of Clementine.
   Copyright 2012, David Sansome <me@davidsansome.com>
   
   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "addpodcastdialog.h"
#include "podcastbackend.h"
#include "podcastservice.h"
#include "core/application.h"
#include "core/logging.h"
#include "internet/internetmodel.h"
#include "library/libraryview.h"
#include "ui/iconloader.h"
#include "ui/standarditemiconloader.h"

#include <QMenu>

const char* PodcastService::kServiceName = "Podcasts";
const char* PodcastService::kSettingsGroup = "Podcasts";

PodcastService::PodcastService(Application* app, InternetModel* parent)
  : InternetService(kServiceName, app, parent, parent),
    use_pretty_covers_(true),
    icon_loader_(new StandardItemIconLoader(app->album_cover_loader(), this)),
    context_menu_(NULL),
    root_(NULL),
    backend_(app->podcast_backend())
{
  icon_loader_->SetModel(model());
}

PodcastService::~PodcastService() {
}

QStandardItem* PodcastService::CreateRootItem() {
  root_ = new QStandardItem(QIcon(":providers/podcast16.png"), tr("Podcasts"));
  root_->setData(true, InternetModel::Role_CanLazyLoad);
  return root_;
}

void PodcastService::LazyPopulate(QStandardItem* parent) {
  switch (parent->data(InternetModel::Role_Type).toInt()) {
  case InternetModel::Type_Service:
    PopulatePodcastList(parent);
    break;
  }
}

void PodcastService::PopulatePodcastList(QStandardItem* parent) {
  if (default_icon_.isNull()) {
    default_icon_ = QIcon(":providers/podcast16.png");
  }

  foreach (const Podcast& podcast, backend_->GetAllSubscriptions()) {
    const int unlistened_count = podcast.extra("db:unlistened_count").toInt();
    QString title = podcast.title();

    QStandardItem* item = new QStandardItem;

    if (unlistened_count > 0) {
      // Add the number of new episodes after the title.
      title.append(QString(" (%1)").arg(unlistened_count));

      // Set a bold font
      QFont font(item->font());
      font.setBold(true);
      item->setFont(font);
    }

    item->setText(podcast.title());
    item->setIcon(default_icon_);

    // Load the podcast's image if it has one
    if (podcast.image_url().isValid()) {
      icon_loader_->LoadIcon(podcast.image_url().toString(), QString(), item);
    }

    parent->appendRow(item);
  }
}

void PodcastService::ShowContextMenu(const QModelIndex& index,
                                     const QPoint& global_pos) {
  if (!context_menu_) {
    context_menu_ = new QMenu;
    context_menu_->addAction(IconLoader::Load("list-add"), tr("Add podcast..."),
                             this, SLOT(AddPodcast()));
  }

  context_menu_->popup(global_pos);
}

void PodcastService::ReloadSettings() {
  QSettings s;
  s.beginGroup(LibraryView::kSettingsGroup);

  use_pretty_covers_ = s.value("pretty_covers", true).toBool();
  // TODO: reload the podcast icons that are already loaded?
}

QModelIndex PodcastService::GetCurrentIndex() {
  return QModelIndex();
}

void PodcastService::AddPodcast() {
  if (!add_podcast_dialog_) {
    add_podcast_dialog_.reset(new AddPodcastDialog(app_));
  }

  add_podcast_dialog_->show();
}
