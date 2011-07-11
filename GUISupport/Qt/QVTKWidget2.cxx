/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifdef _MSC_VER
// Disable warnings that Qt headers give.
#pragma warning(disable:4127)
#pragma warning(disable:4512)
#endif

#include "QVTKWidget2.h"
#include "vtkEventQtSlotConnect.h"

#include <QMouseEvent>
#include <QResizeEvent>
#include <QApplication>

#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "QVTKInteractorAdapter.h"
#include "QVTKInteractor.h"

#if defined(VTK_USE_TDX) && defined(Q_WS_X11)
# include "vtkTDxUnixDevice.h"
#endif

QVTKWidget2::QVTKWidget2(QWidget* p, const QGLWidget* shareWidget, Qt::WindowFlags f)
  : QGLWidget(p, shareWidget, f), mRenWin(NULL)
{
  this->UseTDx=false;
  mIrenAdapter = new QVTKInteractorAdapter(this);
  mConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->setMouseTracking(true);       
  this->setAutoBufferSwap(false);
}

QVTKWidget2::QVTKWidget2(QGLContext* ctx, QWidget* p, const QGLWidget* shareWidget, Qt::WindowFlags f)
  : QGLWidget(ctx, p, shareWidget, f), mRenWin(NULL)
{
  this->UseTDx=false;
  mIrenAdapter = new QVTKInteractorAdapter(this);
  mConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->setMouseTracking(true);       
  this->setAutoBufferSwap(false);
}

QVTKWidget2::QVTKWidget2(const QGLFormat& fmt, QWidget* p, const QGLWidget* shareWidget, Qt::WindowFlags f)
  : QGLWidget(fmt, p, shareWidget, f), mRenWin(NULL)
{
  this->UseTDx=false;
  mIrenAdapter = new QVTKInteractorAdapter(this);
  mConnect = vtkSmartPointer<vtkEventQtSlotConnect>::New();
  this->setMouseTracking(true);       
  this->setAutoBufferSwap(false);
}

/*! destructor */

QVTKWidget2::~QVTKWidget2()
{
  // get rid of the VTK window
  this->SetRenderWindow(NULL);
}

// ----------------------------------------------------------------------------
void QVTKWidget2::SetUseTDx(bool useTDx)
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
bool QVTKWidget2::GetUseTDx() const
{
  return this->UseTDx;
}

/*! get the render window
 */
vtkGenericOpenGLRenderWindow* QVTKWidget2::GetRenderWindow()
{
  if (!this->mRenWin)
    {
    // create a default vtk window
    vtkGenericOpenGLRenderWindow* win = vtkGenericOpenGLRenderWindow::New();
    this->SetRenderWindow(win);
    win->Delete();
    }

  return this->mRenWin;
}

/*! set the render window
  this will bind a VTK window with the Qt window
  it'll also replace an existing VTK window
*/
void QVTKWidget2::SetRenderWindow(vtkGenericOpenGLRenderWindow* w)
{
  // do nothing if we don't have to
  if(w == this->mRenWin)
    {
    return;
    }

  // unregister previous window
  if(this->mRenWin)
    {
    this->mRenWin->Finalize();
    mConnect->Disconnect(mRenWin, vtkCommand::WindowMakeCurrentEvent, this, SLOT(MakeCurrent()));
    mConnect->Disconnect(mRenWin, vtkCommand::WindowIsCurrentEvent, this, SLOT(IsCurrent(vtkObject*, unsigned long, void*, void*)));
    mConnect->Disconnect(mRenWin, vtkCommand::WindowFrameEvent, this, SLOT(Frame()));
    mConnect->Disconnect(mRenWin, vtkCommand::StartEvent, this, SLOT(Start()));
    mConnect->Disconnect(mRenWin, vtkCommand::EndEvent, this, SLOT(End()));
    mConnect->Disconnect(mRenWin, vtkCommand::WindowIsDirectEvent, this, SLOT(IsDirect(vtkObject*, unsigned long, void*, void*)));
    mConnect->Disconnect(mRenWin, vtkCommand::WindowSupportsOpenGLEvent, this, SLOT(SupportsOpenGL(vtkObject*, unsigned long, void*, void*)));
    }

  // now set the window
  this->mRenWin = w;

  if(this->mRenWin)
    {
    // if it is mapped somewhere else, unmap it
    this->mRenWin->Finalize();

    // tell the vtk window what the size of this window is
    this->mRenWin->SetSize(this->width(), this->height());
    this->mRenWin->SetPosition(this->x(), this->y());

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

    mConnect->Connect(mRenWin, vtkCommand::WindowMakeCurrentEvent, this, SLOT(MakeCurrent()));
    mConnect->Connect(mRenWin, vtkCommand::WindowIsCurrentEvent, this, SLOT(IsCurrent(vtkObject*, unsigned long, void*, void*)));
    mConnect->Connect(mRenWin, vtkCommand::WindowFrameEvent, this, SLOT(Frame()));
    mConnect->Connect(mRenWin, vtkCommand::StartEvent, this, SLOT(Start()));
    mConnect->Connect(mRenWin, vtkCommand::EndEvent, this, SLOT(End()));
    mConnect->Connect(mRenWin, vtkCommand::WindowIsDirectEvent, this, SLOT(IsDirect(vtkObject*, unsigned long, void*, void*)));
    mConnect->Connect(mRenWin, vtkCommand::WindowSupportsOpenGLEvent, this, SLOT(SupportsOpenGL(vtkObject*, unsigned long, void*, void*)));
    }
}



