/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

/*========================================================================
 For general information about using VTK and Qt, see:
 http://www.vtk.org/Wiki/VTK/Examples/Cxx#Qt
=========================================================================*/

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#pragma warning(disable:4512)
#endif

#include "QVTKWidget.h"

#include "QVTKPaintEngine.h"
#include "QVTKInteractorAdapter.h"
#include "QVTKInteractor.h"

#include "qevent.h"
#include "qapplication.h"
#include "qpainter.h"
#include "qsignalmapper.h"
#include "qtimer.h"
#include "vtkRenderingOpenGLConfigure.h"
#if defined(Q_WS_X11) // aka Qt4
# include "qx11info_x11.h"
#elif defined(Q_OS_LINUX) // aka Qt5
# include <QX11Info>
#endif

#if defined(Q_OS_WIN)
# include <windows.h>
# include <QSysInfo>
#endif

#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkRenderWindow.h"
#include "vtkCommand.h"
#include "vtkOStrStreamWrapper.h"
#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include "vtkConfigure.h"
#include "vtkUnsignedCharArray.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"

#if defined(VTK_USE_TDX) && (defined(Q_WS_X11) || defined(Q_OS_LINUX))
# include "vtkTDxUnixDevice.h"
#endif

/*! constructor */
QVTKWidget::QVTKWidget(QWidget* p, Qt::WindowFlags f)
  : QWidget(p, f | Qt::MSWindowsOwnDC), mRenWin(NULL),
    cachedImageCleanFlag(false),
    automaticImageCache(false), maxImageCacheRenderRate(1.0),
    mDeferRenderInPaintEvent(false),
    renderEventCallbackObserverId(0)
{
  this->UseTDx=false;
  // no background
  this->setAttribute(Qt::WA_NoBackground);
  // no double buffering
  this->setAttribute(Qt::WA_PaintOnScreen);

  // default to strong focus
  this->setFocusPolicy(Qt::StrongFocus);

  // default to enable mouse events when a mouse button isn't down
  // so we can send enter/leave events to VTK
  this->setMouseTracking(true);

  // set expanding to take up space for better default layouts
  this->setSizePolicy(
    QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding )
    );

  mPaintEngine = new QVTKPaintEngine;

  this->mCachedImage = vtkImageData::New();
  this->mCachedImage->SetOrigin(0,0,0);
  this->mCachedImage->SetSpacing(1,1,1);

  mIrenAdapter = new QVTKInteractorAdapter(this);

  this->mDeferedRenderTimer.setSingleShot(true);
  this->mDeferedRenderTimer.setInterval(0);
  this->connect(&this->mDeferedRenderTimer, SIGNAL(timeout()), SLOT(doDeferredRender()));
}

/*! destructor */

QVTKWidget::~QVTKWidget()
{
  // get rid of the VTK window
  this->SetRenderWindow(NULL);

  this->mCachedImage->Delete();

  delete mPaintEngine;
}

// ----------------------------------------------------------------------------
void QVTKWidget::SetUseTDx(bool useTDx)
{
  if(useTDx!=this->UseTDx)
  {
    this->UseTDx=useTDx;

    if(this->UseTDx)
    {
#if defined(VTK_USE_TDX) && (defined(Q_WS_X11) || defined(Q_OS_LINUX))
       QByteArray theSignal=
         QMetaObject::normalizedSignature("CreateDevice(vtkTDxDevice *)");
      if(QApplication::instance()->metaObject()->indexOfSignal(theSignal)!=-1)
      {
        QObject::connect(QApplication::instance(),
                         SIGNAL(CreateDevice(vtkTDxDevice *)),
                         this,
                         SLOT(setDevice(vtkTDxDevice *)));
      }
      else
      {
        vtkGenericWarningMacro("Missing signal CreateDevice on QApplication. 3DConnexion device will not work. Define it or derive your QApplication from QVTKApplication.");
      }
#endif
    }
  }
}

