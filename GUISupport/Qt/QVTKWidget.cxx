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

#if defined(VTK_USE_TDX) && defined(Q_WS_WIN)
# include "vtkTDxWinDevice.h"
#endif

#if defined(VTK_USE_TDX) && defined(Q_WS_MAC)
# include "vtkTDxMacDevice.h"
#endif

# include "QVTKPaintEngine.h"

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

// function to get VTK keysyms from ascii characters
static const char* ascii_to_key_sym(int);
// function to get VTK keysyms from Qt keys
static const char* qt_key_to_key_sym(Qt::Key);

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

  // give the size to the interactor and vtk window
  this->mRenWin->vtkRenderWindow::SetSize(this->width(), this->height());
  if(this->mRenWin->GetInteractor())
    {
    this->mRenWin->GetInteractor()->SetSize(this->width(), this->height());
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
    
  // give the size to the interactor and vtk window
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

  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    {
    iren = this->mRenWin->GetInteractor();
    }
  
  if(!iren || !iren->GetEnabled())
    {
    return;
    }

  // give interactor the event information
  iren->SetEventInformationFlipY(e->x(), e->y(), 
                              (e->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0, 
                              (e->modifiers() & Qt::ShiftModifier ) > 0 ? 1 : 0,
                              0,
                              e->type() == QEvent::MouseButtonDblClick ? 1 : 0);

  // invoke appropriate vtk event
  switch(e->button())
    {
    case Qt::LeftButton:
      iren->InvokeEvent(vtkCommand::LeftButtonPressEvent, e);
      break;

    case Qt::MidButton:
      iren->InvokeEvent(vtkCommand::MiddleButtonPressEvent, e);
      break;

    case Qt::RightButton:
      iren->InvokeEvent(vtkCommand::RightButtonPressEvent, e);
      break;

    default:
      break;
    }
}

/*! handle mouse move event
 */
void QVTKWidget::mouseMoveEvent(QMouseEvent* e)
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
  
  // give interactor the event information
  iren->SetEventInformationFlipY(e->x(), e->y(), 
                             (e->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0, 
                             (e->modifiers() & Qt::ShiftModifier ) > 0 ? 1 : 0);
  
  // invoke vtk event
  iren->InvokeEvent(vtkCommand::MouseMoveEvent, e);
}


/*! handle enter event
 */
void QVTKWidget::enterEvent(QEvent* e)
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

  iren->InvokeEvent(vtkCommand::EnterEvent, e);
}

/*! handle leave event
 */
void QVTKWidget::leaveEvent(QEvent* e)
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
  
  iren->InvokeEvent(vtkCommand::LeaveEvent, e);
}

/*! handle mouse release event
 */
void QVTKWidget::mouseReleaseEvent(QMouseEvent* e)
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
  
  // give vtk event information
  iren->SetEventInformationFlipY(e->x(), e->y(), 
                             (e->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0, 
                             (e->modifiers() & Qt::ShiftModifier ) > 0 ? 1 : 0);
  
  // invoke appropriate vtk event
  switch(e->button())
    {
    case Qt::LeftButton:
      iren->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, e);
      break;

    case Qt::MidButton:
      iren->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, e);
      break;

    case Qt::RightButton:
      iren->InvokeEvent(vtkCommand::RightButtonReleaseEvent, e);
      break;

    default:
      break;
    }
}

/*! handle key press event
 */
void QVTKWidget::keyPressEvent(QKeyEvent* e)
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
  
  // get key and keysym information
  int ascii_key = e->text().length() ? e->text().unicode()->toLatin1() : 0;
  const char* keysym = ascii_to_key_sym(ascii_key);
  if(!keysym)
    {
    // get virtual keys
    keysym = qt_key_to_key_sym(static_cast<Qt::Key>(e->key()));
    }

  if(!keysym)
    {
    keysym = "None";
    }
  
  // give interactor event information
  iren->SetKeyEventInformation(
    (e->modifiers() & Qt::ControlModifier),
    (e->modifiers() & Qt::ShiftModifier),
    ascii_key, e->count(), keysym);

  // invoke vtk event
  iren->InvokeEvent(vtkCommand::KeyPressEvent, e);
  
  // invoke char event only for ascii characters
  if(ascii_key)
    {
    iren->InvokeEvent(vtkCommand::CharEvent, e);
    }
}

