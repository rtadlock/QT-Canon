/*
  timelapsewidget.cpp
 
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
#include <QSettings>

#include <stdexcept>
#include <iostream>

#include "timelapsewidget.h"
#include "tabdialog.h"

TimeLapseWidget::TimeLapseWidget(QWidget *parent, QSettings *qs) : QWidget(parent), sdkLoaded(false), timerId(-1), running(false), curCam(0)
{
    mainLayout = new QVBoxLayout;

    
    pictureFrame = tlConstruct(parent, qs);

    movieFrame = mwConstruct(parent, qs);

    tw = new QTabWidget;
    tw->addTab(pictureFrame, tr("Shooting Options"));
    tw->addTab(movieFrame, tr("Movie Options"));

    mainLayout->addWidget(tw);

    setLayout(mainLayout);
}

QFrame *TimeLapseWidget::tlConstruct(QWidget *, QSettings *qs) {
    
    QCCamera::initCamLibrary();

    // Initialize GUI stuff
    QGridLayout *layout = new QGridLayout();
    
    layout->addWidget(new QLabel("Available Cameras"),0,0,1,1);

    attachedCameras = new QComboBox;
    populateCameraList();
    layout->addWidget(attachedCameras, 0,1,1,2);

    layout->addWidget(new QLabel("Output directory"),1,0,1,1);
    saveDirectory = new QLineEdit();
    layout->addWidget( saveDirectory,1,1,1,1 );

    QCompleter *completer = new QCompleter(this);
    completer->setModel(new QDirModel(completer));
    saveDirectory->setCompleter(completer);

    directoryButton = new QPushButton( tr("Browse...") );

    layout->addWidget( directoryButton,1,2,1,1 );
    
    intervalBox = new QSpinBox;
    intervalBox->setMinimum(1);
    intervalBox->setMaximum(99999);
    
    lengthOfTimeBox = new QTimeEdit( QTime(0, 0, 0, 0), this );
    lengthOfTimeBox->setDisplayFormat( "HH:mm:ss" );

    layout->addWidget(new QLabel( "Interval" ), 2,0);
    layout->addWidget(intervalBox, 2,1);

    seconds = new QRadioButton( "S" );
    seconds->setChecked( true );
    minutes = new QRadioButton( "M" );

    layout->addWidget( seconds, 2, 2, 1, 1, Qt::AlignLeft );
    layout->addWidget( minutes, 2, 2, 1, 1, Qt::AlignRight );

    layout->addWidget(new QLabel("Time to run"), 3,0);
    layout->addWidget( lengthOfTimeBox, 3,1);

    startButton = new QPushButton(QIcon(":/images/play.svgz"), "Start");
    stopButton = new QPushButton(QIcon(":/images/stop.svgz"), "Stop");

    layout->addWidget(startButton, 4,0);
    layout->addWidget(stopButton, 4,1);

    intervalBox->setFocus();

    setupConnections();

    startButton->setDisabled( false );
    stopButton->setDisabled( true );

    saveDirectory->setText(".");

    qs->beginGroup("TimeLapseTab");
    intervalBox->setValue(qs->value("intervalBox", 15).toInt());
    lengthOfTimeBox->setTime(qs->value("lengthOfTimeBox", QTime(0,0,0,0)).toTime());
    saveDirectory->setText(qs->value("saveDirectory", ".").toString());
    seconds->setChecked(qs->value("seconds", true).toBool());
    minutes->setChecked(qs->value("minutes", false).toBool());
    qs->endGroup();

    saveDirectoryUpdated();

#ifndef CANON_SDK_BUILD
    camFinder = new QFileSystemWatcher(this);
    QFileInfoList dirs = QDir("/dev/bus/usb/").entryInfoList();
    for (QFileInfoList::iterator iter = dirs.begin();
         iter != dirs.end();
         ++iter) {
        camFinder->addPath(iter->absoluteFilePath());
    }
    connect(camFinder, SIGNAL(directoryChanged(QString)),
            this, SLOT(populateCameraList()));
#endif

    QFrame *rv = new QFrame;
    rv->setLayout(layout);
    return rv;
}

QFrame *TimeLapseWidget::mwConstruct(QWidget *, QSettings *qs) {
    // Initialize GUI stuff
    QGridLayout *layout = new QGridLayout();

    available_codecs << tr("WMV") << tr("MPEG4") << tr("MJPEG") << tr("FLV") << tr("FFV1") << tr("H264");

    codecLookup[tr("WMV")] = tr("wmv2");
    codecLookup[tr("MPEG4")] = tr("mpeg4");
    codecLookup[tr("MJPEG")] = tr("mjpeg");
    codecLookup[tr("FLV")] = tr("flv");
    codecLookup[tr("FFV1")] = tr("ffv1");
    codecLookup[tr("H264")] = tr("libx264");

    extensionLookup[tr("WMV")] = tr("wmv");
    extensionLookup[tr("MPEG4")] = tr("mpg");
    extensionLookup[tr("MJPEG")] = tr("mjpeg");
    extensionLookup[tr("FLV")] = tr("flv");
    extensionLookup[tr("FFV1")] = tr("avi");
    extensionLookup[tr("H264")] = tr("mp4");

    int curY=0;

    layout->addWidget(new QLabel("Movie title "),curY,0,1,1);
    movie_title = new QLineEdit;
    movie_title->setText("TimeLapse_" + QDateTime::currentDateTime().toString("yyyy_MM_dd_hh.mm.ss"));
    layout->addWidget(movie_title,curY++,1,1,1);

    connect(movie_title, SIGNAL(textChanged(QString)), this, SLOT(fileInfoChanged()));

    layout->addWidget(new QLabel("Codecs "),curY,0,1,1);

    codec = new QComboBox;
    codec->addItems(available_codecs);
    layout->addWidget(codec, curY++,1,1,2 );
    connect(codec, SIGNAL(currentIndexChanged(QString)), this, SLOT(fileInfoChanged()));

    layout->addWidget(new QLabel("Output filename "),curY,0,1,1);
    file_name = new QLineEdit;
    layout->addWidget(file_name, curY++,1,1,1);
    file_name->setEnabled(false);
    fileInfoChanged();

    layout->addWidget(new QLabel("Movie size "),curY,0,1,1);
    video_size = new QComboBox;
    video_size->addItem(tr("Small (640x480)"));
    video_size->addItem(tr("Medium (1024x768)"));
    video_size->addItem(tr("Large (1280x1024)"));
    video_size->addItem(tr("Original"));
                        
    layout->addWidget(video_size, curY++,1,1,1);

    layout->addWidget(new QLabel("Frame rate "),curY,0,1,1);
    frame_rate = new QSpinBox;
    frame_rate->setRange(1,30);
    frame_rate->setValue(12);
    layout->addWidget(frame_rate,curY++,1,1,1);

    layout->addWidget(new QLabel("Bit rate "),curY,0,1,1);
    bit_rate = new QSpinBox;
    bit_rate->setRange(100,2000);
    bit_rate->setValue(1200);
    bit_rate->setSingleStep(100);
    layout->addWidget(bit_rate,curY++,1,1,1);

    remove_pictures = new QCheckBox( "Remove images after encode" );
    remove_pictures->setChecked(true);
    layout->addWidget(remove_pictures, curY++,0,1,2);

    qs->beginGroup("MovieTab");
    codec->setCurrentIndex(qs->value("codec", 0).toInt());
    frame_rate->setValue(qs->value("frame_rate", "12").toInt());
    bit_rate->setValue(qs->value("bit_rate", "1200").toInt());
    video_size->setCurrentIndex(qs->value("video_size", 0).toInt());
    remove_pictures->setChecked(qs->value("remove_pictures").toBool());
    qs->endGroup();

    QFrame *rv = new QFrame;
    rv->setLayout(layout);
    return rv;
}


void TimeLapseWidget::setupConnections()
{
    connect(startButton, SIGNAL(clicked()), this, SLOT(startShooting()));
    connect(stopButton, SIGNAL(clicked()), this, SLOT(stopPressed()));
    connect(directoryButton, SIGNAL(clicked()), this, SLOT(setDirectory()));
    connect(saveDirectory, SIGNAL(textChanged(QString)), this, SLOT(saveDirectoryUpdated()));
    connect(attachedCameras, SIGNAL(currentIndexChanged(QString)), this, SLOT(attachedCameraSelectionChanged()));
}

TimeLapseWidget::~TimeLapseWidget()
{
    if (curCam)
    {
        delete curCam;
    }
    QCCamera::cleanupCamLibrary();
}

/*!
  Display the about box
*/
void TimeLapseWidget::about()
{
    QMessageBox::about(this, tr("About QtCanon"),
                       tr("<h2>QtCanon 0.1</h2>"
                          "<p>Copyright &copy; 2009 Robert Tadlock and Jeremiah LaRocco</p>"
                          ));
}