// ----------------------------------------------------------------------------
bool QVTKWidget::GetUseTDx() const
{
  return this->UseTDx;
}

/*! get the render window
 */
vtkRenderWindow* QVTKWidget::GetRenderWindow()
{
  if (!this->mRenWin)
  {
    // create a default vtk window
    vtkRenderWindow* win = vtkRenderWindow::New();
    this->SetRenderWindow(win);
    win->Delete();
  }

  return this->mRenWin;
}

/*! set the render window
  this will bind a VTK window with the Qt window
  it'll also replace an existing VTK window
*/
void QVTKWidget::SetRenderWindow(vtkRenderWindow* w)
{
  // do nothing if we don't have to
  if(w == this->mRenWin)
  {
    return;
  }

  // unregister previous window
  if(this->mRenWin)
  {
    if (this->renderEventCallbackObserverId)
    {
      this->mRenWin->RemoveObserver(this->renderEventCallbackObserverId);
      this->renderEventCallbackObserverId = 0;
    }
    //clean up window as one could remap it
    if(this->mRenWin->GetMapped())
    {
      this->mRenWin->Finalize();
    }
#if defined(Q_WS_X11) || defined(Q_OS_LINUX)
    this->mRenWin->SetDisplayId(NULL);
#endif
    this->mRenWin->SetWindowId(NULL);
    this->mRenWin->UnRegister(NULL);
  }

  // now set the window
  this->mRenWin = w;

  if(this->mRenWin)
  {
    // register new window
    this->mRenWin->Register(NULL);

    // if it is mapped somewhere else, unmap it
    if(this->mRenWin->GetMapped())
    {
      this->mRenWin->Finalize();
    }

#if defined(Q_WS_X11) || defined(Q_OS_LINUX)
    // give the qt display id to the vtk window
    this->mRenWin->SetDisplayId(QX11Info::display());
#endif

    // special x11 setup
    x11_setup_window();

    // give the qt window id to the vtk window
    this->mRenWin->SetWindowId( reinterpret_cast<void*>(this->winId()));

    // tell the vtk window what the size of this window is
    this->mRenWin->vtkRenderWindow::SetSize(this->width(), this->height());
    this->mRenWin->vtkRenderWindow::SetPosition(this->x(), this->y());

    // have VTK start this window and create the necessary graphics resources
    if(isVisible())
    {
      this->mRenWin->Start();
    }

    // if an interactor wasn't provided, we'll make one by default
    if(!this->mRenWin->GetInteractor())
    {
      // create a default interactor
      QVTKInteractor* iren = QVTKInteractor::New();
      iren->SetUseTDx(this->UseTDx);
      this->mRenWin->SetInteractor(iren);
      iren->Initialize();

      // now set the default style
      vtkInteractorStyle* s = vtkInteractorStyleTrackballCamera::New();
      iren->SetInteractorStyle(s);

      iren->Delete();
      s->Delete();
    }

    // tell the interactor the size of this window
    this->mRenWin->GetInteractor()->SetSize(this->width(), this->height());

    // Add an observer to monitor when the image changes.  Should work most
    // of the time.  The application will have to call
    // markCachedImageAsDirty for any other case.
    this->renderEventCallbackObserverId =
      this->mRenWin->AddObserver(vtkCommand::RenderEvent,
        this, &QVTKWidget::renderEventCallback);
  }
}



/*! get the Qt/VTK interactor
 */
QVTKInteractor* QVTKWidget::GetInteractor()
{
  return QVTKInteractor
    ::SafeDownCast(this->GetRenderWindow()->GetInteractor());
}

void QVTKWidget::markCachedImageAsDirty()
{
  if (this->cachedImageCleanFlag)
  {
    this->cachedImageCleanFlag = false;
    emit cachedImageDirty();
  }
}