/*! handle key release event
 */
void QVTKWidget::keyReleaseEvent(QKeyEvent* e)
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
  
  // get key and keysym info
  int ascii_key = e->text().length() ? e->text().unicode()->toLatin1() : 0;
  const char* keysym = ascii_to_key_sym(ascii_key);
  if(!keysym)
    {
    // get virtual keys
    keysym = qt_key_to_key_sym((Qt::Key)e->key());
    }

  if(!keysym)
    {
    keysym = "None";
    }

  // give event information to interactor
  iren->SetKeyEventInformation(
    (e->modifiers() & Qt::ControlModifier),
    (e->modifiers() & Qt::ShiftModifier),
    ascii_key, e->count(), keysym);

  // invoke vtk event
  iren->InvokeEvent(vtkCommand::KeyReleaseEvent, e);
}

#ifndef QT_NO_WHEELEVENT
void QVTKWidget::wheelEvent(QWheelEvent* e)
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

// VTK supports wheel mouse events only in version 4.5 or greater
  // give event information to interactor
  iren->SetEventInformationFlipY(e->x(), e->y(), 
                             (e->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0, 
                             (e->modifiers() & Qt::ShiftModifier ) > 0 ? 1 : 0);
  
  // invoke vtk event
  // if delta is positive, it is a forward wheel event
  if(e->delta() > 0)
    {
    iren->InvokeEvent(vtkCommand::MouseWheelForwardEvent, e);
    }
  else
    {
    iren->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, e);
    }
}
#endif

void QVTKWidget::focusInEvent(QFocusEvent*)
{
  // These prevent updates when the window 
  // gains or loses focus.  By default, Qt
  // does an update because the color group's 
  // active status changes.  We don't even use
  // color groups so we do nothing here.
  
  // For 3Dconnexion devices:
  QVTKInteractor* iren = this->GetInteractor();
  // Note that this class can have interactor of type
  // other than QVTKInteractor
  if (iren)
    {
    iren->StartListening();
    }
}

void QVTKWidget::focusOutEvent(QFocusEvent*)
{
  // These prevent updates when the window 
  // gains or loses focus.  By default, Qt
  // does an update because the color group's 
  // active status changes.  We don't even use
  // color groups so we do nothing here.
  
  // For 3DConnexion devices:
  QVTKInteractor* iren = this->GetInteractor();
  // Note that this class can have interactor of type
  // other than QVTKInteractor
  if (iren)
    {
    iren->StopListening();
    }
}


void QVTKWidget::contextMenuEvent(QContextMenuEvent* e)
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
  
  // give interactor the event information
  iren->SetEventInformationFlipY(e->x(), e->y(), 
                             (e->modifiers() & Qt::ControlModifier) > 0 ? 1 : 0, 
                             (e->modifiers() & Qt::ShiftModifier ) > 0 ? 1 : 0);

  // invoke event and pass qt event for additional data as well
  iren->InvokeEvent(QVTKWidget::ContextMenuEvent, e);
  
}

void QVTKWidget::dragEnterEvent(QDragEnterEvent* e)
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
  
  // invoke event and pass qt event for additional data as well
  iren->InvokeEvent(QVTKWidget::DragEnterEvent, e);
}

void QVTKWidget::dragMoveEvent(QDragMoveEvent* e)
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
  
  // give interactor the event information
  iren->SetEventInformationFlipY(e->pos().x(), e->pos().y());

  // invoke event and pass qt event for additional data as well
  iren->InvokeEvent(QVTKWidget::DragMoveEvent, e);
}

void QVTKWidget::dragLeaveEvent(QDragLeaveEvent* e)
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
  
  // invoke event and pass qt event for additional data as well
  iren->InvokeEvent(QVTKWidget::DragLeaveEvent, e);
}

void QVTKWidget::dropEvent(QDropEvent* e)
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
  
  // give interactor the event information
  iren->SetEventInformationFlipY(e->pos().x(), e->pos().y());

  // invoke event and pass qt event for additional data as well
  iren->InvokeEvent(QVTKWidget::DropEvent, e);
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

