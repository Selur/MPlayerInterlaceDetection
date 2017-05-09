/*
 * MyListWidget.cpp
 *
 *  Created on: Dec 25, 2011
 *      Author: Selur
 */

#include "MyListWidget.h"
#include <QUrl>
#include <QFileInfo>
#include <QMessageBox>
#include <QDirIterator>
#include <QMimeData>
#include <QDropEvent>
#include <QStringList>
#include <QDragEnterEvent>

MyListWidget::MyListWidget(QWidget *parent) :
    QListWidget(parent), m_currentItems()
{
  this->setObjectName("MyListWidget");
  this->setDragEnabled(true);
  this->setAcceptDrops(true);
  this->setDropIndicatorShown(true);
}

MyListWidget::~MyListWidget()
{
}

void MyListWidget::remove(QListWidgetItem *item)
{
  item = this->takeItem(this->row(item));
  m_currentItems.removeAll(item->text());
}

void MyListWidget::addLocation(QString location)
{
  QStringList unwantedExtensions;
  foreach (QString ext, unwantedExtensions) {
    if (!location.endsWith(ext, Qt::CaseInsensitive))  {
      return;
    }
  }
  if (m_currentItems.contains(location)) { //ignore locations that are already in the list
    return;
  }
  m_currentItems << location; //add new location
  this->addItem(location);
}

void MyListWidget::addDirContent(QString location)
{
  QDirIterator iter(location, QDirIterator::Subdirectories);
  QFileInfo info;
  QString item;
  while (iter.hasNext()) {
    item = iter.next();
    info = QFileInfo(item);
    if (info.isDir()) {
      continue;
    } else {
      this->addLocation(item);
    }
  }
}

void MyListWidget::addLocations(QList<QUrl> urls)
{
  QUrl url;
  QFileInfo info;
  QString location;
  for (int i = 0, j = urls.count(); i < j; i++) {
    url = urls.at(i);
    location = url.toLocalFile().trimmed();
    info = QFileInfo(location);
    if (!info.isDir()) { //normal file
      this->addLocation(location);
      continue;
    }
    this->addDirContent(location);
  }
}

void MyListWidget::dropEvent(QDropEvent *event)
{
  const QMimeData *mData = event->mimeData();
  if (mData->hasUrls()) {
    this->addLocations(event->mimeData()->urls());
  } else {
    QListWidget::dropEvent(event);
    return;
  }
  event->accept();
}

void MyListWidget::dragMoveEvent(QDragMoveEvent *event)
{
  event->accept();
}
void MyListWidget::dragEnterEvent(QDragEnterEvent *event)
{
  event->accept();
}
