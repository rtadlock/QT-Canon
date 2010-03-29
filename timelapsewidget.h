/*
  timelapsewidget.h
 
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

#ifndef TIME_LAPSE_WIDGET_H
#define TIME_LAPSE_WIDGET_H

#include <QWidget>
#include <QProcess>
#include <QMap>

#include "qccamera.h"

class QAction;
class QCheckBox;
class QComboBox;
class QIcon;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QSettings;
class QSpinBox;
class QStatusBar;
class QTimeEdit;
class QLayout;
class QVBoxLayout;
class QTabWidget;
class QFrame;

#ifndef CANON_SDK_BUILD
class QFileSystemWatcher;
#endif

typedef QVector<QString> str_vect;
typedef str_vect::iterator strv_iter;

class TimeLapseWidget : public QWidget {
    Q_OBJECT;

public:

    enum VideoSize
    {
        Small = 0,
        Medium = 1,
        Large = 2,
        Original = 3
    };

    TimeLapseWidget(QWidget *parent = 0, QSettings *qs=0);

    QFrame *tlConstruct(QWidget *parent, QSettings *qs);
    QFrame *mwConstruct(QWidget *parent, QSettings *qs);

    ~TimeLapseWidget();
    
    void saveSettings(QSettings *qs);

    QStringList available_codecs;
    QMap<QString, QString > codecLookup;
    QMap<QString, QString > extensionLookup;

signals:
    void statusUpdate(QString);

public slots:
    void about();
    void closeEvent(QCloseEvent *event);
    void startShooting();
    void stopShooting();
    void stopPressed();
    void timerEvent(QTimerEvent *event);
    void setDirectory();
    void saveDirectoryUpdated();
    void attachedCameraSelectionChanged();
    void populateCameraList();


    void movieDone(int exitCode, QProcess::ExitStatus exitStatus);
    void movieError(QProcess::ProcessError error);
    void fileInfoChanged();

private:
    void makeMovie();
    void setupConnections();
    void handleException(std::exception *err, QString helpMsg = tr(""));

    void shootingStopped();
    void shootingStarted();

    void removeImages();

    QPushButton *startButton;
    QPushButton *stopButton;
    QPushButton *directoryButton;

    QSpinBox *intervalBox;
    QTimeEdit *lengthOfTimeBox;

    QLineEdit *saveDirectory;

    QComboBox *attachedCameras;

    QRadioButton *seconds;
    QRadioButton *minutes;

    QImage *lastImg;
    QLabel *preview;

#ifndef CANON_SDK_BUILD
    QFileSystemWatcher *camFinder;
#endif

    bool sdkLoaded;
    int timerId;
    int numPics;

    bool running;
    size_t interval;
    QTime * stopTime;

    QCCamera *curCam;

    str_vect cameras;

    QLineEdit *file_name;
    QLineEdit *movie_title;
    QSpinBox *frame_rate;
    QSpinBox *bit_rate;
    QComboBox *codec;
    QComboBox *video_size;
    QCheckBox *remove_pictures;

    QFrame *pictureFrame;
    QFrame *movieFrame;

    QVBoxLayout *mainLayout;

    QTabWidget *tw;

    QStringList imagesCreated;
};

#endif
