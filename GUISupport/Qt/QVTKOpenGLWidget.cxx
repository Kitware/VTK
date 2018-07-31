/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKOpenGLWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "QVTKOpenGLWidget.h"

#include "QVTKInteractor.h"
#include "QVTKInteractorAdapter.h"
#include "QVTKOpenGLWindow.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QLayout>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QSurfaceFormat>
#include <QtDebug>

//-----------------------------------------------------------------------------
QVTKOpenGLWidget::QVTKOpenGLWidget(QWidget* parent, Qt::WindowFlags f)
  : QVTKOpenGLWidget(QOpenGLContext::currentContext(), parent, f)
{}

//-----------------------------------------------------------------------------
QVTKOpenGLWidget::QVTKOpenGLWidget(QOpenGLContext *shareContext,
  QWidget* parent, Qt::WindowFlags f)
  : QVTKOpenGLWidget(nullptr, shareContext, parent, f)
{}

//-----------------------------------------------------------------------------
QVTKOpenGLWidget::QVTKOpenGLWidget(vtkGenericOpenGLRenderWindow* w,
  QWidget* parent, Qt::WindowFlags f)
  : QVTKOpenGLWidget(w, QOpenGLContext::currentContext(), parent, f)
{}

//-----------------------------------------------------------------------------
QVTKOpenGLWidget::QVTKOpenGLWidget(vtkGenericOpenGLRenderWindow* w,
  QOpenGLContext *shareContext, QWidget* parent, Qt::WindowFlags f)
  : QWidget(parent, f)
{
  // Work around for bug paraview/paraview#18285
  // https://gitlab.kitware.com/paraview/paraview/issues/18285
  // This ensure that kde will not grab the window
  this->setProperty("_kde_no_window_grab", true);

  // Internal QVTKOpenGLWindow creation
  this->qVTKOpenGLWindowInternal = new QVTKOpenGLWindow(w, shareContext);
  QVBoxLayout* vBoxLayout = new QVBoxLayout(this);
  QWidget* container = QWidget::createWindowContainer(this->qVTKOpenGLWindowInternal, this, f);
  container->setAttribute(Qt::WA_TransparentForMouseEvents);
  container->setMouseTracking(true);

  vBoxLayout->addWidget(container);

  vBoxLayout->setContentsMargins(0,0,0,0);

  // Forward signals triggered by the internal QVTKOpenGLWindow
  this->connect(this->qVTKOpenGLWindowInternal, SIGNAL(windowEvent(QEvent*)),
    this, SLOT(windowEvent(QEvent*)));

  this->connect(this, SIGNAL(widgetEvent(QEvent*)),
    this->qVTKOpenGLWindowInternal, SLOT(widgetEvent(QEvent*)));

  // enable mouse tracking to process mouse events
  this->setMouseTracking(true);

  // enable keyboard focus to process keyboard events
  this->setFocus();
  // default to strong focus to accept focus by tabbing and clicking
  this->setFocusPolicy(Qt::StrongFocus);

  // forward the original QWidget size to the internal window
  this->resize(Superclass::size());

  // enable qt gesture events
  grabGesture(Qt::PinchGesture);
  grabGesture(Qt::PanGesture);
  grabGesture(Qt::TapGesture);
  grabGesture(Qt::TapAndHoldGesture);
  grabGesture(Qt::SwipeGesture);
}

