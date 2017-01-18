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

#include "vtkGUISupportQtModule.h" // For export macro
#include "QVTKInteractor.h"
#include <QWidget>
#include <QTimer>

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
# elif defined(VTK_USE_COCOA)
#  error "VTK configured to use Cocoa, but Qt configured to use Carbon"
# endif
#endif


#include "QVTKWin32Header.h"

//! QVTKWidget displays a VTK window in a Qt window.
class VTKGUISUPPORTQT_EXPORT QVTKWidget : public QWidget
{
  Q_OBJECT

  Q_PROPERTY(bool automaticImageCacheEnabled
             READ isAutomaticImageCacheEnabled
             WRITE setAutomaticImageCacheEnabled)
  Q_PROPERTY(double maxRenderRateForImageCache
             READ maxRenderRateForImageCache
             WRITE setMaxRenderRateForImageCache)
  Q_PROPERTY(bool deferRenderInPaintEvent
             READ deferRenderInPaintEvent
             WRITE setDeferRenderInPaintEvent)

public:
  //! constructor
  QVTKWidget(QWidget* parent = NULL, Qt::WindowFlags f = 0);
  //! destructor
  ~QVTKWidget() VTK_OVERRIDE;

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
  // after every render with a DesiredUpdateRate that is less than
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
  void showEvent(QShowEvent*) VTK_OVERRIDE;

  QPaintEngine* paintEngine() const VTK_OVERRIDE;

  // Description:
  // Use a 3DConnexion device. Initial value is false.
  // If VTK is not build with the TDx option, this is no-op.
  // If VTK is build with the TDx option, and a device is not connected,
  // a warning is emitted.
  // It is must be called before the first Render to be effective, otherwise
  // it is ignored.
  void SetUseTDx(bool useTDx);
  bool GetUseTDx() const;

  // Description:
  // When set to true (default is false), paintEvent() will never directly trigger
  // a render on the vtkRenderWindow (via vtkRenderWindowInteractor::Render()).
  // Instead, it starts a timer that then triggers the render on idle. This, in
  // general is a good strategy for cases where Render may take a while with
  // applications wanting to report progress and consequently trigger paint
  // events on other widgets like progress bars, etc.
  // There is one caveat: when paintEvent() is called using a redirected paint device,
  // then this flag is ignored and the paintEvent() will trigger
  // vtkRenderWindowInteractor::Render(), if needed.
  void setDeferRenderInPaintEvent(bool val);
  bool deferRenderInPaintEvent() const;

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

protected Q_SLOTS:
  // Description:
  // Request to defer a render call i.e. start the mDeferedRenderTimer. When the
  // timer times out, it will call doDeferredRender() to do the actual
  // rendering.
  virtual void deferRender();

  // Description:
  // Called when the mDeferedRenderTimer times out to do the rendering.
  virtual void doDeferredRender();

protected:
  // overloaded resize handler
  void resizeEvent(QResizeEvent* event) VTK_OVERRIDE;
  // overloaded move handler
  void moveEvent(QMoveEvent* event) VTK_OVERRIDE;
  // overloaded paint handler
  void paintEvent(QPaintEvent* event) VTK_OVERRIDE;

  // overloaded mouse press handler
  void mousePressEvent(QMouseEvent* event) VTK_OVERRIDE;
  // overloaded mouse move handler
  void mouseMoveEvent(QMouseEvent* event) VTK_OVERRIDE;
  // overloaded mouse release handler
  void mouseReleaseEvent(QMouseEvent* event) VTK_OVERRIDE;
  // overloaded key press handler
  void keyPressEvent(QKeyEvent* event) VTK_OVERRIDE;
  // overloaded key release handler
  void keyReleaseEvent(QKeyEvent* event) VTK_OVERRIDE;
  // overloaded enter event
  void enterEvent(QEvent*) VTK_OVERRIDE;
  // overloaded leave event
  void leaveEvent(QEvent*) VTK_OVERRIDE;
#ifndef QT_NO_WHEELEVENT
  // overload wheel mouse event
  void wheelEvent(QWheelEvent*) VTK_OVERRIDE;
#endif
  // overload focus event
  void focusInEvent(QFocusEvent*) VTK_OVERRIDE;
  // overload focus event
  void focusOutEvent(QFocusEvent*) VTK_OVERRIDE;
  // overload Qt's event() to capture more keys
  bool event( QEvent* e ) VTK_OVERRIDE;

  // overload context menu event
  void contextMenuEvent(QContextMenuEvent*) VTK_OVERRIDE;
  // overload drag enter event
  void dragEnterEvent(QDragEnterEvent*) VTK_OVERRIDE;
  // overload drag move event
  void dragMoveEvent(QDragMoveEvent*) VTK_OVERRIDE;
  // overload drag leave event
  void dragLeaveEvent(QDragLeaveEvent*) VTK_OVERRIDE;
  // overload drop event
  void dropEvent(QDropEvent*) VTK_OVERRIDE;

  // method called in paintEvent() to render the image cache on to the device.
  // return false, if cache couldn;t be used for painting. In that case, the
  // paintEvent() method will continue with the default painting code.
  virtual bool paintCachedImage();

  // the vtk render window
  vtkRenderWindow* mRenWin;
  bool UseTDx;

  // the paint engine
  QPaintEngine* mPaintEngine;

  // set up an X11 window based on a visual and colormap
  // that VTK chooses
  void x11_setup_window();

#if defined(Q_OS_WIN)
  bool winEvent(MSG* msg, long* result);

#if QT_VERSION >= 0x050000
  bool nativeEvent(const QByteArray& eventType, void* message, long* result);
#endif

#endif

protected:
  vtkImageData* mCachedImage;
  bool cachedImageCleanFlag;
  bool automaticImageCache;
  double maxImageCacheRenderRate;
  QVTKInteractorAdapter* mIrenAdapter;
  bool mDeferRenderInPaintEvent;


private:
  //! unimplemented operator=
  QVTKWidget const& operator=(QVTKWidget const&);
  //! unimplemented copy
  QVTKWidget(const QVTKWidget&);

  unsigned long renderEventCallbackObserverId;

  // Description:
  // Callback called on every vtkCommand::RenderEvent fired by the
  // vtkRenderWindow.
  void renderEventCallback();
  QTimer mDeferedRenderTimer;
};

#endif
