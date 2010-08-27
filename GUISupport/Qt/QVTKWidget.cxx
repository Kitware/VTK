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
 http://www.trolltech.com/products/3rdparty/vtksupport.html
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
#if defined(Q_WS_X11)
#include "qx11info_x11.h"
#endif

#include "vtkstd/map"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkRenderWindow.h"
#if defined(QVTK_USE_CARBON)
#  include "vtkCarbonRenderWindow.h"
#endif
#include "vtkCommand.h"
#include "vtkOStrStreamWrapper.h"
#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include "vtkConfigure.h"
#include "vtkUnsignedCharArray.h"
#include "vtkImageData.h"
#include "vtkPointData.h"

#if defined(VTK_USE_TDX) && defined(Q_WS_X11)
# include "vtkTDxUnixDevice.h"
#endif

// function to dirty cache when a render occurs.
static void dirty_cache(vtkObject *, unsigned long, void *, void *);

/*! constructor */
QVTKWidget::QVTKWidget(QWidget* p, Qt::WFlags f)
  : QWidget(p, f | Qt::MSWindowsOwnDC), mRenWin(NULL),
    cachedImageCleanFlag(false),
    automaticImageCache(false), maxImageCacheRenderRate(1.0)

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
  this->mCachedImage->SetScalarTypeToUnsignedChar();
  this->mCachedImage->SetOrigin(0,0,0);
  this->mCachedImage->SetSpacing(1,1,1);

#if defined(QVTK_USE_CARBON)
  this->DirtyRegionHandler = 0;
  this->DirtyRegionHandlerUPP = 0;
#endif

  mIrenAdapter = new QVTKInteractorAdapter(this);

}

/*! destructor */

QVTKWidget::~QVTKWidget()
{
  // get rid of the VTK window
  this->SetRenderWindow(NULL);
  
  this->mCachedImage->Delete();

  if(mPaintEngine)
    {
    delete mPaintEngine;
    }
}

