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

/*========================================================================
 !!! WARNING for those who want to contribute code to this file.
 !!! If you use a commercial edition of Qt, you can modify this code.
 !!! If you use an open source version of Qt, you are free to modify
 !!! and use this code within the guidelines of the GPL license.
 !!! Unfortunately, you cannot contribute the changes back into this
 !!! file.  Doing so creates a conflict between the GPL and BSD-like VTK
 !!! license.
=========================================================================*/

#include "QVTKWidget.h"

#include "qevent.h"
#include "qapplication.h"
#include "qpainter.h"
#if QT_VERSION >= 0x040000 && defined(Q_WS_X11)
#include "qx11info_x11.h"
#endif

#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkRenderWindow.h"
#include "vtkCommand.h"
#include "vtkOStrStreamWrapper.h"
#include "vtkObjectFactory.h"
#include "vtkCallbackCommand.h"
#include "vtkConfigure.h"
#include "vtkToolkits.h"


// VTK 4.5 added some major functionality, so we'll make a short define to use
#if (VTK_MAJOR_VERSION > 4) || (VTK_MAJOR_VERSION == 4 && VTK_MINOR_VERSION >=5)
#define QVTK_HAVE_VTK_4_5
#endif


// function to get VTK keysyms from ascii characters
static const char* ascii_to_key_sym(int);
// function to get VTK keysyms from Qt keys
static const char* qt_key_to_key_sym(Qt::Key);

// function to dirty cache when a render occurs.
static void dirty_cache(vtkObject *, unsigned long, void *, void *);



#if QT_VERSION < 0x040000
/*! constructor */
QVTKWidget::QVTKWidget(QWidget* parent, const char* name, Qt::WFlags f)
#if QT_VERSION < 0x030000
    : QWidget(parent, name, f | 0x10000000)  // WWinOwnDC
#else
    : QWidget(parent, name, f | Qt::WWinOwnDC )
#endif
     , mRenWin(NULL), cachedImageCleanFlag(false),
     automaticImageCache(false), maxImageCacheRenderRate(1.0)
{
  // no background
  setBackgroundMode( Qt::NoBackground );

   // default to strong focus
  this->setFocusPolicy(QWidget::StrongFocus);

  // default to enable mouse events when a mouse button isn't down
  // so we can send enter/leave events to VTK
  this->setMouseTracking(true);       
  
  // set expanding to take up space for better default layouts
  this->setSizePolicy( 
       QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding )
       );
}
#endif


#if QT_VERSION >= 0x040000
/*! constructor */
QVTKWidget::QVTKWidget(QWidget* parent, Qt::WFlags f)
    : QWidget(parent, f | Qt::WWinOwnDC), mRenWin(NULL),
          cachedImageCleanFlag(false),
          automaticImageCache(false), maxImageCacheRenderRate(1.0)

{
  // no background
  this->setAttribute(Qt::WA_PaintOnScreen);
  this->setAttribute(Qt::WA_NoSystemBackground);

   // default to strong focus
  this->setFocusPolicy(Qt::StrongFocus);

  // default to enable mouse events when a mouse button isn't down
  // so we can send enter/leave events to VTK
  this->setMouseTracking(true);       
  
  // set expanding to take up space for better default layouts
  this->setSizePolicy( 
       QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding )
       );
}
#endif


/*! destructor */

