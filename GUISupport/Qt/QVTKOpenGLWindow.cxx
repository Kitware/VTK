/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKOpenGLWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "QVTKOpenGLWindow.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_2_Core>
#include <QOpenGLTexture>
#include <QPointer>
#include <QScopedValueRollback>
#include <QtDebug>

#include "QVTKInteractor.h"
#include "QVTKInteractorAdapter.h"
#include "QVTKRenderWindowAdapter.h"
#include "vtkCommand.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLState.h"

QVTKOpenGLWindow::QVTKOpenGLWindow(QOpenGLWindow::UpdateBehavior ub, QWindow* p)
  : QVTKOpenGLWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New(), nullptr, ub, p)
{
}

QVTKOpenGLWindow::QVTKOpenGLWindow(
  QOpenGLContext* shareContext, QOpenGLWindow::UpdateBehavior ub, QWindow* p)
  : QVTKOpenGLWindow(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New(), shareContext, ub, p)
{
}

QVTKOpenGLWindow::QVTKOpenGLWindow(
  vtkGenericOpenGLRenderWindow* rw, QOpenGLWindow::UpdateBehavior ub, QWindow* p)
  : QVTKOpenGLWindow(rw, nullptr, ub, p)
{
}

QVTKOpenGLWindow::QVTKOpenGLWindow(vtkGenericOpenGLRenderWindow* renderWin,
  QOpenGLContext* shareContext, QOpenGLWindow::UpdateBehavior ub, QWindow* p)
  : Superclass(shareContext, ub, p)
  , RenderWindow(nullptr)
  , RenderWindowAdapter(nullptr)
  , EnableHiDPI(true)
  , UnscaledDPI(72)
  , DefaultCursor(QCursor(Qt::ArrowCursor))
{
  this->setRenderWindow(renderWin);
}

