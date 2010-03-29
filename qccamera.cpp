/*
  qccamera.cpp
 
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

#include "qccamera.h"

#include <QTextStream>
#include <QString>

QVector<QString> QCCamera::camList;

QString QCCamera::name() {
    return camera_name;
}

#ifdef CANON_SDK_BUILD
QString QCCamera::curFile;

EdsError QCCamera::downloadImage(EdsDirectoryItemRef directoryItem, QString dir_to_save_to)
{
    EdsError err = EDS_ERR_OK;
    EdsStreamRef stream = NULL;
    EdsDirectoryItemInfo dirItemInfo;

    err = EdsGetDirectoryItemInfo( directoryItem, & dirItemInfo );

    if(err == EDS_ERR_OK)
    {
        curFile = dir_to_save_to + "/" + dirItemInfo.szFileName;
        
        err = EdsCreateFileStream( curFile.toUtf8(),
                                   kEdsFileCreateDisposition_CreateAlways,
                                   kEdsAccess_ReadWrite, &stream);
    }

    if(err == EDS_ERR_OK)
    {
        err = EdsDownload( directoryItem, dirItemInfo.size, stream);
    }

    if(err == EDS_ERR_OK)
    {
        err = EdsDownloadComplete(directoryItem);
    }

    if( stream != NULL)
    {
        EdsRelease(stream);
        stream = NULL;
    }
    return err;
}

EdsError EDSCALLBACK QCCamera::handleObjectEvent( EdsObjectEvent event, EdsBaseRef object, EdsVoid * context )
{
    EdsError err = EDS_ERR_OK;
    
    switch(event)
    {
        case kEdsObjectEvent_DirItemRequestTransfer:
        {
            QString savedir = *((QString*)context);
            err = downloadImage( object, savedir );
            break;
        }
        default:
            break;
    }

    // Object must be released
    if(object)
    {
        EdsRelease(object);
    }

    return err;
}

QCCamera::QCCamera( int childID, QString local_camera_name, QString image_dir )
{
    if( local_camera_name.compare("") != 0 )
        camera_name = local_camera_name;

    if( childID >= 0 )
        id = childID;
    else
        id = -1;

    if( id >= 0 && image_dir.compare( "" ) != 0 )
    {
        EdsError err = EDS_ERR_OK;
        EdsCameraListRef cameraList = NULL;
        EdsUInt32 count = 0;
        err = EdsGetCameraList(&cameraList);

        // Get number of cameras
        if(err == EDS_ERR_OK)
        {
            err = EdsGetChildCount(cameraList, &count);
            if(count == 0)
            {
                err = EDS_ERR_DEVICE_NOT_FOUND;
            }
        }

        if(err == EDS_ERR_OK)
        {
            err = EdsGetChildAtIndex(cameraList , id , &camera);
        }

        // Release camera list
        if(cameraList != NULL)
        {
            EdsRelease(cameraList);
            cameraList = NULL;
        }

        if(err == EDS_ERR_OK)
        {
            err = EdsOpenSession( camera);
        }

        EdsUInt32 storage = kEdsSaveTo_Host;

        err = EdsSetPropertyData(camera, kEdsPropID_SaveTo, 0 , sizeof(storage), &storage);
        setSaveDir( image_dir );
    }
}

bool QCCamera::setSaveDir( const QString save_dir )
{

        EdsError err = EDS_ERR_OK;

        saveDir = save_dir;

        err = EdsSetObjectEventHandler( camera, kEdsObjectEvent_All, handleObjectEvent, &saveDir );
        
        if(err == EDS_ERR_OK)
            return true;
        else
            return false;
}

QString QCCamera::takePhoto()
{
    EdsError err = EDS_ERR_OK;
    err = EdsSendCommand( camera, kEdsCameraCommand_TakePicture , 0);

    return curFile;
}

QCCamera::~QCCamera()
{
    EdsCloseSession( camera );
}

void QCCamera::initCamLibrary() {
    EdsInitializeSDK();
}

void QCCamera::cleanupCamLibrary() {
    EdsTerminateSDK();    
}

const QVector<QString> &QCCamera::getAttachedCameras()
{
    //verify that the SDK has been intialized
    camList.clear();

    EdsError	 err = EDS_ERR_OK;
    EdsUInt32	 count = 0;
    EdsCameraListRef cameraList = NULL;
    EdsCameraRef camera = NULL;
    EdsDeviceInfo deviceInfo;

    //Acquisition of camera list
    if( err == EDS_ERR_OK )
    {
        err = EdsGetCameraList( &cameraList );
        if( err == EDS_ERR_OK )
        {
            err = EdsGetChildCount( cameraList, &count );
            if( count > 0 )
            {
                for( EdsUInt32 i = 0; i< count; i++ )
                {
                    err = EdsGetChildAtIndex( cameraList , i , &camera );
                    if( err == EDS_ERR_OK )
                    {
                        err = EdsGetDeviceInfo( camera , &deviceInfo );
                        if( err == EDS_ERR_OK && camera == NULL )
                            err = EDS_ERR_DEVICE_NOT_FOUND;
                        else
                            camList.push_back( deviceInfo.szDeviceDescription );
                    }
                }
            }
            else
            {
                camList.push_back( "No camera detected" );
            }

            if( cameraList != NULL )
            {
                EdsRelease(cameraList);
                cameraList = NULL;
            }
        }
    }
    else
    {
        camList.push_back( "No camera detected" );
    }

    return camList;
}

#else

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extern "C" {
#include <gphoto2/gphoto2.h>
};

#include <stdexcept>
#include <cstring>

GPPortInfoList *QCCamera::portinfolist;
CameraAbilitiesList	*QCCamera::abilities;
QMap<QString, QString> QCCamera::camPorts;

static int herr(int ret, const char *fname, const int lnum);

QCCamera::QCCamera( int, QString local_camera_name, QString image_dir )
{

    if (local_camera_name == "") throw std::runtime_error("No camera specified.");

    QString model = local_camera_name;
    if (camPorts.find(model) ==  camPorts.end()) return;
    
    QString port = camPorts[model];

	int m, p;

	CameraAbilities	a;
    cameracontext = gp_context_new();

	herr(gp_camera_new(&camera), "gp_camera_new", __LINE__);;
    
    m = herr(gp_abilities_list_lookup_model (abilities, model.toUtf8()), "gp_abilities_list_lookup_model", __LINE__);

    herr(gp_abilities_list_get_abilities(abilities, m, &a), "gp_abilities_list_get_abilities", __LINE__);
                                                  
    herr(gp_camera_set_abilities(camera, a), "gp_camera_set_abilities", __LINE__);

    p = herr(gp_port_info_list_lookup_path(portinfolist, port.toUtf8()), "gp_port_info_list_lookup_path", __LINE__);

    GPPortInfo gpi;
    herr(gp_port_info_list_get_info(portinfolist, p , &gpi), "gp_port_info_list_get_info", __LINE__);
    
    herr(gp_camera_set_port_info(camera, gpi), "gp_camera_set_port_info", __LINE__);

    herr(gp_camera_init(camera, cameracontext), "gp_camera_init", __LINE__);

    setSaveDir( image_dir );

    enable_capture();
}

QCCamera::~QCCamera()
{
    gp_camera_exit(camera, cameracontext);
}

void QCCamera::initCamLibrary() {
}

void QCCamera::cleanupCamLibrary() {
}

const QVector<QString> &QCCamera::getAttachedCameras() {

    QCCamera::camList.clear();

	CameraList *xlist = NULL;

	herr(gp_list_new(&xlist), "gp_list_new", __LINE__);

	if (!portinfolist) {
		/* Load all the port drivers we have... */
		herr(gp_port_info_list_new(&portinfolist), "gp_port_info_list_new", __LINE__);
		herr(gp_port_info_list_load(portinfolist), "gp_port_info_list_load", __LINE__);

        /* Load all the camera drivers we have... */
        herr(gp_abilities_list_new(&abilities), "gp_abilities_list_new", __LINE__);
        herr(gp_abilities_list_load(abilities, 0), "gp_abilities_list_load", __LINE__);
	}

	// /* ... and autodetect the currently attached cameras. */
    herr(gp_abilities_list_detect(abilities, portinfolist, xlist, 0), "gp_abilities_list_detect", __LINE__);

	/* Filter out the "usb:" entry */
    int cnt = herr(gp_list_count(xlist), "gp_list_count", __LINE__);
	for (int i=0;i<cnt;i++) {
		const char *name, *value;

		herr(gp_list_get_name(xlist, i, &name), "gp_list_get_name", __LINE__);
		herr(gp_list_get_value(xlist, i, &value), "gp_list_get_value", __LINE__);

        // "usb:" appears to be some kind of gphoto bug?
        // if (!std::strcmp ("usb:",value)) continue;
        
        camList.push_back(name);
        QCCamera::camPorts[name] = value;
	}
	herr(gp_list_free(xlist), "gp_list_free", __LINE__);

    return camList;
}