QVTKWidget::~QVTKWidget()
{
  // get rid of the VTK window
  this->SetRenderWindow(NULL);
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
void QVTKWidget::SetRenderWindow(vtkRenderWindow* window)
{
  // do nothing if we don't have to
  if(window == this->mRenWin)
    return;

  // unregister previous window
  if(this->mRenWin)
  {
#if defined(QVTK_HAVE_VTK_4_5)
    //clean up window as one could remap it
    if(this->mRenWin->GetMapped())
      this->mRenWin->Finalize();
#ifdef Q_WS_X11
    this->mRenWin->SetDisplayId(NULL);
#endif
#endif
    this->mRenWin->UnRegister(NULL);
  }

  // now set the window
  this->mRenWin = window;

  if(this->mRenWin)
  {
    // register new window
    this->mRenWin->Register(NULL);

#ifdef Q_WS_X11
    // give the qt display id to the vtk window
#if QT_VERSION < 0x040000
    this->mRenWin->SetDisplayId( this->x11Display() );
#else
    this->mRenWin->SetDisplayId(QX11Info::display());
#endif
#endif

    // special x11 setup
    x11_setup_window();
    
#ifdef Q_WS_MAC
    // give the Qt/Mac window handle to VTK and flag whether we have a parent
    this->mRenWin->SetWindowId(reinterpret_cast<void*>(this->handle()));
    this->mRenWin->SetParentId(reinterpret_cast<void*>(0x1));
#else
    // give the qt window id to the vtk window for Windows and X11
    this->mRenWin->SetWindowId( reinterpret_cast<void*>(this->winId()));
#endif


    // tell the vtk window what the size of this window is
    this->mRenWin->vtkRenderWindow::SetSize(this->width(), this->height());
    this->mRenWin->vtkRenderWindow::SetPosition(this->x(), this->y());
    
    // have VTK start this window and create the necessary graphics resources
    if(isVisible())
      {
      this->mRenWin->Start();
#if defined (Q_WS_MAC) && (QT_VERSION < 0x040000)
      macFixRect();
#endif
      }
    
    // if an interactor wasn't provided, we'll make one by default
    if(!this->mRenWin->GetInteractor())
    {
      // create a default interactor
      QVTKInteractor* iren = QVTKInteractor::New();
      this->mRenWin->SetInteractor(iren);
      iren->Initialize();
        
      // now set the default style
      vtkInteractorStyle* style = vtkInteractorStyleTrackballCamera::New();
      iren->SetInteractorStyle(style);
      
      iren->Delete();
      style->Delete();
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
  if (this->cachedImageCleanFlag) return;

  this->cachedImage = QPixmap::grabWindow(this->winId());
  this->cachedImageCleanFlag = true;
  emit cachedImageClean();
}

void QVTKWidget::setAutomaticImageCacheEnabled(bool flag)
{
  this->automaticImageCache = flag;
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

QPixmap &QVTKWidget::cachedImagePixmap()
{
  // Make sure image is up to date.
  this->paintEvent(NULL);
  this->saveImageToCache();

  return this->cachedImage;
}

/*! overloaded Qt's event handler to capture additional keys that Qt has
    default behavior for (for example the Tab and Shift-Tab key)
*/
bool QVTKWidget::event(QEvent* e)
{
  if(QObject::event(e))
    return TRUE;

  if(e->type() == QEvent::KeyPress)
  {
    QKeyEvent* ke = static_cast<QKeyEvent*>(e);
    keyPressEvent(ke);
    return ke->isAccepted();
  }

  return QWidget::event(e);
}


/*! handle resize event
 */
void QVTKWidget::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);

  if(!this->mRenWin)
    return;

  // give the size to the interactor and vtk window
  this->mRenWin->vtkRenderWindow::SetSize(this->width(), this->height());
  if(this->mRenWin->GetInteractor())
    this->mRenWin->GetInteractor()->SetSize(this->width(), this->height());
  this->markCachedImageAsDirty();
  
#if defined (Q_WS_MAC) && (QT_VERSION < 0x040000)
  macFixRect();
#endif
}

void QVTKWidget::moveEvent(QMoveEvent* event)
{
  QWidget::moveEvent(event);

  if(!this->mRenWin)
    return;
    
  // give the size to the interactor and vtk window
  this->mRenWin->vtkRenderWindow::SetPosition(this->x(), this->y());
  
#if defined (Q_WS_MAC) && (QT_VERSION < 0x040000)
  macFixRect();
#endif
}

/*! handle paint event
 */
void QVTKWidget::paintEvent(QPaintEvent* )
{
  if (this->cachedImageCleanFlag)
    {
    QPainter painter(this);
    painter.drawPixmap(0, 0, this->cachedImage);
    return;
    }

  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    iren = this->mRenWin->GetInteractor();

  if(!iren || !iren->GetEnabled())
    return;

  iren->Render();
}

/*! handle mouse press event
 */
void QVTKWidget::mousePressEvent(QMouseEvent* event)
{

  // Emit a mouse press event for anyone who might be interested
  emit mouseEvent(event);

  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    iren = this->mRenWin->GetInteractor();
  
  if(!iren || !iren->GetEnabled())
    return;

  // give interactor the event information
#if QT_VERSION < 0x040000
  iren->SetEventInformationFlipY(event->x(), event->y(), 
      (event->state() & Qt::ControlButton), 
      (event->state() & Qt::ShiftButton ),
      0,
      event->type() == QEvent::MouseButtonDblClick ? 1 : 0);
#else
  iren->SetEventInformationFlipY(event->x(), event->y(), 
      (event->modifiers() & Qt::ControlModifier), 
      (event->modifiers() & Qt::ShiftModifier ),
      0,
      event->type() == QEvent::MouseButtonDblClick ? 1 : 0);
#endif

  // invoke appropriate vtk event
  switch(event->button())
  {
    case Qt::LeftButton:
      iren->InvokeEvent(vtkCommand::LeftButtonPressEvent, event);
      break;

    case Qt::MidButton:
      iren->InvokeEvent(vtkCommand::MiddleButtonPressEvent, event);
      break;

    case Qt::RightButton:
      iren->InvokeEvent(vtkCommand::RightButtonPressEvent, event);
      break;

    default:
      break;
  }
}

/*! handle mouse move event
 */
void QVTKWidget::mouseMoveEvent(QMouseEvent* event)
{
  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    iren = this->mRenWin->GetInteractor();
  
  if(!iren || !iren->GetEnabled())
    return;
  
  // give interactor the event information
#if QT_VERSION < 0x040000
  iren->SetEventInformationFlipY(event->x(), event->y(), 
      (event->state() & Qt::ControlButton), 
      (event->state() & Qt::ShiftButton ));
#else
  iren->SetEventInformationFlipY(event->x(), event->y(), 
      (event->modifiers() & Qt::ControlModifier), 
      (event->modifiers() & Qt::ShiftModifier ));
#endif
  
  // invoke vtk event
  iren->InvokeEvent(vtkCommand::MouseMoveEvent, event);
}


/*! handle enter event
 */
void QVTKWidget::enterEvent(QEvent* event)
{
  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    iren = this->mRenWin->GetInteractor();
  
  if(!iren || !iren->GetEnabled())
    return;

  iren->InvokeEvent(vtkCommand::EnterEvent, event);
}

/*! handle leave event
 */
void QVTKWidget::leaveEvent(QEvent* event)
{
  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    iren = this->mRenWin->GetInteractor();
  
  if(!iren || !iren->GetEnabled())
    return;
  
  iren->InvokeEvent(vtkCommand::LeaveEvent, event);
}

/*! handle mouse release event
 */
void QVTKWidget::mouseReleaseEvent(QMouseEvent* event)
{
  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    iren = this->mRenWin->GetInteractor();
  
  if(!iren || !iren->GetEnabled())
    return;
  
  // give vtk event information
#if QT_VERSION < 0x040000
  iren->SetEventInformationFlipY(event->x(), event->y(), 
      (event->state() & Qt::ControlButton), 
      (event->state() & Qt::ShiftButton ));
#else
  iren->SetEventInformationFlipY(event->x(), event->y(), 
      (event->modifiers() & Qt::ControlModifier), 
      (event->modifiers() & Qt::ShiftModifier ));
#endif
  
  // invoke appropriate vtk event
  switch(event->button())
  {
    case Qt::LeftButton:
      iren->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, event);
      break;

    case Qt::MidButton:
      iren->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, event);
      break;

    case Qt::RightButton:
      iren->InvokeEvent(vtkCommand::RightButtonReleaseEvent, event);
      break;

    default:
      break;
  }
}