class QVTKInteractorInternal : public QObject
{
public:
  QVTKInteractorInternal(QObject* p)
    : QObject(p)
    {
      this->SignalMapper = new QSignalMapper(this);
    }
  ~QVTKInteractorInternal()
    {
    }
  QSignalMapper* SignalMapper;
  typedef vtkstd::map<int, QTimer*> TimerMap;
  TimerMap Timers;
};


/*! allocation method for Qt/VTK interactor
 */
vtkStandardNewMacro(QVTKInteractor);

/*! constructor for Qt/VTK interactor
 */
QVTKInteractor::QVTKInteractor()
{
  this->Internal = new QVTKInteractorInternal(this);
  QObject::connect(this->Internal->SignalMapper, SIGNAL(mapped(int)), this, SLOT(TimerEvent(int)) );

#if defined(VTK_USE_TDX) && defined(Q_WS_WIN)
  this->Device=vtkTDxWinDevice::New();
#endif
#if defined(VTK_USE_TDX) && defined(Q_WS_MAC)
  this->Device=vtkTDxMacDevice::New();
#endif
}
  
void QVTKInteractor::Initialize()
{
#if defined(VTK_USE_TDX) && defined(Q_WS_WIN)
  if(this->UseTDx)
    {
    // this is QWidget::winId();
    HWND hWnd=static_cast<HWND>(this->GetRenderWindow()->GetGenericWindowId());
    if(!this->Device->GetInitialized())
      {
      this->Device->SetInteractor(this);
      this->Device->SetWindowHandle(hWnd);
      this->Device->Initialize();
      }
    }
#endif
#if defined(VTK_USE_TDX) && defined(Q_WS_MAC)
  if(this->UseTDx)
    {
    if(!this->Device->GetInitialized())
      {
      this->Device->SetInteractor(this);
      // Do not initialize the device here.
      }
    }
#endif
  this->Initialized = 1;
  this->Enable();
}


/*! start method for interactor
 */
void QVTKInteractor::Start()
{
  vtkErrorMacro(<<"QVTKInteractor cannot control the event loop.");
}

/*! terminate the application
 */
void QVTKInteractor::TerminateApp()
{
  // we are in a GUI so let's terminate the GUI the normal way
  //qApp->exit();
}

// ----------------------------------------------------------------------------
void QVTKInteractor::StartListening()
{
#if defined(VTK_USE_TDX) && defined(Q_WS_WIN)
  if(this->Device->GetInitialized() && !this->Device->GetIsListening())
    {
    this->Device->StartListening();
    }
#endif
#if defined(VTK_USE_TDX) && defined(Q_WS_MAC)
  if(this->UseTDx && !this->Device->GetInitialized())
    {
    this->Device->Initialize();
    }
#endif
}

// ----------------------------------------------------------------------------
void QVTKInteractor::StopListening()
{
#if defined(VTK_USE_TDX) && defined(Q_WS_WIN)
  if(this->Device->GetInitialized() && this->Device->GetIsListening())
    {
    this->Device->StopListening();
    }
#endif
#if defined(VTK_USE_TDX) && defined(Q_WS_MAC)
  if(this->UseTDx && this->Device->GetInitialized())
    {
    this->Device->Close();
    }
#endif
}


/*! handle timer event
 */
void QVTKInteractor::TimerEvent(int timerId)
{
  if ( !this->GetEnabled() ) 
    {
    return;
    }
  this->InvokeEvent(vtkCommand::TimerEvent, (void*)&timerId);

  if(this->IsOneShotTimer(timerId))
    {
    this->DestroyTimer(timerId);  // 'cause our Qt timers are always repeating
    }
}

/*! constructor
 */
QVTKInteractor::~QVTKInteractor()
{
#if defined(VTK_USE_TDX) && defined(Q_WS_WIN)
  this->Device->Delete();
#endif
#if defined(VTK_USE_TDX) && defined(Q_WS_MAC)
  this->Device->Delete();
#endif
}

/*! create Qt timer with an interval of 10 msec.
 */
int QVTKInteractor::InternalCreateTimer(int timerId, int vtkNotUsed(timerType), unsigned long duration)
{
  QTimer* timer = new QTimer(this);
  timer->start(duration);
  this->Internal->SignalMapper->setMapping(timer, timerId);
  QObject::connect(timer, SIGNAL(timeout()), this->Internal->SignalMapper, SLOT(map()));
  int platformTimerId = timer->timerId();
  this->Internal->Timers.insert(QVTKInteractorInternal::TimerMap::value_type(platformTimerId, timer));
  return platformTimerId;
}

