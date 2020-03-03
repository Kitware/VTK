/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QVTKOpenGLStereoWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "QVTKOpenGLStereoWidget.h"

#include "QVTKInteractor.h"
#include "QVTKInteractorAdapter.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkLogger.h"
#include "vtkRenderWindowInteractor.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QLayout>
#include <QOpenGLContext>
#include <QResizeEvent>
#include <QSurfaceFormat>
#include <QtDebug>

//-----------------------------------------------------------------------------
QVTKOpenGLStereoWidget::QVTKOpenGLStereoWidget(QWidget* parent, Qt::WindowFlags f)
  : QVTKOpenGLStereoWidget(vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New(), nullptr, parent, f)
{
}

//-----------------------------------------------------------------------------
QVTKOpenGLStereoWidget::QVTKOpenGLStereoWidget(
  QOpenGLContext* shareContext, QWidget* parent, Qt::WindowFlags f)
  : QVTKOpenGLStereoWidget(
      vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New(), shareContext, parent, f)
{
}

//-----------------------------------------------------------------------------
QVTKOpenGLStereoWidget::QVTKOpenGLStereoWidget(
  vtkGenericOpenGLRenderWindow* w, QWidget* parent, Qt::WindowFlags f)
  : QVTKOpenGLStereoWidget(w, QOpenGLContext::currentContext(), parent, f)
{
}

//-----------------------------------------------------------------------------
QVTKOpenGLStereoWidget::QVTKOpenGLStereoWidget(
  vtkGenericOpenGLRenderWindow* w, QOpenGLContext* shareContext, QWidget* parent, Qt::WindowFlags f)
  : QWidget(parent, f)
  , VTKOpenGLWindow(nullptr)
{
  QVBoxLayout* vBoxLayout = new QVBoxLayout(this);
  vBoxLayout->setContentsMargins(0, 0, 0, 0);

  this->VTKOpenGLWindow = new QVTKOpenGLWindow(w, shareContext);
  QWidget* container = QWidget::createWindowContainer(this->VTKOpenGLWindow, this, f);
  container->setAttribute(Qt::WA_TransparentForMouseEvents);
  container->setMouseTracking(true);
  vBoxLayout->addWidget(container);

  // Forward signals triggered by the internal QVTKOpenGLWindow
  QObject::connect(this->VTKOpenGLWindow.data(), &QVTKOpenGLWindow::windowEvent,
    [this](QEvent* evt) { QApplication::sendEvent(this, evt); });

  // enable mouse tracking to process mouse events
  this->setMouseTracking(true);

  // default to strong focus to accept focus by tabbing and clicking
  this->setFocusPolicy(Qt::StrongFocus);

  // Work around for bug paraview/paraview#18285
  // https://gitlab.kitware.com/paraview/paraview/issues/18285
  // This ensure that kde will not grab the window
  this->setProperty("_kde_no_window_grab", true);

  // enable qt gesture events
  grabGesture(Qt::PinchGesture);
  grabGesture(Qt::PanGesture);
  grabGesture(Qt::TapGesture);
  grabGesture(Qt::TapAndHoldGesture);
  grabGesture(Qt::SwipeGesture);
}

//-----------------------------------------------------------------------------
QVTKOpenGLStereoWidget::~QVTKOpenGLStereoWidget() {}

//-----------------------------------------------------------------------------
QImage QVTKOpenGLStereoWidget::grabFramebuffer()
{
  return this->VTKOpenGLWindow->grabFramebuffer();
}

//-----------------------------------------------------------------------------
void QVTKOpenGLStereoWidget::resizeEvent(QResizeEvent* evt)
{
  vtkLogScopeF(TRACE, "resizeEvent(%d, %d)", evt->size().width(), evt->size().height());
  this->Superclass::resizeEvent(evt);
}

//-----------------------------------------------------------------------------
void QVTKOpenGLStereoWidget::paintEvent(QPaintEvent* evt)
{
  vtkLogScopeF(TRACE, "paintEvent");
  this->Superclass::paintEvent(evt);

  // this is generally not needed; however, there are cases when the after a
  // resize, the embedded QVTKOpenGLWindow doesn't repaint even though it
  // correctly gets the resize event; explicitly triggering update on the
  // internal widget overcomes that issue.
  this->VTKOpenGLWindow->update();
}

//-----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
void QVTKOpenGLStereoWidget::SetRenderWindow(vtkRenderWindow* win)
{
  VTK_LEGACY_REPLACED_BODY(
    QVTKOpenGLStereoWidget::SetRenderWindow, "VTK 8.3", QVTKOpenGLStereoWidget::setRenderWindow);
  vtkGenericOpenGLRenderWindow* gwin = vtkGenericOpenGLRenderWindow::SafeDownCast(win);
  if (gwin == nullptr && win != nullptr)
  {
    qDebug() << "QVTKOpenGLStereoWidget requires a `vtkGenericOpenGLRenderWindow`. `"
             << win->GetClassName() << "` is not supported.";
  }
  this->setRenderWindow(gwin);
}
#endif

//-----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
void QVTKOpenGLStereoWidget::SetRenderWindow(vtkGenericOpenGLRenderWindow* win)
{
  VTK_LEGACY_REPLACED_BODY(
    QVTKOpenGLStereoWidget::SetRenderWindow, "VTK 8.3", QVTKOpenGLStereoWidget::setRenderWindow);
  this->setRenderWindow(win);
}
#endif

//-----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
vtkRenderWindow* QVTKOpenGLStereoWidget::GetRenderWindow()
{
  VTK_LEGACY_REPLACED_BODY(
    QVTKOpenGLStereoWidget::GetRenderWindow, "VTK 8.3", QVTKOpenGLStereoWidget::renderWindow);
  return this->renderWindow();
}
#endif

//-----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
QVTKInteractorAdapter* QVTKOpenGLStereoWidget::GetInteractorAdapter()
{
  VTK_LEGACY_BODY(QVTKOpenGLStereoWidget::GetInteractorAdapter, "VTK 8.3");
  return nullptr;
}
#endif

//-----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
QVTKInteractor* QVTKOpenGLStereoWidget::GetInteractor()
{
  VTK_LEGACY_REPLACED_BODY(
    QVTKOpenGLStereoWidget::GetInteractor, "VTK 8.3", QVTKOpenGLStereoWidget::interactor);
  return this->interactor();
}
#endif

//-----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
void QVTKOpenGLStereoWidget::setQVTKCursor(const QCursor& cursor)
{
  VTK_LEGACY_REPLACED_BODY(
    QVTKOpenGLStereoWidget::setQVTKCursor, "VTK 8.3", QVTKOpenGLStereoWidget::setCursor);
  this->setCursor(cursor);
}
#endif

//-----------------------------------------------------------------------------
#if !defined(VTK_LEGACY_REMOVE)
void QVTKOpenGLStereoWidget::setDefaultQVTKCursor(const QCursor& cursor)
{
  VTK_LEGACY_REPLACED_BODY(QVTKOpenGLStereoWidget::setDefaultQVTKCursor, "VTK 8.3",
    QVTKOpenGLStereoWidget::setDefaultCursor);
  this->setDefaultCursor(cursor);
}
#endif