/*! handle key press event
 */
void QVTKWidget::keyPressEvent(QKeyEvent* event)
{
  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    iren = this->mRenWin->GetInteractor();
  
  if(!iren || !iren->GetEnabled())
    return;
  
  // get key and keysym information
  int ascii_key = event->text().length() ? event->text().unicode()->latin1() : 0;
  const char* keysym = ascii_to_key_sym(ascii_key);
  if(!keysym)
  {
    // get virtual keys
    keysym = qt_key_to_key_sym(static_cast<Qt::Key>(event->key()));
  }

  if(!keysym)
  {
    keysym = "None";
  }
  
  // give interactor event information
#if QT_VERSION < 0x040000
  iren->SetKeyEventInformation(
      (event->state() & Qt::ControlButton),
      (event->state() & Qt::ShiftButton),
      ascii_key, event->count(), keysym);
#else
  iren->SetKeyEventInformation(
      (event->modifiers() & Qt::ControlModifier),
      (event->modifiers() & Qt::ShiftModifier),
      ascii_key, event->count(), keysym);
#endif
  
  // invoke vtk event
  iren->InvokeEvent(vtkCommand::KeyPressEvent, event);
  
  // invoke char event only for ascii characters
  if(ascii_key)
    iren->InvokeEvent(vtkCommand::CharEvent, event);
}