void TimeLapseWidget::closeEvent(QCloseEvent *event)
{
    event->accept();
}

void TimeLapseWidget::setDirectory()
{
    QFileDialog::Options options = QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly;
    QString directory =
            QFileDialog::getExistingDirectory(this,
                                              tr("Save pictures..."),
                                              saveDirectory->text(),
                                              options);
    if(  !directory.isEmpty() )
    {
        QFileInfo qfi(directory);

         if ( qfi.exists() && qfi.isWritable() )
         {
             saveDirectory->setText( directory );
             if( attachedCameras->currentText().compare( "No camera detected" ) != 0 )
             {
                 startButton->setDisabled( false );
             }
         }
         else
         {
             QMessageBox::critical( this, tr("Error"),tr("Please select a writable directory") );
             saveDirectory->clear();
             startButton->setDisabled( true );
             stopButton->setDisabled( true );
         }
     }
}

void TimeLapseWidget::attachedCameraSelectionChanged()
{
}

void TimeLapseWidget::saveDirectoryUpdated()
{
    QFileInfo qfi(saveDirectory->text());

    if( qfi.exists() && qfi.isDir()
        && qfi.isWritable()
        && attachedCameras->currentText().compare( "No camera detected" ) != 0 )
    {
        startButton->setDisabled( false );
    }
    else
    {
        startButton->setDisabled( true );
    }
}

