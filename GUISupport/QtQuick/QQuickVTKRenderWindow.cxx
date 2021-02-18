/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QQuickVTKRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "QQuickVTKRenderWindow.h"

// vtk includes
#include "QQuickVTKInteractorAdapter.h"
#include "QVTKInteractor.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkRenderWindowInteractor.h"

// Qt includes
#include <QQuickWindow>

//-------------------------------------------------------------------------------------------------
QQuickVTKRenderWindow::QQuickVTKRenderWindow(QQuickItem* parent)
  : Superclass(parent)
{
  // Accept mouse events
  setAcceptHoverEvents(true);
  setAcceptedMouseButtons(Qt::AllButtons);

  this->setRenderWindow(vtkGenericOpenGLRenderWindow::New());
  this->m_interactorAdapter = new QQuickVTKInteractorAdapter(this);

  connect(
    this, SIGNAL(windowChanged(QQuickWindow*)), this, SLOT(handleWindowChanged(QQuickWindow*)));
}

//-------------------------------------------------------------------------------------------------
QQuickVTKRenderWindow::~QQuickVTKRenderWindow() {}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::sync()
{
  if (!this->isVisible())
  {
    return;
  }

  QSize windowSize = window()->size() * window()->devicePixelRatio();
  this->m_renderWindow->SetSize(windowSize.width(), windowSize.height());

  QRectF rect(this->position().x(), this->position().y(), this->width(), this->height());
  rect = mapRectToScene(rect);
  this->m_renderWindow->SetPosition(rect.bottomLeft().x(), rect.bottomLeft().y());
  this->m_renderWindow->SetSize(rect.width(), rect.height());
  // this->m_renderWindow->SetSize(this->width(), this->height());
  // this->m_renderWindow->SetPosition(this->position().x(), this->position().y());

  //// Explicitly set the viewport for each render
  // QRectF rect(0, 0, this->width(), this->height());
  // rect = mapRectToScene(rect);
  // this->setViewport(rect);

  m_interactorAdapter->ProcessEvents(this->m_renderWindow->GetInteractor());
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::paint()
{
  if (!this->isVisible())
  {
    return;
  }

  if (!m_initialized)
  {
    initializeOpenGLFunctions();
    vtkRenderWindowInteractor* interactor = this->m_renderWindow->GetInteractor();
    interactor->Initialize();
    this->m_renderWindow->SetMapped(1);
    this->m_renderWindow->SetIsCurrent(1);

    m_initialized = true;
  }

  glUseProgram(0);
  this->m_renderWindow->OpenGLInit();
  this->m_renderWindow->Render();
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::cleanup() {}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::handleWindowChanged(QQuickWindow* w)
{
  if (this->window())
  {
    QObject::disconnect(
      this->window(), &QQuickWindow::beforeSynchronizing, this, &QQuickVTKRenderWindow::sync);
    QObject::disconnect(
      window(), &QQuickWindow::beforeRendering, this, &QQuickVTKRenderWindow::paint);
    QObject::disconnect(
      window(), &QQuickWindow::sceneGraphInvalidated, this, &QQuickVTKRenderWindow::cleanup);
  }

  this->m_interactorAdapter->setQQuickWindow(w);
  if (w)
  {
    QObject::connect(w, &QQuickWindow::beforeSynchronizing, this, &QQuickVTKRenderWindow::sync,
      Qt::DirectConnection);
    QObject::connect(
      w, &QQuickWindow::beforeRendering, this, &QQuickVTKRenderWindow::paint, Qt::DirectConnection);
    QObject::connect(w, &QQuickWindow::sceneGraphInvalidated, this, &QQuickVTKRenderWindow::cleanup,
      Qt::DirectConnection);
    // Do not clear the scenegraph before the QML rendering
    // to preserve the VTK render
    w->setClearBeforeRendering(false);
    // This allows the cleanup method to be called on the render thread
    w->setPersistentSceneGraph(false);
  }
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::setRenderWindow(vtkRenderWindow* renWin)
{
  auto gwin = vtkGenericOpenGLRenderWindow::SafeDownCast(renWin);
  if (renWin != nullptr && gwin == nullptr)
  {
    qDebug() << "QQuickVTKRenderWindow requires a `vtkGenericOpenGLRenderWindow`. `"
             << renWin->GetClassName() << "` is not supported.";
  }
  this->setRenderWindow(gwin);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::setRenderWindow(vtkGenericOpenGLRenderWindow* renWin)
{
  if (this->m_renderWindow == renWin)
  {
    return;
  }

  this->m_renderWindow = renWin;
  this->m_initialized = false;

  vtkNew<QVTKInteractor> iren;
  iren->SetRenderWindow(this->m_renderWindow);
  // now set the default style
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  iren->SetInteractorStyle(style);
}

//-------------------------------------------------------------------------------------------------
vtkRenderWindow* QQuickVTKRenderWindow::renderWindow() const
{
  return this->m_renderWindow;
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry)
{
  m_interactorAdapter->QueueGeometryChanged(newGeometry, oldGeometry);

  Superclass::geometryChanged(newGeometry, oldGeometry);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::mousePressEvent(QMouseEvent* event)
{
  event->accept();
  m_interactorAdapter->QueueMouseEvent(this, event);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::mouseMoveEvent(QMouseEvent* event)
{
  event->accept();
  m_interactorAdapter->QueueMouseEvent(this, event);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::mouseReleaseEvent(QMouseEvent* event)
{
  event->accept();
  m_interactorAdapter->QueueMouseEvent(this, event);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::wheelEvent(QWheelEvent* event)
{
  event->accept();
  m_interactorAdapter->QueueWheelEvent(this, event);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::hoverMoveEvent(QHoverEvent* event)
{
  event->accept();
  m_interactorAdapter->QueueHoverEvent(this, event);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::keyPressEvent(QKeyEvent* event)
{
  event->accept();
  m_interactorAdapter->QueueKeyEvent(event);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::keyReleaseEvent(QKeyEvent* event)
{
  event->accept();
  m_interactorAdapter->QueueKeyEvent(event);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::focusInEvent(QFocusEvent* event)
{
  event->accept();
  m_interactorAdapter->QueueFocusEvent(event);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderWindow::focusOutEvent(QFocusEvent* event)
{
  event->accept();
  m_interactorAdapter->QueueFocusEvent(event);
}

//-------------------------------------------------------------------------------------------------
bool QQuickVTKRenderWindow::event(QEvent* e)
{
  return Superclass::event(e);
}
