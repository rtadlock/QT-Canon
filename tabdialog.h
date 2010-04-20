/*
  tabdialog.h
 
  Copyright (C) 2009 Robert Tadlock and Jeremiah LaRocco

  This file is part of QtCanon

  QtCanon is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  QtCanon is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with QtCanon.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef TABDIALOG_H
#define TABDIALOG_H

#include <QDialog>

class QTabWidget;
class QSettings;
class QStatusBar;

class TimeLapseWidget;
class MovieWidget;

class TabDialog : public QDialog
{
    Q_OBJECT;
public:
    TabDialog( QWidget *parent = 0 );
    ~TabDialog();

public slots:
    void updateAppStatus(QString);
private:
    void readSettings();

    QTabWidget *tabWidget;

    TimeLapseWidget *tlw;

    QSettings *qset;

    QStatusBar *applicationStatus;
};

#endif // TABDIALOG_H