void QVTKWidget::saveImageToCache()
{
  if (this->cachedImageCleanFlag)
  {
    return;
  }

  int w = this->width();
  int h = this->height();
  this->mCachedImage->SetExtent(0, w-1, 0, h-1, 0, 0);
  this->mCachedImage->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
  vtkUnsignedCharArray* array = vtkArrayDownCast<vtkUnsignedCharArray>(
    this->mCachedImage->GetPointData()->GetScalars());
  // We use back-buffer if
  this->mRenWin->GetPixelData(0, 0, this->width()-1, this->height()-1,
    this->mRenWin->GetDoubleBuffer()? 0 /*back*/ : 1 /*front*/, array);
  this->cachedImageCleanFlag = true;
  emit cachedImageClean();
}

void QVTKWidget::setAutomaticImageCacheEnabled(bool flag)
{
  this->automaticImageCache = flag;
  if (!flag)
  {
    this->mCachedImage->Initialize();
    this->mCachedImage->SetOrigin(0,0,0);
    this->mCachedImage->SetSpacing(1,1,1);
    this->markCachedImageAsDirty();
  }
}
bool QVTKWidget::isAutomaticImageCacheEnabled() const
{
  return this->automaticImageCache;
}

void QVTKWidget::setMaxRenderRateForImageCache(double rate)
{
  this->maxImageCacheRenderRate = rate;
}
double QVTKWidget::maxRenderRateForImageCache() const
{
  return this->maxImageCacheRenderRate;
}

void QVTKWidget::setDeferRenderInPaintEvent(bool val)
{
  this->mDeferRenderInPaintEvent = val;
}

bool QVTKWidget::deferRenderInPaintEvent() const
{
  return this->mDeferRenderInPaintEvent;
}

vtkImageData* QVTKWidget::cachedImage()
{
  // Make sure image is up to date.
  this->paintEvent(NULL);
  this->saveImageToCache();

  return this->mCachedImage;
}

/*! overloaded Qt's event handler to capture additional keys that Qt has
  default behavior for (for example the Tab and Shift-Tab key)
*/
bool QVTKWidget::event(QEvent* e)
{
  if(e->type() == QEvent::ParentAboutToChange)
  {
    this->markCachedImageAsDirty();
    if (this->mRenWin)
    {
      // Finalize the window to remove graphics resources associated with
      // this window
      if(this->mRenWin->GetMapped())
      {
        this->mRenWin->Finalize();
      }
    }
  }
  else if(e->type() == QEvent::ParentChange)
  {
    if(this->mRenWin)
    {
      x11_setup_window();
      // connect to new window
      this->mRenWin->SetWindowId( reinterpret_cast<void*>(this->winId()));

      // start up the window to create graphics resources for this window
      if(isVisible())
      {
        this->mRenWin->Start();
      }
    }
  }
  else if(e->type() == QEvent::TouchBegin ||
          e->type() == QEvent::TouchUpdate ||
          e->type() == QEvent::TouchEnd)
  {
    if(this->mRenWin)
    {
      mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
      if (e->isAccepted())
      {
        return true;
      }
    }
  }

  if(QObject::event(e))
  {
    return true;
  }

  if(e->type() == QEvent::KeyPress)
  {
    QKeyEvent* ke = static_cast<QKeyEvent*>(e);
    this->keyPressEvent(ke);
    return ke->isAccepted();
  }

  return QWidget::event(e);
}


/*! handle resize event
 */
void QVTKWidget::resizeEvent(QResizeEvent* e)
{
  QWidget::resizeEvent(e);

  if(!this->mRenWin)
  {
    return;
  }

  // Don't set size on subclass of vtkRenderWindow or it triggers recursion.
  // Getting this event in the first place means the window was already
  // resized and we're updating the sizes in VTK.
  this->mRenWin->vtkRenderWindow::SetSize(this->width(), this->height());

  // and update the interactor
  if(this->mRenWin->GetInteractor())
  {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
  }

  this->markCachedImageAsDirty();
}