/*! handle key release event
 */
void QVTKWidget::keyReleaseEvent(QKeyEvent* event)
{

  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    iren = this->mRenWin->GetInteractor();
  
  if(!iren || !iren->GetEnabled())
    return;
  
  // get key and keysym info
  int ascii_key = event->text().length() ? event->text().unicode()->latin1() : 0;
  const char* keysym = ascii_to_key_sym(ascii_key);
  if(!keysym)
  {
    // get virtual keys
    keysym = qt_key_to_key_sym((Qt::Key)event->key());
  }

  if(!keysym)
  {
    keysym = "None";
  }

  // give event information to interactor
#if QT_VERSION < 0x040000
  iren->SetKeyEventInformation(
      (event->state() & Qt::ControlButton),
      (event->state() & Qt::ShiftButton),
      ascii_key, event->count(), keysym);
#else
  iren->SetKeyEventInformation(
      (event->modifiers() & Qt::ControlModifier),
      (event->modifiers() & Qt::ShiftModifier),
      ascii_key, event->count(), keysym);
#endif

  // invoke vtk event
  iren->InvokeEvent(vtkCommand::KeyReleaseEvent, event);
}

#ifndef QT_NO_WHEELEVENT
void QVTKWidget::wheelEvent(QWheelEvent* event)
{
  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    iren = this->mRenWin->GetInteractor();
  
  if(!iren || !iren->GetEnabled())
    return;

// VTK supports wheel mouse events only in version 4.5 or greater
#if defined(QVTK_HAVE_VTK_4_5)
  
  // give event information to interactor
#if QT_VERSION < 0x040000
  iren->SetEventInformationFlipY(event->x(), event->y(), 
      (event->state() & Qt::ControlButton), 
      (event->state() & Qt::ShiftButton ));
#else
  iren->SetEventInformationFlipY(event->x(), event->y(), 
      (event->modifiers() & Qt::ControlModifier), 
      (event->modifiers() & Qt::ShiftModifier ));
#endif
  
  // invoke vtk event
  // if delta is positive, it is a forward wheel event
  if(event->delta() > 0)
    iren->InvokeEvent(vtkCommand::MouseWheelForwardEvent, event);
  else
    iren->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, event);

#else
  QWidget::wheelEvent(event);
#endif
}
#endif

void QVTKWidget::focusInEvent(QFocusEvent*)
{
    // These prevent updates when the window 
    // gains or loses focus.  By default, Qt
    // does an update because the color group's 
    // active status changes.  We don't even use
    // color groups so we do nothing here.
}

void QVTKWidget::focusOutEvent(QFocusEvent*)
{
    // These prevent updates when the window 
    // gains or loses focus.  By default, Qt
    // does an update because the color group's 
    // active status changes.  We don't even use
    // color groups so we do nothing here.
}


void QVTKWidget::contextMenuEvent(QContextMenuEvent* event)
{
  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    iren = this->mRenWin->GetInteractor();
  
  if(!iren || !iren->GetEnabled())
    return;
  
  // give interactor the event information
#if QT_VERSION < 0x040000
  iren->SetEventInformationFlipY(event->x(), event->y(), 
      (event->state() & Qt::ControlButton), 
      (event->state() & Qt::ShiftButton ));
#else
  iren->SetEventInformationFlipY(event->x(), event->y(), 
      (event->modifiers() & Qt::ControlModifier), 
      (event->modifiers() & Qt::ShiftModifier ));
#endif

  // invoke event and pass qt event for additional data as well
  iren->InvokeEvent(QVTKWidget::ContextMenuEvent, event);
  
}

void QVTKWidget::dragEnterEvent(QDragEnterEvent* event)
{
  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    iren = this->mRenWin->GetInteractor();
  
  if(!iren || !iren->GetEnabled())
    return;
  
  // invoke event and pass qt event for additional data as well
  iren->InvokeEvent(QVTKWidget::DragEnterEvent, event);
}

void QVTKWidget::dragMoveEvent(QDragMoveEvent* event)
{
  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    iren = this->mRenWin->GetInteractor();
  
  if(!iren || !iren->GetEnabled())
    return;
  
  // give interactor the event information
  iren->SetEventInformationFlipY(event->pos().x(), event->pos().y());

  // invoke event and pass qt event for additional data as well
  iren->InvokeEvent(QVTKWidget::DragMoveEvent, event);
}

void QVTKWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    iren = this->mRenWin->GetInteractor();
  
  if(!iren || !iren->GetEnabled())
    return;
  
  // invoke event and pass qt event for additional data as well
  iren->InvokeEvent(QVTKWidget::DragLeaveEvent, event);
}

void QVTKWidget::dropEvent(QDropEvent* event)
{
  vtkRenderWindowInteractor* iren = NULL;
  if(this->mRenWin)
    iren = this->mRenWin->GetInteractor();
  
  if(!iren || !iren->GetEnabled())
    return;
  
  // give interactor the event information
  iren->SetEventInformationFlipY(event->pos().x(), event->pos().y());

  // invoke event and pass qt event for additional data as well
  iren->InvokeEvent(QVTKWidget::DropEvent, event);
}


/*! handle reparenting of widgets
 */
#if QT_VERSION < 0x040000
void QVTKWidget::reparent(QWidget* parent, Qt::WFlags f, const QPoint& p, bool showit)
{
  this->markCachedImageAsDirty();

#if defined(QVTK_HAVE_VTK_4_5)
  if (this->mRenWin)
    {
    // Finalize the window to remove graphics resources associated with
    // this window
    if(this->mRenWin->GetMapped())
      this->mRenWin->Finalize();

    // have QWidget reparent as normal, but don't show
    QWidget::reparent(parent, f, p, false);

    x11_setup_window();
 
    // connect to new window
#if defined(Q_WS_MAC)
    this->mRenWin->SetWindowId(reinterpret_cast<void*>(this->handle()));
#else
    this->mRenWin->SetWindowId( reinterpret_cast<void*>(this->winId()));
#endif

    // start up the window to create graphics resources for this window
    if(isVisible())
      this->mRenWin->Start();
    }
  
  // show if requested
  if(showit)
    show();
#endif
}
#else
void QVTKWidget::setParent(QWidget* parent, Qt::WFlags f)
{
  this->markCachedImageAsDirty();

#if defined(QVTK_HAVE_VTK_4_5)
  if (this->mRenWin)
    {
    // Finalize the window to remove graphics resources associated with
    // this window
    if(this->mRenWin->GetMapped())
      this->mRenWin->Finalize();

    // have QWidget reparent as normal, but don't show
    QWidget::setParent(parent, f);
    
    x11_setup_window();
 
    // connect to new window
#if defined(Q_WS_MAC)
    this->mRenWin->SetWindowId(reinterpret_cast<void*>(this->handle()));
#else
    this->mRenWin->SetWindowId( reinterpret_cast<void*>(this->winId()));
#endif

    // start up the window to create graphics resources for this window
    if(isVisible())
      this->mRenWin->Start();
    }
  
#endif
}
#endif

void QVTKWidget::hide()
{
#if defined(QVTK_HAVE_VTK_4_5)
  if (this->mRenWin)
    {
    // gotta finalize the window on the mac to make it really disappear
    // if it needs starting up again, paintEvent() will do that
    this->mRenWin->Finalize();
    }
#endif
  QWidget::hide();
}

void QVTKWidget::show()
{
  this->markCachedImageAsDirty();

  QWidget::show();
#if defined(QVTK_HAVE_VTK_4_5)
  if (this->mRenWin)
    {
    // gotta start the window on the mac to make it come back
    this->mRenWin->Start();
    }
#endif
}

/*! allocation method for Qt/VTK interactor
*/
vtkStandardNewMacro(QVTKInteractor);