//-----------------------------------------------------------------------------
QVTKOpenGLWidget::~QVTKOpenGLWidget()
{
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::SetRenderWindow(vtkGenericOpenGLRenderWindow* renWin)
{
  this->qVTKOpenGLWindowInternal->SetRenderWindow(renWin);
  this->qVTKOpenGLWindowInternal->setEnableHiDPI(this->EnableHiDPI);
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::SetRenderWindow(vtkRenderWindow* win)
{
  vtkGenericOpenGLRenderWindow* gwin =
    vtkGenericOpenGLRenderWindow::SafeDownCast(win);
  this->SetRenderWindow(gwin);
  if (gwin == nullptr && win != nullptr)
  {
    qDebug() << "QVTKOpenGLWidget requires a `vtkGenericOpenGLRenderWindow`. `"
             << win->GetClassName() << "` is not supported.";
  }
  this->qVTKOpenGLWindowInternal->setEnableHiDPI(this->EnableHiDPI);
}

//-----------------------------------------------------------------------------
vtkRenderWindow* QVTKOpenGLWidget::GetRenderWindow()
{
  return this->qVTKOpenGLWindowInternal->GetRenderWindow();
}

//-----------------------------------------------------------------------------
vtkRenderWindowInteractor* QVTKOpenGLWidget::GetInteractor()
{
  return this->GetRenderWindow()->GetInteractor();
}

//-----------------------------------------------------------------------------
QVTKInteractorAdapter* QVTKOpenGLWidget::GetInteractorAdapter()
{
  return this->qVTKOpenGLWindowInternal->GetInteractorAdapter();
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::setFormat(const QSurfaceFormat& format)
{
  this->qVTKOpenGLWindowInternal->setFormat(format);
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::setEnableHiDPI(bool enable)
{
  this->EnableHiDPI = enable;
  this->qVTKOpenGLWindowInternal->setEnableHiDPI(this->EnableHiDPI);
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::setQVTKCursor(const QCursor &cursor)
{
  this->qVTKOpenGLWindowInternal->setCursor(cursor);
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::windowEvent(QEvent* e)
{
  QApplication::sendEvent(this, e);
}

//-----------------------------------------------------------------------------
bool QVTKOpenGLWidget::event(QEvent* e)
{
  if (e->type() == QEvent::MouseMove ||
    e->type() == QEvent::MouseButtonRelease ||
    e->type() == QEvent::MouseButtonPress ||
    e->type() == QEvent::MouseButtonDblClick)
  {
    QMouseEvent* mouse_event = static_cast<QMouseEvent*>(e);
    if (e != nullptr)
    {
      emit(mouseEvent(mouse_event));
    }
  }
  return Superclass::event(e);
}

//-----------------------------------------------------------------------------
void QVTKOpenGLWidget::resizeEvent(QResizeEvent* event)
{
  Superclass::resizeEvent(event);

  emit(resized());

  const qreal devicePixelRatio_ = this->EnableHiDPI ? this->qVTKOpenGLWindowInternal->devicePixelRatio() : 1.;
  const QSize widgetSize = this->size();
  const QSize deviceSize = widgetSize * devicePixelRatio_;

  this->qVTKOpenGLWindowInternal->GetInteractorAdapter()->SetDevicePixelRatio(devicePixelRatio_);

  // pass the new size to the internal window
  if (this->GetInteractor())
  {
    this->GetInteractor()->SetSize(deviceSize.width(), deviceSize.height());
  }

  vtkGenericOpenGLRenderWindow* w = vtkGenericOpenGLRenderWindow::SafeDownCast(
    this->GetRenderWindow());

  if (w != nullptr)
  {
    w->SetScreenSize(deviceSize.width(), deviceSize.height());
    w->SetSize(deviceSize.width(), deviceSize.height());
    // Set screen size on render window.
    const QRect screenGeometry = QApplication::desktop()->screenGeometry(this);
    w->SetScreenSize(screenGeometry.width(), screenGeometry.height());
    w->SetPosition(this->x() * devicePixelRatio_, this->y() * devicePixelRatio_);
  }

  if (deviceSize.width() > 0 && deviceSize.height() > 0 && this->isValid())
  {
    this->GetRenderWindow()->GetInteractor()->Render();
    // Release the context for other windows to use it.
    this->qVTKOpenGLWindowInternal->doneCurrent();
  }

  // Having this widget as a native widget can cause undesirable stacking
  // issues with its internal QOpenGLWindow.
  Q_ASSERT(this->testAttribute(Qt::WA_NativeWindow) == false);
}

//-----------------------------------------------------------------------------
bool QVTKOpenGLWidget::testingEvent(QEvent* e)
{
  // Forward mouse and resize events to the internal QVTKOpenGLWindow
  // (QTBUG-61836)
  if (e->type() == QEvent::MouseMove ||
    e->type() == QEvent::MouseButtonRelease ||
    e->type() == QEvent::MouseButtonPress ||
    e->type() == QEvent::MouseButtonDblClick ||
    e->type() == QEvent::Wheel ||
    e->type() == QEvent::Resize)
  {
    emit(widgetEvent(e));
  }

  return true;
}

//-----------------------------------------------------------------------------
QSurfaceFormat QVTKOpenGLWidget::defaultFormat()
{
  return QVTKOpenGLWindow::defaultFormat();
}

//-----------------------------------------------------------------------------
bool QVTKOpenGLWidget::isValid()
{
  return this->qVTKOpenGLWindowInternal->isValid();
}

//-----------------------------------------------------------------------------
QImage QVTKOpenGLWidget::grabFramebuffer()
{
  return this->qVTKOpenGLWindowInternal->grabFramebuffer();
}