void QVTKWidget::moveEvent(QMoveEvent* e)
{
  QWidget::moveEvent(e);

  if(!this->mRenWin)
  {
    return;
  }

  // Don't set size on subclass of vtkRenderWindow or it triggers recursion.
  // Getting this event in the first place means the window was already
  // resized and we're updating the sizes in VTK.
  this->mRenWin->vtkRenderWindow::SetPosition(this->x(), this->y());
}

/*! handle paint event
 */
void QVTKWidget::paintEvent(QPaintEvent* )
{
  vtkRenderWindowInteractor* iren = this->mRenWin ? this->mRenWin->GetInteractor() : NULL;
  if (!iren || !iren->GetEnabled())
  {
    return;
  }

  // In Qt 4.1+ let's support redirected painting
  // if redirected, let's grab the image from VTK, and paint it to the device
  QPaintDevice* device = QPainter::redirected(this);
  bool usingRedirectedDevice = (device != NULL && device != this);

  // if we have a saved image, use it
  if (this->paintCachedImage() == false)
  {
    // we don't defer render in redirected painting is active since the target
    // being painted to may not be around when the deferred render call happens.
    if (!usingRedirectedDevice && this->mDeferRenderInPaintEvent)
    {
      this->deferRender();
    }
    else
    {
      iren->Render();
    }
  }

  // Irrespective of whether cache was used on or, if using redirected painting
  // is being employed, we need to "paint" the image from the render window to
  // the redirected target.
  if (usingRedirectedDevice)
  {
    Q_ASSERT(device);

    int w = this->width();
    int h = this->height();
    QImage img(w, h, QImage::Format_RGB32);
    vtkUnsignedCharArray* pixels = vtkUnsignedCharArray::New();
    pixels->SetArray(img.bits(), w*h*4, 1);
    this->mRenWin->GetRGBACharPixelData(0, 0, w-1, h-1, 1, pixels);
    pixels->Delete();
    img = img.rgbSwapped();
    img = img.mirrored();

    QPainter painter(this);
    painter.drawImage(QPointF(0.0,0.0), img);
  }
}

/*! handle mouse press event
 */
void QVTKWidget::mousePressEvent(QMouseEvent* e)
{

  // Emit a mouse press event for anyone who might be interested
  emit mouseEvent(e);

  if(this->mRenWin)
  {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
  }

}

/*! handle mouse move event
 */
void QVTKWidget::mouseMoveEvent(QMouseEvent* e)
{
  if(this->mRenWin)
  {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());

    // Emit a mouse press event for anyone who might be interested
    emit mouseEvent(e);
  }
}


/*! handle enter event
 */
void QVTKWidget::enterEvent(QEvent* e)
{
  if(this->mRenWin)
  {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
  }
}

/*! handle leave event
 */
void QVTKWidget::leaveEvent(QEvent* e)
{
  if(this->mRenWin)
  {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
  }
}

/*! handle mouse release event
 */
void QVTKWidget::mouseReleaseEvent(QMouseEvent* e)
{
  if(this->mRenWin)
  {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());

    // Emit a mouse press event for anyone who might be interested
    emit mouseEvent(e);
  }
}

/*! handle key press event
 */
void QVTKWidget::keyPressEvent(QKeyEvent* e)
{
  if(this->mRenWin)
  {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
  }
}

/*! handle key release event
 */
void QVTKWidget::keyReleaseEvent(QKeyEvent* e)
{
  if(this->mRenWin)
  {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
  }
}

void QVTKWidget::wheelEvent(QWheelEvent* e)
{
  if(this->mRenWin)
  {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
  }
}

void QVTKWidget::focusInEvent(QFocusEvent* e)
{
  // These prevent updates when the window
  // gains or loses focus.  By default, Qt
  // does an update because the color group's
  // active status changes.  We don't even use
  // color groups so we do nothing here.

  // also pass to interactor
  mIrenAdapter->ProcessEvent(e, this->GetInteractor());
}

void QVTKWidget::focusOutEvent(QFocusEvent* e)
{
  // These prevent updates when the window
  // gains or loses focus.  By default, Qt
  // does an update because the color group's
  // active status changes.  We don't even use
  // color groups so we do nothing here.

  // also pass to interactor
  mIrenAdapter->ProcessEvent(e, this->GetInteractor());
}