void TimeLapseWidget::startShooting()
{
    if (running) {
        return;
    }

    if( !curCam ) {
        try {

            curCam = new QCCamera( attachedCameras->currentIndex(),
                                   attachedCameras->currentText(),
                                   saveDirectory->text());

        } catch (std::runtime_error re) {

            handleException(&re, tr("Check that no other instances of the program are running, and that your camera is plugged in and turned on."));
            return;
        }
    } else {

        try {

            curCam->setSaveDir( saveDirectory->text());

        } catch (std::runtime_error re) {
            handleException(&re);
            return;
        }
    }

    imagesCreated.clear();
    shootingStarted();

    startButton->setEnabled( false );
    stopButton->setEnabled( true );
    directoryButton->setEnabled( false );
    intervalBox->setEnabled( false );
    lengthOfTimeBox->setEnabled( false );
    saveDirectory->setEnabled( false );
    attachedCameras->setEnabled( false );
    seconds->setEnabled( false );
    minutes->setEnabled( false );

    interval = intervalBox->value();

    numPics = 0;
    emit statusUpdate(tr("Pictures taken: %1").arg(numPics) );

    // Add 2 seconds to account for shutter delay and lag

    // TODO: Figure out exactly what timing we want.
    // For the short intervals I'm testing with, there are cases where I take a picture every 2 seconds, for 10 seconds, and it only takes 4 pictures.
    // Even for longer intervals and durations, the number of pictures has been one or two less than expected.
    // We need to figure out:
    //    Is it more important to take a specific number of pictures, or run for a certain amount of time?
    //    Does it even matter?
    //    Maybe we can specify a run time and a "movie time"?  Like "Take pictures for an hour and make a movie 60 seconds long"
    //       That doesn't solve the problem entirely, but doesn't emphasize the exact number of pictures so much
    // Also, should the first picture be taken immediately when the "Start" button is pressed, or should it wait for one interval first?
    stopTime = new QTime( QTime::currentTime().addSecs( ( lengthOfTimeBox->time().hour() * 60 * 60 ) + ( lengthOfTimeBox->time().minute() * 60 ) + lengthOfTimeBox->time().second() + 2) );

    if( seconds->isChecked() )
        timerId = startTimer( 1000 * interval );
    else
        timerId = startTimer( 1000 * (interval * 60) );

    running = true;
}

