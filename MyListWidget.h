/*
 * MyListWidget.h
 *
 *  Created on: Dec 25, 2011
 *      Author: Selur
 */

#ifndef MYLISTWIDGET_H_
#define MYLISTWIDGET_H_

#include <QList>
#include <QListWidget>
class QDropEvent;
class QDragEnterEvent;
class QStringList;
class QUrl;

class MyListWidget : public QListWidget
{
  Q_OBJECT
  public:
    MyListWidget(QWidget *parent = 0);
    virtual ~MyListWidget();
    void remove(QListWidgetItem *item);

  protected:
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dropEvent(QDropEvent *event);

  private:
    QStringList m_currentItems;
    void addLocations(QList<QUrl> urls);
    void addDirContent(QString location);
    void addLocation(QString location);

};

#endif /* MYLISTWIDGET_H_ */