void QVTKWidget::contextMenuEvent(QContextMenuEvent* e)
{
  if(this->mRenWin)
  {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
  }
}

void QVTKWidget::dragEnterEvent(QDragEnterEvent* e)
{
  if(this->mRenWin)
  {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
  }
}

void QVTKWidget::dragMoveEvent(QDragMoveEvent* e)
{
  if(this->mRenWin)
  {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
  }
}

void QVTKWidget::dragLeaveEvent(QDragLeaveEvent* e)
{
  if(this->mRenWin)
  {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
  }
}

void QVTKWidget::dropEvent(QDropEvent* e)
{
  if(this->mRenWin)
  {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
  }
}

void QVTKWidget::showEvent(QShowEvent* e)
{
  this->markCachedImageAsDirty();

  QWidget::showEvent(e);
}

QPaintEngine* QVTKWidget::paintEngine() const
{
  return mPaintEngine;
}


// X11 stuff near the bottom of the file
// to prevent namespace collisions with Qt headers

#if defined(Q_WS_X11) || defined (Q_OS_LINUX)
#if defined(VTK_USE_OPENGL_LIBRARY)
#include "vtkXOpenGLRenderWindow.h"
#endif
#endif

#ifdef VTK_USE_TDX
// Description:
// Receive notification of the creation of the TDxDevice
void QVTKWidget::setDevice(vtkTDxDevice *device)
{
#if defined(Q_WS_X11) || defined(Q_OS_LINUX)
  if(this->GetInteractor()->GetDevice()!=device)
  {
    this->GetInteractor()->SetDevice(device);
  }
#else
  (void)device; // to avoid warnings.
#endif
}
#endif

void QVTKWidget::x11_setup_window()
{
#if defined(Q_WS_X11)
  // NOTE: deliberately not executing this code for Qt5. It caused issues with
  // glewInit() when I did that. Just letting the Qt create the visual/colormap
  // seems to work better.

  // this whole function is to allow this window to have a
  // different colormap and visual than the rest of the Qt application
  // this is very important if Qt's default visual and colormap is
  // not enough to get a decent graphics window


  // save widget states
  bool tracking = this->hasMouseTracking();
  Qt::FocusPolicy focus_policy = focusPolicy();
  bool visible = isVisible();
  if(visible)
  {
    hide();
  }


  // get visual and colormap from VTK
  XVisualInfo* vi = 0;
  Colormap cmap = 0;
  Display* display = reinterpret_cast<Display*>(mRenWin->GetGenericDisplayId());

  // check ogl and mesa and get information we need to create a decent window
#if defined(VTK_USE_OPENGL_LIBRARY)
  vtkXOpenGLRenderWindow* ogl_win = vtkXOpenGLRenderWindow::SafeDownCast(mRenWin);
  if(ogl_win)
  {
    vi = ogl_win->GetDesiredVisualInfo();
    cmap = ogl_win->GetDesiredColormap();
  }
#endif

  // can't get visual, oh well.
  // continue with Qt's default visual as it usually works
  if(!vi)
  {
    if(visible)
    {
      show();
    }
    return;
  }

  // create the X window based on information VTK gave us
  XSetWindowAttributes attrib;
  attrib.colormap = cmap;
  attrib.border_pixel = 0;
  attrib.background_pixel = 0;

  Window p = RootWindow(display, DefaultScreen(display));
  if(parentWidget())
  {
    p = parentWidget()->winId();
  }

  XWindowAttributes a;
  XGetWindowAttributes(display, this->winId(), &a);

  Window win = XCreateWindow(display, p, a.x, a.y, a.width, a.height,
                             0, vi->depth, InputOutput, vi->visual,
                             CWBackPixel|CWBorderPixel|CWColormap, &attrib);

  // backup colormap stuff
  Window *cmw;
  Window *cmwret;
  int count;
  if ( XGetWMColormapWindows(display, topLevelWidget()->winId(), &cmwret, &count) )
  {
    cmw = new Window[count+1];
    memcpy( (char *)cmw, (char *)cmwret, sizeof(Window)*count );
    XFree( (char *)cmwret );
    int i;
    for ( i=0; i<count; i++ )
    {
      if ( cmw[i] == winId() )
      {
        cmw[i] = win;
        break;
      }
    }
    if ( i >= count )
    {
      cmw[count++] = win;
    }
  }
  else
  {
    count = 1;
    cmw = new Window[count];
    cmw[0] = win;
  }


  // tell Qt to initialize anything it needs to for this window
  create(win);

  // restore colormaps
  XSetWMColormapWindows( display, topLevelWidget()->winId(), cmw, count );

  delete [] cmw;
  XFree(vi);

  XFlush(display);

  // restore widget states
  this->setMouseTracking(tracking);
  this->setAttribute(Qt::WA_NoBackground);
  this->setAttribute(Qt::WA_PaintOnScreen);
  this->setFocusPolicy(focus_policy);
  if(visible)
  {
    show();
  }

#endif
}