void TimeLapseWidget::stopShooting()
{
    if (timerId != -1)
    {
        startButton->setEnabled( true );
        stopButton->setEnabled( false );
        directoryButton->setEnabled( true );
        intervalBox->setEnabled( true );
        lengthOfTimeBox->setEnabled( true );
        saveDirectory->setEnabled( true );
        attachedCameras->setEnabled( true );
        seconds->setEnabled( true );
        minutes->setEnabled( true );
        
        shootingStopped();
        killTimer(timerId);
        running = false;
        timerId = -1;
        delete stopTime;
    }
}
void TimeLapseWidget::stopPressed() {

    stopShooting();

    // Only ask about the movie if there are any pictures to make into a movie
    if (numPics>0) {
        QMessageBox msgBox;
        msgBox.setText(tr("%1 images were taken before \"Stop\" was pressed.\n").arg(numPics));
        msgBox.setInformativeText(tr("Do you want to make a movie with these images?"));

        // msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        QPushButton *yesButton = msgBox.addButton(QMessageBox::Yes);
        msgBox.addButton(QMessageBox::No);
        QPushButton *noAndDeleteButton = msgBox.addButton(tr("No, and delete images"), QMessageBox::ActionRole);

        msgBox.exec();

        if (msgBox.clickedButton() == yesButton) {
            makeMovie();
        } else if (msgBox.clickedButton() == noAndDeleteButton) {
            removeImages();
        }
    }
}

void TimeLapseWidget::timerEvent( QTimerEvent *event )
{
  if (event->timerId() != timerId)
  {
      QWidget::timerEvent(event);
      return;
  }

  if ( !running || !curCam ) return;
  
  if ( QTime::currentTime().secsTo( *stopTime ) <= 0 )
  {
      stopShooting();
      makeMovie();
  }
  else
  {
      numPics++;
      emit statusUpdate(tr("Pictures taken: %1").arg(numPics) );
      try
      {
          imagesCreated << curCam->takePhoto();
          
      }
      catch (std::runtime_error re)
      {
          handleException(&re);
          // stop shooting, otherwise the suer could get a stupid number of dialog boxes.
          stopShooting();
      }
  }
}

void TimeLapseWidget::handleException(std::exception *err, QString helpMsg)
{
    QString errMsg = tr("The following exception occured:\n%1\n\n%2").arg(QString(err->what())).arg(helpMsg);
    QMessageBox::critical( this, tr("Error"), errMsg);
}


void TimeLapseWidget::saveSettings(QSettings *qs) {
    qs->beginGroup("TimeLapseTab");
    qs->setValue("intervalBox", intervalBox->text());
    qs->setValue("lengthOfTimeBox", lengthOfTimeBox->time());
    qs->setValue("saveDirectory", saveDirectory->text());
    qs->setValue("seconds", seconds->isChecked());
    qs->setValue("minutes", minutes->isChecked());
    qs->endGroup();

    qs->beginGroup("MovieTab");
    qs->setValue("codec", codec->currentIndex());
    qs->setValue("frame_rate", frame_rate->text());
    qs->setValue("bit_rate", bit_rate->text());
    qs->setValue("video_size", video_size->currentIndex());
    qs->setValue("remove_pictures", remove_pictures->isChecked());
    qs->endGroup();

}

void TimeLapseWidget::populateCameraList() {
    
    cameras = QCCamera::getAttachedCameras();

    bool curCamExists = false;
    
    attachedCameras->clear();

    for (strv_iter iter = cameras.begin(); iter != cameras.end(); ++iter) {
        if (curCam) {
            if (*iter == curCam->name()) {
                curCamExists = true;
            }
        }
        attachedCameras->addItem(*iter);
    }

    // If the current camera is no longer in the list, stop shooting.
    if (curCam && running && !curCamExists ) {
        stopShooting();
        delete curCam;
        curCam = 0;
    } 
}

