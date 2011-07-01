/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*=========================================================================

  Copyright 2004 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/

/*========================================================================
 For general information about using VTK and Qt, see:
 http://www.trolltech.com/products/3rdparty/vtksupport.html
=========================================================================*/

// .NAME QVTKWidget - Display a vtkRenderWindow in a Qt's QWidget.
// .SECTION Description
// QVTKWidget provides a way to display VTK data in a Qt widget.

#ifndef Q_VTK_WIDGET_H
#define Q_VTK_WIDGET_H

#include "QVTKInteractor.h"
#include <QtGui/QWidget>

class QVTKInteractorAdapter;

class vtkRenderWindow;
#include <vtkConfigure.h>
#include <vtkToolkits.h>
class vtkImageData;

#include "vtkTDxConfigure.h" // defines VTK_USE_TDX
#ifdef VTK_USE_TDX
class vtkTDxDevice;
#endif

#if defined(Q_WS_MAC)
# if defined(QT_MAC_USE_COCOA) && defined(VTK_USE_COCOA)
#  define QVTK_USE_COCOA
# elif !defined(QT_MAC_USE_COCOA) && defined(VTK_USE_CARBON)
#  define QVTK_USE_CARBON
# elif defined(VTK_USE_COCOA)
#  error "VTK configured to use Cocoa, but Qt configured to use Carbon"
# elif defined(VTK_USE_CARBON)
#  error "VTK configured to use Carbon, but Qt configured to use Cocoa"
# endif
#endif


#if defined(QVTK_USE_CARBON)
#include <Carbon/Carbon.h>    // Event handling for dirty region
#endif

#include "QVTKWin32Header.h"

//! QVTKWidget displays a VTK window in a Qt window.
class QVTK_EXPORT QVTKWidget : public QWidget
{
  Q_OBJECT

  Q_PROPERTY(bool automaticImageCacheEnabled
             READ isAutomaticImageCacheEnabled
             WRITE setAutomaticImageCacheEnabled)
  Q_PROPERTY(double maxRenderRateForImageCache
             READ maxRenderRateForImageCache
             WRITE setMaxRenderRateForImageCache)

public:
  //! constructor
  QVTKWidget(QWidget* parent = NULL, Qt::WFlags f = 0);
  //! destructor
  virtual ~QVTKWidget();

  // Description:
  // Set the vtk render window, if you wish to use your own vtkRenderWindow
  virtual void SetRenderWindow(vtkRenderWindow*);
    
  // Description:
  // Get the vtk render window.
  virtual vtkRenderWindow* GetRenderWindow();
    
  // Description:
  // Get the Qt/vtk interactor that was either created by default or set by the user
  virtual QVTKInteractor* GetInteractor();
    
  // Description:
  // Enum for additional event types supported.
  // These events can be picked up by command observers on the interactor
  enum vtkCustomEvents
  {
    ContextMenuEvent = QVTKInteractor::ContextMenuEvent,
    DragEnterEvent = QVTKInteractor::DragEnterEvent,
    DragMoveEvent = QVTKInteractor::DragMoveEvent,
    DragLeaveEvent = QVTKInteractor::DragLeaveEvent,
    DropEvent = QVTKInteractor::DropEvent
  };

  // Description:
  // Enables/disables automatic image caching.  If disabled (the default),
  // QVTKWidget will not call saveImageToCache() on its own.
  virtual void setAutomaticImageCacheEnabled(bool flag);
  virtual bool isAutomaticImageCacheEnabled() const;

  // Description:
  // If automatic image caching is enabled, then the image will be cached
  // after every render with a DesiredUpdateRate that is greater than
  // this parameter.  By default, the vtkRenderWindowInteractor will
  // change the desired render rate depending on the user's
  // interactions. (See vtkRenderWindow::DesiredUpdateRate,
  // vtkRenderWindowInteractor::DesiredUpdateRate and
  // vtkRenderWindowInteractor::StillUpdateRate for more details.)
  virtual void setMaxRenderRateForImageCache(double rate);
  virtual double maxRenderRateForImageCache() const;

  // Description:
  // Returns the current image in the window.  If the image cache is up
  // to date, that is returned to avoid grabbing other windows.
  virtual vtkImageData* cachedImage();
    