bool QCCamera::setSaveDir( const QString save_dir)
{
    saveDir = save_dir;
    return true;
}

QString QCCamera::takePhoto()
{
    CameraFilePath camera_file_path;
    CameraFile *camerafile;
    const char *filedata;
    unsigned long int filesize;
    static int curPhoto = 0;

    // THe sample code I used said this is required, but ignored...
    std::strcpy(camera_file_path.folder, saveDir.toUtf8());
    std::strcpy(camera_file_path.name, "foo.jpg");

    herr(gp_camera_capture(camera, GP_CAPTURE_IMAGE, &camera_file_path, 
                           cameracontext), "gp_camera_capture", __LINE__);

    herr(gp_file_new(&camerafile), "gp_file_new", __LINE__);

    herr(gp_camera_file_get(camera, camera_file_path.folder, 
                            camera_file_path.name,
                            GP_FILE_TYPE_NORMAL, camerafile, cameracontext),"gp_camera_file_get", __LINE__);

    herr(gp_file_get_data_and_size(camerafile, &filedata, &filesize), "gp_file_get_data_and_size", __LINE__);

    QString fileName;
    QTextStream out(&fileName);
    out.setPadChar('0');
    out << saveDir << "/IMG_" << qSetFieldWidth(4) << curPhoto << ".JPG";
    ++curPhoto;
    int fd = open(fileName.toUtf8(), O_CREAT | O_WRONLY, 0644);
    write(fd, filedata, filesize);
    close(fd);

    herr(gp_camera_file_delete(camera, camera_file_path.folder, 
                               camera_file_path.name,
                               cameracontext), "gp_camera_file_delete", __LINE__);
    gp_file_free(camerafile);
    return fileName;
}