#if defined(Q_OS_WIN)
bool QVTKWidget::winEvent(MSG* msg, long*)
{
  // Starting with Windows Vista, Microsoft introduced WDDM.
  // We need to call InvalidateRect() to work with WDDM correctly,
  // especially when AERO is off.
  if(msg->message == WM_PAINT &&
    QSysInfo::windowsVersion() >= QSysInfo::WV_VISTA)
  {
    InvalidateRect((HWND)this->winId(), NULL, FALSE);
  }
  return false;
}

#if QT_VERSION >= 0x050000
bool QVTKWidget::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
  if (eventType == "windows_generic_MSG")
  {
    winEvent((MSG*)message, result);
  }
  return false;
}
#endif
#endif

//-----------------------------------------------------------------------------
bool QVTKWidget::paintCachedImage()
{
  // if we have a saved image, use it
  if (this->cachedImageCleanFlag)
  {
    vtkUnsignedCharArray* array = vtkArrayDownCast<vtkUnsignedCharArray>(
      this->mCachedImage->GetPointData()->GetScalars());
    // put cached image into back buffer if we can
    this->mRenWin->SetPixelData(0, 0, this->width()-1, this->height()-1,
                                array, !this->mRenWin->GetDoubleBuffer());
    // swap buffers, if double buffering
    this->mRenWin->Frame();
    // or should we just put it on the front buffer?
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void QVTKWidget::renderEventCallback()
{
  if (this->mRenWin)
  {
    // prevent capturing the selection buffer as the cached image. to do this
    // we iterate through each renderer in the view and check if they have an
    // active selector object. if so we return without saving the image
    vtkRendererCollection *renderers = this->mRenWin->GetRenderers();
    if(renderers)
    {
      renderers->InitTraversal();

      while(vtkRenderer *renderer = renderers->GetNextItem())
      {
        if(renderer->GetSelector() != NULL)
        {
          return;
        }
      }
    }

    // Render happened. If we have requested a render to happen, it has happened,
    // so no need to request another render. Stop the timer.
    this->mDeferedRenderTimer.stop();

    this->markCachedImageAsDirty();
    if (this->isAutomaticImageCacheEnabled() &&
      (this->mRenWin->GetDesiredUpdateRate() < this->maxRenderRateForImageCache()))
    {
      this->saveImageToCache();
    }
  }
}

//-----------------------------------------------------------------------------
void QVTKWidget::deferRender()
{
  this->mDeferedRenderTimer.start();
}

//-----------------------------------------------------------------------------
void QVTKWidget::doDeferredRender()
{
  vtkRenderWindowInteractor* iren = this->mRenWin ? this->mRenWin->GetInteractor() : NULL;
  if (iren && iren->GetEnabled())
  {
    iren->Render();
  }
}