/*! get the Qt/VTK interactor
 */
QVTKInteractor* QVTKWidget2::GetInteractor()
{
  return QVTKInteractor
    ::SafeDownCast(this->GetRenderWindow()->GetInteractor());
}

void QVTKWidget2::Start()
{
  makeCurrent();
  mRenWin->PushState();
  mRenWin->OpenGLInit();
}

void QVTKWidget2::End()
{
  mRenWin->PopState();
}

/*! handle resize event
 */
void QVTKWidget2::resizeGL(int w, int h)
{
  if(!this->mRenWin)
    {
    return;
    }

  this->mRenWin->SetSize(w,h);

  // and update the interactor
  if(this->mRenWin->GetInteractor())
    {
    QResizeEvent e(QSize(w,h), QSize());
    mIrenAdapter->ProcessEvent(&e, this->mRenWin->GetInteractor());
    }
}

void QVTKWidget2::moveEvent(QMoveEvent* e)
{
  QWidget::moveEvent(e);

  if(!this->mRenWin)
    {
    return;
    }

  this->mRenWin->SetPosition(this->x(), this->y());
}

/*! handle paint event
 */
void QVTKWidget2::paintGL()
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

  iren->Render();
}

/*! handle mouse press event
 */
void QVTKWidget2::mousePressEvent(QMouseEvent* e)
{
  if(this->mRenWin)
    {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
    }

}

/*! handle mouse move event
 */
void QVTKWidget2::mouseMoveEvent(QMouseEvent* e)
{
  if(this->mRenWin)
    {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
    }
}


/*! handle enter event
 */
void QVTKWidget2::enterEvent(QEvent* e)
{
  if(this->mRenWin)
    {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
    }
}

/*! handle leave event
 */
void QVTKWidget2::leaveEvent(QEvent* e)
{
  if(this->mRenWin)
    {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
    }
}

/*! handle mouse release event
 */
void QVTKWidget2::mouseReleaseEvent(QMouseEvent* e)
{
  if(this->mRenWin)
    {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
    }
}

/*! handle key press event
 */
void QVTKWidget2::keyPressEvent(QKeyEvent* e)
{
  if(this->mRenWin)
    {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
    }
}

/*! handle key release event
 */
void QVTKWidget2::keyReleaseEvent(QKeyEvent* e)
{
  if(this->mRenWin)
    {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
    }
}

void QVTKWidget2::wheelEvent(QWheelEvent* e)
{
  if(this->mRenWin)
    {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
    }
}

void QVTKWidget2::contextMenuEvent(QContextMenuEvent* e)
{
  if(this->mRenWin)
    {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
    }
}

void QVTKWidget2::dragEnterEvent(QDragEnterEvent* e)
{
  if(this->mRenWin)
    {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
    }
}

void QVTKWidget2::dragMoveEvent(QDragMoveEvent* e)
{
  if(this->mRenWin)
    {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
    }
}

void QVTKWidget2::dragLeaveEvent(QDragLeaveEvent* e)
{
  if(this->mRenWin)
    {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
    }
}

void QVTKWidget2::dropEvent(QDropEvent* e)
{
  if(this->mRenWin)
    {
    mIrenAdapter->ProcessEvent(e, this->mRenWin->GetInteractor());
    }
}

#ifdef VTK_USE_TDX
// Description:
// Receive notification of the creation of the TDxDevice
void QVTKWidget2::setDevice(vtkTDxDevice *device)
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

void QVTKWidget2::MakeCurrent()
{
  this->makeCurrent();
}

void QVTKWidget2::IsCurrent(vtkObject*, unsigned long, void*, void* call_data)
{
  bool* ptr = reinterpret_cast<bool*>(call_data);
  *ptr = QGLContext::currentContext() == this->context();
}

void QVTKWidget2::IsDirect(vtkObject*, unsigned long, void*, void* call_data)
{
  int* ptr = reinterpret_cast<int*>(call_data);
  *ptr = this->context()->format().directRendering();
}

void QVTKWidget2::SupportsOpenGL(vtkObject*, unsigned long, void*, void* call_data)
{
  int* ptr = reinterpret_cast<int*>(call_data);
  *ptr = QGLFormat::hasOpenGL();
}

void QVTKWidget2::Frame()
{
  if(mRenWin->GetSwapBuffers())
    this->swapBuffers();
  
  // This callback will call swapBuffers() for us
  // because sometimes VTK does a render without coming through this paintGL()

  // if you want paintGL to always be called for each time VTK renders
  // 1. turn off EnableRender on the interactor,
  // 2. turn off SwapBuffers on the render window,
  // 3. add an observer for the RenderEvent coming from the interactor
  // 4. implement the callback on the observer to call updateGL() on this widget
  // 5. overload QVTKWidget2::paintGL() to call mRenWin->Render() instead iren->Render()

}

void QVTKWidget2::setAutoBufferSwap(bool f)
{
  QGLWidget::setAutoBufferSwap(f);
}
  
bool QVTKWidget2::autoBufferSwap() const
{
  return QGLWidget::autoBufferSwap();
}