void QCCamera::enable_capture() {
    CameraWidget *rootconfig; // okay, not really
    CameraWidget *actualrootconfig;
    CameraWidget *child;
    CameraWidget *capture = child;
    
    const char *widgetinfo;
    const char *widgetlabel;

    herr(gp_camera_get_config(camera, &rootconfig, cameracontext), "gp_camera_get_config", __LINE__);
    
    actualrootconfig = rootconfig;

    herr(gp_widget_get_child_by_name(rootconfig, "main", &child), "gp_widget_get_child_by_name", __LINE__);

    rootconfig = child;
    herr(gp_widget_get_child_by_name(rootconfig, "settings", &child), "gp_widget_get_child_by_name", __LINE__);

    rootconfig = child;
    herr(gp_widget_get_child_by_name(rootconfig, "capture", &child), "gp_widget_get_child_by_name", __LINE__);

    capture = child;

    gp_widget_get_name(capture, &widgetinfo);

    gp_widget_get_label(capture, &widgetlabel);

    int widgetid;
    gp_widget_get_id(capture, &widgetid);

    CameraWidgetType widgettype;
    gp_widget_get_type(capture, &widgettype);

    int one=1;
    herr(gp_widget_set_value(capture, &one), "gp_widget_set_value", __LINE__);

    herr(gp_camera_set_config(camera, actualrootconfig, cameracontext), "gp_camera_set_config", __LINE__);
}

int herr(int ret, const char *fname, const int lnum) {
    if (ret<GP_OK) {
        QString tmp;
        QTextStream out(&tmp);
        out << fname << " failed on line " << lnum << ".";
        throw std::runtime_error((const char*)tmp.toUtf8());
    }
    return ret;
}

#endif