void TimeLapseWidget::makeMovie() {
    QString picture_dir = saveDirectory->text();
    static QMap<VideoSize, QString> sizeArgs;
    if (sizeArgs.size()==0) {
        sizeArgs[Small] = tr("640x480");
        sizeArgs[Medium] = tr("1024x768");
        sizeArgs[Large] = tr("1280x1024");
    }

    try {

        QProcess *ffmpeg = new QProcess(this);
        QStringList args;
        
        args << tr("-y")
             << tr("-r") << frame_rate->text()
             << tr("-b") <<  bit_rate->text()
             << tr("-i") << tr("IMG_%04d.JPG")
             << tr("-vcodec") << codecLookup[codec->currentText()];

        if (video_size->currentText()!=tr("Original")) {
            args << tr("-s") << sizeArgs[VideoSize(video_size->currentIndex())];
        }
        args << file_name->text();
        ffmpeg->setWorkingDirectory(picture_dir);

        emit statusUpdate(tr("Encoding...") );

        connect(ffmpeg, SIGNAL(finished(int, QProcess::ExitStatus)),
                this, SLOT(movieDone(int, QProcess::ExitStatus)));
        connect(ffmpeg, SIGNAL(error(QProcess::ProcessError)),
                this, SLOT(movieError(QProcess::ProcessError)));
        ffmpeg->start("ffmpeg", args, QIODevice::ReadOnly);


    } catch (std::runtime_error re) {
        emit statusUpdate(tr("An error occured while running ffmpeg") );
        return;
    }
}

void TimeLapseWidget::removeImages() {
    foreach (QString img, imagesCreated) {
        std::cout << "Removing " << img.toStdString() << std::endl;
        QFile::remove(img);
    }
}

void TimeLapseWidget::movieDone(int , QProcess::ExitStatus stat) {

    if ( stat == QProcess::NormalExit) {
        if( remove_pictures->isChecked() && !saveDirectory->text().isEmpty() ) {
            removeImages();
        }
        emit statusUpdate(tr("Finished encoding.") );
    } else {
        emit statusUpdate(tr("Encoding failed") );
        QMessageBox::critical(this, tr("FFMPEG Error"),
                              tr("FFMPEG crashed!\nNo images are being deleted, and are located in \"%1\".").arg(saveDirectory->text()));
    }
}

void TimeLapseWidget::movieError(QProcess::ProcessError error) {
    static QMap<QProcess::ProcessError, QString> errs;
    if (errs.size()==0) {
        errs[QProcess::FailedToStart] = tr("Could not start FFMPEG.  Check that it is installed and the binary is in your PATH and retry.");
        errs[QProcess::Crashed] = tr("FFMPEG crashed.  Check that it is installed correctly and retry.");
        errs[QProcess::Timedout] = tr("FFMPEG time out.  Check that it is installed correctly and retry.");
        errs[QProcess::WriteError] = tr("FFMPEG Write Error.  Check that FFMPEG is installed and in your PATH and retry.");
        errs[QProcess::ReadError] = tr("FFMPEG Read Error.  Check that FFMPEG is installed and in your PATH and retry.");
        errs[QProcess::UnknownError] = tr("FFMPEG Unknown Error.  Check that FFMPEG is installed and in your PATH and retry.");
    }
    QMessageBox::critical(this, tr("FFMPEG Error!"),
                          errs[error] + tr("\nNo images are being deleted, and are located in \"%1\".").arg(saveDirectory->text()));
    emit statusUpdate(tr("Encoding failed!!") );
}

void TimeLapseWidget::fileInfoChanged() {
    //change the extension of the file_name to match the chosen codec
    file_name->setText(movie_title->text().toLower() + tr(".") + extensionLookup[codec->currentText()]);
}

void TimeLapseWidget::shootingStopped() {
    tw->setTabEnabled(1, true);
}

void TimeLapseWidget::shootingStarted() {
    
    tw->setTabEnabled(1, false);
}