/*! destroy timer
 */
int QVTKInteractor::InternalDestroyTimer(int platformTimerId)
{
  QVTKInteractorInternal::TimerMap::iterator iter = this->Internal->Timers.find(platformTimerId);
  if(iter != this->Internal->Timers.end())
    {
    iter->second->stop();
    iter->second->deleteLater();
    this->Internal->Timers.erase(iter);
    return 1;
    }
  return 0;
}


// ***** keysym stuff below  *****

static const char *AsciiToKeySymTable[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, "Tab", 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  "space", "exclam", "quotedbl", "numbersign",
  "dollar", "percent", "ampersand", "quoteright",
  "parenleft", "parenright", "asterisk", "plus",
  "comma", "minus", "period", "slash",
  "0", "1", "2", "3", "4", "5", "6", "7",
  "8", "9", "colon", "semicolon", "less", "equal", "greater", "question",
  "at", "A", "B", "C", "D", "E", "F", "G",
  "H", "I", "J", "K", "L", "M", "N", "O",
  "P", "Q", "R", "S", "T", "U", "V", "W",
  "X", "Y", "Z", "bracketleft",
  "backslash", "bracketright", "asciicircum", "underscore",
  "quoteleft", "a", "b", "c", "d", "e", "f", "g",
  "h", "i", "j", "k", "l", "m", "n", "o",
  "p", "q", "r", "s", "t", "u", "v", "w",
  "x", "y", "z", "braceleft", "bar", "braceright", "asciitilde", "Delete",
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

const char* ascii_to_key_sym(int i)
{
  if(i >= 0)
    {
    return AsciiToKeySymTable[i];
    }
  return 0;
}

#define QVTK_HANDLE(x,y) \
  case x : \
    ret = y; \
    break;

const char* qt_key_to_key_sym(Qt::Key i)
{
  const char* ret = 0;
  switch(i)
    {
    // Cancel
    QVTK_HANDLE(Qt::Key_Backspace, "BackSpace")
      QVTK_HANDLE(Qt::Key_Tab, "Tab")
      QVTK_HANDLE(Qt::Key_Backtab, "Tab")
      //QVTK_HANDLE(Qt::Key_Clear, "Clear")
      QVTK_HANDLE(Qt::Key_Return, "Return")
      QVTK_HANDLE(Qt::Key_Enter, "Return")
      QVTK_HANDLE(Qt::Key_Shift, "Shift_L")
      QVTK_HANDLE(Qt::Key_Control, "Control_L")
      QVTK_HANDLE(Qt::Key_Alt, "Alt_L")
      QVTK_HANDLE(Qt::Key_Pause, "Pause")
      QVTK_HANDLE(Qt::Key_CapsLock, "Caps_Lock")
      QVTK_HANDLE(Qt::Key_Escape, "Escape")
      QVTK_HANDLE(Qt::Key_Space, "space")
      //QVTK_HANDLE(Qt::Key_Prior, "Prior")
      //QVTK_HANDLE(Qt::Key_Next, "Next")
      QVTK_HANDLE(Qt::Key_End, "End")
      QVTK_HANDLE(Qt::Key_Home, "Home")
      QVTK_HANDLE(Qt::Key_Left, "Left")
      QVTK_HANDLE(Qt::Key_Up, "Up")
      QVTK_HANDLE(Qt::Key_Right, "Right")
      QVTK_HANDLE(Qt::Key_Down, "Down")

      // Select
      // Execute
      QVTK_HANDLE(Qt::Key_SysReq, "Snapshot")
      QVTK_HANDLE(Qt::Key_Insert, "Insert")
      QVTK_HANDLE(Qt::Key_Delete, "Delete")
      QVTK_HANDLE(Qt::Key_Help, "Help")
      QVTK_HANDLE(Qt::Key_0, "0")
      QVTK_HANDLE(Qt::Key_1, "1")
      QVTK_HANDLE(Qt::Key_2, "2")
      QVTK_HANDLE(Qt::Key_3, "3")
      QVTK_HANDLE(Qt::Key_4, "4")
      QVTK_HANDLE(Qt::Key_5, "5")
      QVTK_HANDLE(Qt::Key_6, "6")
      QVTK_HANDLE(Qt::Key_7, "7")
      QVTK_HANDLE(Qt::Key_8, "8")
      QVTK_HANDLE(Qt::Key_9, "9")
      QVTK_HANDLE(Qt::Key_A, "a")
      QVTK_HANDLE(Qt::Key_B, "b")
      QVTK_HANDLE(Qt::Key_C, "c")
      QVTK_HANDLE(Qt::Key_D, "d")
      QVTK_HANDLE(Qt::Key_E, "e")
      QVTK_HANDLE(Qt::Key_F, "f")
      QVTK_HANDLE(Qt::Key_G, "g")
      QVTK_HANDLE(Qt::Key_H, "h")
      QVTK_HANDLE(Qt::Key_I, "i")
      QVTK_HANDLE(Qt::Key_J, "h")
      QVTK_HANDLE(Qt::Key_K, "k")
      QVTK_HANDLE(Qt::Key_L, "l")
      QVTK_HANDLE(Qt::Key_M, "m")
      QVTK_HANDLE(Qt::Key_N, "n")
      QVTK_HANDLE(Qt::Key_O, "o")
      QVTK_HANDLE(Qt::Key_P, "p")
      QVTK_HANDLE(Qt::Key_Q, "q")
      QVTK_HANDLE(Qt::Key_R, "r")
      QVTK_HANDLE(Qt::Key_S, "s")
      QVTK_HANDLE(Qt::Key_T, "t")
      QVTK_HANDLE(Qt::Key_U, "u")
      QVTK_HANDLE(Qt::Key_V, "v")
      QVTK_HANDLE(Qt::Key_W, "w")
      QVTK_HANDLE(Qt::Key_X, "x")
      QVTK_HANDLE(Qt::Key_Y, "y")
      QVTK_HANDLE(Qt::Key_Z, "z")
      // KP_0 - KP_9
      QVTK_HANDLE(Qt::Key_Asterisk, "asterisk")
      QVTK_HANDLE(Qt::Key_Plus, "plus")
      // bar
      QVTK_HANDLE(Qt::Key_Minus, "minus")
      QVTK_HANDLE(Qt::Key_Period, "period")
      QVTK_HANDLE(Qt::Key_Slash, "slash")
      QVTK_HANDLE(Qt::Key_F1, "F1")
      QVTK_HANDLE(Qt::Key_F2, "F2")
      QVTK_HANDLE(Qt::Key_F3, "F3")
      QVTK_HANDLE(Qt::Key_F4, "F4")
      QVTK_HANDLE(Qt::Key_F5, "F5")
      QVTK_HANDLE(Qt::Key_F6, "F6")
      QVTK_HANDLE(Qt::Key_F7, "F7")
      QVTK_HANDLE(Qt::Key_F8, "F8")
      QVTK_HANDLE(Qt::Key_F9, "F9")
      QVTK_HANDLE(Qt::Key_F10, "F10")
      QVTK_HANDLE(Qt::Key_F11, "F11")
      QVTK_HANDLE(Qt::Key_F12, "F12")
      QVTK_HANDLE(Qt::Key_F13, "F13")
      QVTK_HANDLE(Qt::Key_F14, "F14")
      QVTK_HANDLE(Qt::Key_F15, "F15")
      QVTK_HANDLE(Qt::Key_F16, "F16")
      QVTK_HANDLE(Qt::Key_F17, "F17")
      QVTK_HANDLE(Qt::Key_F18, "F18")
      QVTK_HANDLE(Qt::Key_F19, "F19")
      QVTK_HANDLE(Qt::Key_F20, "F20")
      QVTK_HANDLE(Qt::Key_F21, "F21")
      QVTK_HANDLE(Qt::Key_F22, "F22")
      QVTK_HANDLE(Qt::Key_F23, "F23")
      QVTK_HANDLE(Qt::Key_F24, "F24")
      QVTK_HANDLE(Qt::Key_NumLock, "Num_Lock")
      QVTK_HANDLE(Qt::Key_ScrollLock, "Scroll_Lock")

      default:
    break;
    }
  return ret;
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

