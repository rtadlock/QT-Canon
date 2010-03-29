/*
  qccamera.h
 
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

#ifndef QT_CANON_CAMERA_H
#define QT_CANON_CAMERA_H

#include <QString>
#include <QVector>

#ifdef CANON_SDK_BUILD

#include <EDSDK.h>

#else

extern "C" {
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-filesys.h>
#include <gphoto2/gphoto2-port-info-list.h>
#include <gphoto2/gphoto2-port-log.h>
#include <gphoto2/gphoto2-setting.h>
};
#include <QMap>

#endif

class QCCamera
{
    public:
        QCCamera( int, QString camera_name, QString image_dir );
        ~QCCamera();
        
        QString takePhoto();
        bool setSaveDir( const QString save_dir);

        QString name();

        static void initCamLibrary();
        static void cleanupCamLibrary();
        static const QVector<QString> &getAttachedCameras();

    private:
        int id;
        QString camera_name;
        QString saveDir;
        static QVector<QString> camList;


#ifdef CANON_SDK_BUILD
private:
        EdsCameraRef camera;
public:
        static EdsError downloadImage(EdsDirectoryItemRef directoryItem, QString dir_to_save_to);
        static EdsError EDSCALLBACK handleObjectEvent( EdsObjectEvent event, EdsBaseRef object, EdsVoid * context );
        static QString curFile;
#else

// GPhoto private stuff goes here...
private:
        static GPPortInfoList *portinfolist;
        static CameraAbilitiesList	*abilities;

        static QMap<QString, QString> camPorts;

        QString model;
        QString port;
    
        Camera *camera;
        GPContext *cameracontext;
    
        void enable_capture();
        /* void capture_to_file(Camera *canon, GPContext *canoncontext); */
#endif
};

#endif