// ----------------------------------------------------------------------------
void QVTKWidget::SetUseTDx(bool useTDx)
{
  if(useTDx!=this->UseTDx)
    {
    this->UseTDx=useTDx;
    if(this->UseTDx)
      {
#if defined(VTK_USE_TDX) && defined(Q_WS_X11)
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
    //clean up window as one could remap it
    if(this->mRenWin->GetMapped())
      {
      this->mRenWin->Finalize();
      }
#ifdef Q_WS_X11
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

#ifdef Q_WS_X11
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
    vtkCallbackCommand *cbc = vtkCallbackCommand::New();
    cbc->SetClientData(this);
    cbc->SetCallback(dirty_cache);
    this->mRenWin->AddObserver(vtkCommand::EndEvent, cbc);
    cbc->Delete();
    }

#if defined(QVTK_USE_CARBON)
  if(mRenWin && !this->DirtyRegionHandlerUPP)
    {
    this->DirtyRegionHandlerUPP = NewEventHandlerUPP(QVTKWidget::DirtyRegionProcessor);
    static EventTypeSpec events[] = { {'cute', 20}, {'Cute', 20} };  
    // kEventClassQt, kEventQtRequestWindowChange from qt_mac_p.h
    // Suggested by Sam Magnuson at Trolltech as best portabile hack 
    // around Apple's missing functionality in HI Toolbox.
    InstallEventHandler(GetApplicationEventTarget(), this->DirtyRegionHandlerUPP, 
                        GetEventTypeCount(events), events, 
                        reinterpret_cast<void*>(this), &this->DirtyRegionHandler);
    }
  else if(!mRenWin && this->DirtyRegionHandlerUPP)
    {
    RemoveEventHandler(this->DirtyRegionHandler);
    DisposeEventHandlerUPP(this->DirtyRegionHandlerUPP);
    this->DirtyRegionHandler = 0;
    this->DirtyRegionHandlerUPP = 0;
    }
#endif
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
  this->mCachedImage->SetWholeExtent(0, w-1, 0, h-1, 0, 0);
  this->mCachedImage->SetNumberOfScalarComponents(3);
  this->mCachedImage->SetExtent(this->mCachedImage->GetWholeExtent());
  this->mCachedImage->AllocateScalars();
  vtkUnsignedCharArray* array = vtkUnsignedCharArray::SafeDownCast(
    this->mCachedImage->GetPointData()->GetScalars());
  this->mRenWin->GetPixelData(0, 0, this->width()-1, this->height()-1, 1,
                              array);
  this->cachedImageCleanFlag = true;
  emit cachedImageClean();
}

void QVTKWidget::setAutomaticImageCacheEnabled(bool flag)
{
  this->automaticImageCache = flag;
  if (!flag)
    {
    this->mCachedImage->Initialize();
    this->mCachedImage->SetScalarTypeToUnsignedChar();
    this->mCachedImage->SetOrigin(0,0,0);
    this->mCachedImage->SetSpacing(1,1,1);
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
  
  if(QObject::event(e))
    {
    return TRUE;
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
  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    {
    iren = this->mRenWin->GetInteractor();
    }

  if(!iren || !iren->GetEnabled())
    {
    return;
    }

  
  // if we have a saved image, use it
  if (this->cachedImageCleanFlag)
    {
    vtkUnsignedCharArray* array = vtkUnsignedCharArray::SafeDownCast(
      this->mCachedImage->GetPointData()->GetScalars());
    // put cached image into back buffer if we can
    this->mRenWin->SetPixelData(0, 0, this->width()-1, this->height()-1,
                                array, !this->mRenWin->GetDoubleBuffer());
    // swap buffers, if double buffering
    this->mRenWin->Frame();
    // or should we just put it on the front buffer?
    return;
    }

  iren->Render();
  
  // In Qt 4.1+ let's support redirected painting
  // if redirected, let's grab the image from VTK, and paint it to the device
  QPaintDevice* device = QPainter::redirected(this);
  if(device != NULL && device != this)
    {
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
    return;
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

#if defined Q_WS_X11
#if defined(VTK_USE_OPENGL_LIBRARY)
#include "vtkXOpenGLRenderWindow.h"
#endif
#ifdef VTK_USE_MANGLED_MESA
#include "vtkXMesaRenderWindow.h"
#endif
#endif

#ifdef VTK_USE_TDX
// Description:
// Receive notification of the creation of the TDxDevice
void QVTKWidget::setDevice(vtkTDxDevice *device)
{
#ifdef Q_WS_X11
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
#if defined Q_WS_X11

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
#ifdef VTK_USE_MANGLED_MESA
  if(!vi)
    {
    vtkXMesaRenderWindow* mgl_win = vtkXMesaRenderWindow::SafeDownCast(mRenWin);
    if(mgl_win)
      {
      vi = mgl_win->GetDesiredVisualInfo();
      cmap = mgl_win->GetDesiredColormap();
      }
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

#if defined (QVTK_USE_CARBON)
OSStatus QVTKWidget::DirtyRegionProcessor(EventHandlerCallRef, EventRef event, void* wid)
{
  QVTKWidget* widget = reinterpret_cast<QVTKWidget*>(wid);
  UInt32 event_kind = GetEventKind(event);
  UInt32 event_class = GetEventClass(event);
  if((event_class == 'cute' || event_class == 'Cute') && event_kind == 20)
    {
    static_cast<vtkCarbonRenderWindow*>(widget->GetRenderWindow())->UpdateGLRegion();
    }
  return eventNotHandledErr;
}

#endif


static void dirty_cache(vtkObject *caller, unsigned long,
                        void *clientdata, void *)
{
  QVTKWidget *widget = reinterpret_cast<QVTKWidget *>(clientdata);
  widget->markCachedImageAsDirty();

  vtkRenderWindow *renwin = vtkRenderWindow::SafeDownCast(caller);
  if (renwin)
    {
    if (   widget->isAutomaticImageCacheEnabled()
           && (  renwin->GetDesiredUpdateRate()
                 < widget->maxRenderRateForImageCache() ) )
      {
      widget->saveImageToCache();
      }
    }
}