/*! constructor for Qt/VTK interactor
*/
QVTKInteractor::QVTKInteractor()
{
  QObject::connect(&mTimer, SIGNAL(timeout()), this, SLOT(TimerEvent()) );
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


/*! handle timer event
*/
void QVTKInteractor::TimerEvent()
{
  if ( !this->GetEnabled() ) 
    {
      return;
    }
  this->InvokeEvent(vtkCommand::TimerEvent, NULL);
}

/*! constructor
 */
QVTKInteractor::~QVTKInteractor()
{
}

/*! create Qt timer with an interval of 10 msec.
*/
int QVTKInteractor::CreateTimer(int timer_type)
{
  if(timer_type == VTKI_TIMER_FIRST)
  {
    mTimer.start(10);
  }
  return 1;
}

/*! destroy timer
*/
int QVTKInteractor::DestroyTimer()
{
  mTimer.stop();
  return 1;
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
  return AsciiToKeySymTable[i];
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
#if QT_VERSION < 0x040000
    QVTK_HANDLE(Qt::Key_BackTab, "Tab")
#else
    QVTK_HANDLE(Qt::Key_Backtab, "Tab")
#endif
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
#if defined(VTK_USE_OPENGL_LIBRARY) || (!defined(QVTK_HAVE_VTK_4_5) && defined(OPENGL_LIBRARY))
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
#if QT_VERSION < 0x040000
  FocusPolicy focus_policy = focusPolicy();
#else
  Qt::FocusPolicy focus_policy = focusPolicy();
#endif
  bool visible = isVisible();
  if(visible)
    hide();

  
  // get visual and colormap from VTK
  XVisualInfo* vi = 0;
  Colormap cmap = 0;
  Display* display = reinterpret_cast<Display*>(mRenWin->GetGenericDisplayId());

  // check ogl and mesa and get information we need to create a decent window
#if defined(VTK_USE_OPENGL_LIBRARY) || (!defined(QVTK_HAVE_VTK_4_5) && defined(OPENGL_LIBRARY))
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
      show();
    return;
  }

  // create the X window based on information VTK gave us
  XSetWindowAttributes attrib;
  attrib.colormap = cmap;
  attrib.border_pixel = black.pixel();

  Window parent = RootWindow(display, DefaultScreen(display));
  if(parentWidget())
    parent = parentWidget()->winId();
  Window win = XCreateWindow(display, parent, x(), y(), width(), height(),
                             0, vi->depth, InputOutput, vi->visual,
                             CWBorderPixel|CWColormap, &attrib);
  
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
      cmw[count++] = win;
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
  
  XFlush(display);

  // restore widget states
  this->setMouseTracking(tracking);
#if QT_VERSION < 0x040000
  setBackgroundMode( Qt::NoBackground );
#else
  this->setAttribute(Qt::WA_PaintOnScreen);
  this->setAttribute(Qt::WA_NoSystemBackground);
#endif
  setFocusPolicy(focus_policy);
  if(visible)
    show();

#endif
}


#if defined (Q_WS_MAC) && QT_VERSION < 0x040000

// gotta do some special stuff on the MAC to make it work right
// this stuff will need changing when using Qt4 with HIViews

#include <AGL/agl.h>

void QVTKWidget::macFixRect()
{
  if(!this->isTopLevel())
  {
    AGLContext context = (AGLContext)this->GetRenderWindow()->GetGenericDisplayId();
    
    GLint bufRect[4];

    // always do AGL_BUFFER_RECT if we have a parent
    if(!aglIsEnabled(context, AGL_BUFFER_RECT))
      aglEnable(context, AGL_BUFFER_RECT);

    // get the clip region
    QRegion clip = this->clippedRegion();
    QRect clip_rect = clip.boundingRect();
    
    // get the position of this widget with respect to the top level widget
    QPoint mp(posInWindow(this));
    int win_height = this->topLevelWidget()->height();
    win_height -= win_height - this->topLevelWidget()->clippedRegion(FALSE).boundingRect().height();

    // give the position and size to agl
    bufRect[0] = mp.x();
    bufRect[1] = win_height -(mp.y() + this->height());
    bufRect[2] = this->width();
    bufRect[3] = this->height();
    aglSetInteger(context, AGL_BUFFER_RECT, bufRect);

    if(clip_rect.isEmpty())
    {
      // no clipping, disable it
      if(!aglIsEnabled(context, AGL_CLIP_REGION))
        aglDisable(context, AGL_CLIP_REGION);
    }
    else
    {
      // we are clipping, so lets enable it
      if(!aglIsEnabled(context, AGL_CLIP_REGION))
        aglEnable(context, AGL_CLIP_REGION);

      // give agl the clip region
      aglSetInteger(context, AGL_CLIP_REGION, (const GLint*)clip.handle(TRUE));
    }

  }
  else
  {
    // update the context
    aglUpdateContext((AGLContext)this->GetRenderWindow()->GetGenericDisplayId());
  }
}

void QVTKWidget::setRegionDirty(bool b)
{
  // the region is dirty and needs redrawn, but not yet
  // signal that it needs to be done when it is possible
  QWidget::setRegionDirty(b);
  QTimer::singleShot(1, this, SLOT(internalMacFixRect()));

}

void QVTKWidget::macWidgetChangedWindow()
{
  macFixRect();
}
#endif

// slot to update the draw region and draw the scene
void QVTKWidget::internalMacFixRect()
{
#if defined(Q_WS_MAC) && QT_VERSION < 0x040000
  this->macFixRect();
  this->update();
#endif
}

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