  // Description:
  // Handle showing of the Widget
  virtual void showEvent(QShowEvent*);

  virtual QPaintEngine* paintEngine() const;
  
  // Description:
  // Use a 3DConnexion device. Initial value is false.
  // If VTK is not build with the TDx option, this is no-op.
  // If VTK is build with the TDx option, and a device is not connected,
  // a warning is emitted.
  // It is must be called before the first Render to be effective, otherwise
  // it is ignored.
  void SetUseTDx(bool useTDx);
  bool GetUseTDx() const;
  

Q_SIGNALS:
  // Description:
  // This signal will be emitted whenever a mouse event occurs
  // within the QVTK window
  void mouseEvent(QMouseEvent* event);

  // Description:
  // This signal will be emitted whenever the cached image goes from clean
  // to dirty.
  void cachedImageDirty();

  // Description:
  // This signal will be emitted whenever the cached image is refreshed.
  void cachedImageClean();

public Q_SLOTS:
  // Description:
  // This will mark the cached image as dirty.  This slot is automatically
  // invoked whenever the render window has a render event or the widget is
  // resized.  Your application should invoke this slot whenever the image in
  // the render window is changed by some other means.  If the image goes
  // from clean to dirty, the cachedImageDirty() signal is emitted.
  void markCachedImageAsDirty();

  // Description:
  // If the cached image is dirty, it is updated with the current image in
  // the render window and the cachedImageClean() signal is emitted.
  void saveImageToCache();

  // Description:
  // Receive notification of the creation of the TDxDevice.
  // Only relevant for Unix.
#ifdef VTK_USE_TDX
  void setDevice(vtkTDxDevice *device);
#endif

protected:
  // overloaded resize handler
  virtual void resizeEvent(QResizeEvent* event);
  // overloaded move handler
  virtual void moveEvent(QMoveEvent* event);
  // overloaded paint handler
  virtual void paintEvent(QPaintEvent* event);

  // overloaded mouse press handler
  virtual void mousePressEvent(QMouseEvent* event);
  // overloaded mouse move handler
  virtual void mouseMoveEvent(QMouseEvent* event);
  // overloaded mouse release handler
  virtual void mouseReleaseEvent(QMouseEvent* event);
  // overloaded key press handler
  virtual void keyPressEvent(QKeyEvent* event);
  // overloaded key release handler
  virtual void keyReleaseEvent(QKeyEvent* event);
  // overloaded enter event
  virtual void enterEvent(QEvent*);
  // overloaded leave event
  virtual void leaveEvent(QEvent*);
#ifndef QT_NO_WHEELEVENT
  // overload wheel mouse event
  virtual void wheelEvent(QWheelEvent*);
#endif
  // overload focus event
  virtual void focusInEvent(QFocusEvent*);
  // overload focus event
  virtual void focusOutEvent(QFocusEvent*);
  // overload Qt's event() to capture more keys
  bool event( QEvent* e );
    
  // overload context menu event
  virtual void contextMenuEvent(QContextMenuEvent*);
  // overload drag enter event
  virtual void dragEnterEvent(QDragEnterEvent*);
  // overload drag move event
  virtual void dragMoveEvent(QDragMoveEvent*);
  // overload drag leave event
  virtual void dragLeaveEvent(QDragLeaveEvent*);
  // overload drop event
  virtual void dropEvent(QDropEvent*);

  // the vtk render window
  vtkRenderWindow* mRenWin;
  bool UseTDx;

  // the paint engine
  QPaintEngine* mPaintEngine;
    
  // set up an X11 window based on a visual and colormap
  // that VTK chooses
  void x11_setup_window();

#if defined(QVTK_USE_CARBON)
  EventHandlerUPP DirtyRegionHandlerUPP;
  EventHandlerRef DirtyRegionHandler;
  static OSStatus DirtyRegionProcessor(EventHandlerCallRef er, EventRef event, void*);
#endif

protected:
    
  vtkImageData* mCachedImage;
  bool cachedImageCleanFlag;
  bool automaticImageCache;
  double maxImageCacheRenderRate;
  QVTKInteractorAdapter* mIrenAdapter;
  
private:
  //! unimplemented operator=
  QVTKWidget const& operator=(QVTKWidget const&);
  //! unimplemented copy
  QVTKWidget(const QVTKWidget&);

};

#endif