//-----------------------------------------------------------------------------
QVTKOpenGLWindow::~QVTKOpenGLWindow()
{
  this->makeCurrent();
  this->cleanupContext();
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWindow::setRenderWindow(vtkRenderWindow* win)
{
  vtkGenericOpenGLRenderWindow* gwin = vtkGenericOpenGLRenderWindow::SafeDownCast(win);
  if (gwin == nullptr && win != nullptr)
  {
    qDebug() << "QVTKOpenGLWindow requires a `vtkGenericOpenGLRenderWindow`. `"
             << win->GetClassName() << "` is not supported.";
  }
  this->setRenderWindow(gwin);
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWindow::setRenderWindow(vtkGenericOpenGLRenderWindow* win)
{
  if (this->RenderWindow == win)
  {
    return;
  }

  // this will release all OpenGL resources associated with the old render
  // window, if any.
  if (this->RenderWindowAdapter)
  {
    this->makeCurrent();
    this->RenderWindowAdapter.reset(nullptr);
  }
  this->RenderWindow = win;
  if (this->RenderWindow)
  {
    this->RenderWindow->SetReadyForRendering(false);

    // if an interactor wasn't provided, we'll make one by default
    if (!this->RenderWindow->GetInteractor())
    {
      // create a default interactor
      vtkNew<QVTKInteractor> iren;
      // iren->SetUseTDx(this->UseTDx);
      this->RenderWindow->SetInteractor(iren);
      iren->Initialize();

      // now set the default style
      vtkNew<vtkInteractorStyleTrackballCamera> style;
      iren->SetInteractorStyle(style);
    }

    if (this->isValid())
    {
      // this typically means that the render window is being changed after the
      // QVTKOpenGLWindow has initialized itself in a previous update
      // pass, so we emulate the steps to ensure that the new vtkRenderWindow is
      // brought to the same state (minus the actual render).
      this->makeCurrent();
      this->initializeGL();
      this->updateSize();
    }
  }
}

//-----------------------------------------------------------------------------
vtkRenderWindow* QVTKOpenGLWindow::renderWindow() const
{
  return this->RenderWindow;
}

//-----------------------------------------------------------------------------
QVTKInteractor* QVTKOpenGLWindow::interactor() const
{
  return this->RenderWindow ? QVTKInteractor::SafeDownCast(this->RenderWindow->GetInteractor())
                            : nullptr;
}

//-----------------------------------------------------------------------------
QSurfaceFormat QVTKOpenGLWindow::defaultFormat(bool stereo_capable)
{
  return QVTKRenderWindowAdapter::defaultFormat(stereo_capable);
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWindow::setEnableHiDPI(bool enable)
{
  this->EnableHiDPI = enable;
  if (this->RenderWindowAdapter)
  {
    this->RenderWindowAdapter->setEnableHiDPI(enable);
  }
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWindow::setUnscaledDPI(int dpi)
{
  this->UnscaledDPI = dpi;
  if (this->RenderWindowAdapter)
  {
    this->RenderWindowAdapter->setUnscaledDPI(dpi);
  }
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWindow::setDefaultCursor(const QCursor& cursor)
{
  this->DefaultCursor = cursor;
  if (this->RenderWindowAdapter)
  {
    this->RenderWindowAdapter->setDefaultCursor(cursor);
  }
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWindow::initializeGL()
{
  this->Superclass::initializeGL();
  if (this->RenderWindow)
  {
    Q_ASSERT(this->RenderWindowAdapter.data() == nullptr);
    this->RenderWindowAdapter.reset(
      new QVTKRenderWindowAdapter(this->context(), this->RenderWindow, this));
    this->RenderWindowAdapter->setDefaultCursor(this->defaultCursor());
    this->RenderWindowAdapter->setEnableHiDPI(this->EnableHiDPI);
    this->RenderWindowAdapter->setUnscaledDPI(this->UnscaledDPI);
  }
  this->connect(this->context(), SIGNAL(aboutToBeDestroyed()), SLOT(cleanupContext()),
    static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::DirectConnection));
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWindow::updateSize()
{
  if (this->RenderWindowAdapter)
  {
    this->RenderWindowAdapter->resize(this->width(), this->height());
  }
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWindow::resizeGL(int w, int h)
{
  vtkLogF(TRACE, "resizeGL(%d, %d)", w, h);
  this->Superclass::resizeGL(w, h);
  this->updateSize();
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWindow::paintGL()
{
  vtkLogF(TRACE, "paintGL");
  this->Superclass::paintGL();
  if (this->RenderWindow)
  {
    Q_ASSERT(this->RenderWindowAdapter);
    this->RenderWindowAdapter->paint();

    // If render was triggered by above calls, that may change the current context
    // due to things like progress events triggering updates on other widgets
    // (e.g. progress bar). Hence we need to make sure to call makeCurrent()
    // before proceeding with blit-ing.
    this->makeCurrent();

    QOpenGLFunctions_3_2_Core* f =
      QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_2_Core>();
    if (f)
    {
      const QSize deviceSize = this->size() * this->devicePixelRatioF();
      const auto fmt = this->context()->format();
      if (fmt.stereo() && this->RenderWindow->GetStereoRender() &&
        this->RenderWindow->GetStereoType() == VTK_STEREO_CRYSTAL_EYES)
      {
        this->RenderWindowAdapter->blitLeftEye(
          this->defaultFramebufferObject(), GL_BACK_LEFT, QRect(QPoint(0, 0), deviceSize));
        this->RenderWindowAdapter->blitRightEye(
          this->defaultFramebufferObject(), GL_BACK_RIGHT, QRect(QPoint(0, 0), deviceSize));
      }
      else
      {
        this->RenderWindowAdapter->blit(
          this->defaultFramebufferObject(), GL_BACK, QRect(QPoint(0, 0), deviceSize));
      }
    }
  }
  else
  {
    // no render window set, just fill with white.
    QOpenGLFunctions* f = QOpenGLContext::currentContext()->functions();
    f->glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    f->glClear(GL_COLOR_BUFFER_BIT);
  }
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWindow::cleanupContext()
{
  this->RenderWindowAdapter.reset(nullptr);
}

//-----------------------------------------------------------------------------
bool QVTKOpenGLWindow::event(QEvent* evt)
{
  // Forward event to the Widget containing this window. This is required
  // due to QTBUG-61836 that prevents the use of the flag
  // Qt::TransparentForMouseInput. This flag should indicate that this window
  // should not catch any event and let them pass through to the widget.
  // The containing widget should then forward back only the required events for
  // this window (such as mouse events and resize events).
  // Until this misbehavior is fixed, we have to handle forwarding of events.
  emit this->windowEvent(evt);

  if (this->RenderWindowAdapter)
  {
    this->RenderWindowAdapter->handleEvent(evt);
  }

  return this->Superclass::event(evt);
}

//-----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
void QVTKOpenGLWindow::SetRenderWindow(vtkRenderWindow* win)
{
  VTK_LEGACY_REPLACED_BODY(
    QVTKOpenGLWindow::SetRenderWindow, "VTK 9.0", QVTKOpenGLWindow::setRenderWindow);
  vtkGenericOpenGLRenderWindow* gwin = vtkGenericOpenGLRenderWindow::SafeDownCast(win);
  if (gwin == nullptr && win != nullptr)
  {
    qDebug() << "QVTKOpenGLWindow requires a `vtkGenericOpenGLRenderWindow`. `"
             << win->GetClassName() << "` is not supported.";
  }
  this->setRenderWindow(gwin);
}
#endif

//-----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
void QVTKOpenGLWindow::SetRenderWindow(vtkGenericOpenGLRenderWindow* win)
{
  VTK_LEGACY_REPLACED_BODY(
    QVTKOpenGLWindow::SetRenderWindow, "VTK 9.0", QVTKOpenGLWindow::setRenderWindow);
  this->setRenderWindow(win);
}
#endif

//-----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
vtkRenderWindow* QVTKOpenGLWindow::GetRenderWindow()
{
  VTK_LEGACY_REPLACED_BODY(
    QVTKOpenGLWindow::GetRenderWindow, "VTK 9.0", QVTKOpenGLWindow::renderWindow);
  return this->renderWindow();
}
#endif

//-----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
QVTKInteractorAdapter* QVTKOpenGLWindow::GetInteractorAdapter()
{
  VTK_LEGACY_BODY(QVTKOpenGLWindow::GetInteractorAdapter, "VTK 9.0");
  return nullptr;
}
#endif

//-----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
QVTKInteractor* QVTKOpenGLWindow::GetInteractor()
{
  VTK_LEGACY_REPLACED_BODY(
    QVTKOpenGLWindow::GetInteractor, "VTK 9.0", QVTKOpenGLWindow::interactor);
  return this->interactor();
}
#endif

//-----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
void QVTKOpenGLWindow::setQVTKCursor(const QCursor& cursor)
{
  VTK_LEGACY_REPLACED_BODY(QVTKOpenGLWindow::setQVTKCursor, "VTK 9.0", QVTKOpenGLWindow::setCursor);
  this->setCursor(cursor);
}
#endif

//-----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
void QVTKOpenGLWindow::setDefaultQVTKCursor(const QCursor& cursor)
{
  VTK_LEGACY_REPLACED_BODY(
    QVTKOpenGLWindow::setDefaultQVTKCursor, "VTK 9.0", QVTKOpenGLWindow::setDefaultCursor);
  this->setDefaultCursor(cursor);
}
#endif
