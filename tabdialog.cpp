/*
  tabdialog.cpp
 
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

#include <QtGui>
#include "tabdialog.h"
#include "timelapsewidget.h"

TabDialog::TabDialog( QWidget* ) {
    qset = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                         "QtCanon", "QtCanon");

    tlw = new TimeLapseWidget(0, qset);
    connect(tlw, SIGNAL(statusUpdate(QString)), this, SLOT(updateAppStatus(QString)));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(tlw);

    applicationStatus = new QStatusBar(tlw);
    applicationStatus->showMessage( "Ready" );
    mainLayout->addWidget( applicationStatus );
    setLayout(mainLayout);

    qset->beginGroup("MainWindow");
    resize(qset->value("size", QSize(600, 200)).toSize());
    move(qset->value("pos", QPoint(200, 200)).toPoint());
    qset->endGroup();

    setWindowTitle(tr("QtCanon"));
    setWindowIcon(QIcon(":/images/camera.svgz"));
}

TabDialog::~TabDialog() {

    qset->beginGroup("MainWindow");
    qset->setValue("size", size());
    qset->setValue("pos", pos());
    qset->endGroup();

    if (tlw)
        tlw->saveSettings(qset);

    qset->sync();

    delete qset;
}

void TabDialog::updateAppStatus(QString status)
{
    applicationStatus->showMessage( status );
}
