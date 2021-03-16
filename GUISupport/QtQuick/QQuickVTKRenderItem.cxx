/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QQuickVTKRenderItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "QQuickVTKRenderItem.h"

// vtk includes
#include "QQuickVTKInteractorAdapter.h"
#include "QQuickVTKRenderWindow.h"
#include "vtkImageData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"

// Qt includes
#include <QQuickWindow>

//-------------------------------------------------------------------------------------------------
QQuickVTKRenderItem::QQuickVTKRenderItem(QQuickItem* parent)
  : Superclass(parent)
{
  // Accept mouse events
  setAcceptHoverEvents(true);
  setAcceptedMouseButtons(Qt::AllButtons);
  setFlag(QQuickItem::ItemIsFocusScope);
  setFlag(QQuickItem::ItemHasContents);

  QObject::connect(
    this, &QQuickItem::windowChanged, this, &QQuickVTKRenderItem::handleWindowChanged);
}

//-------------------------------------------------------------------------------------------------
QQuickVTKRenderItem::~QQuickVTKRenderItem() {}

//-------------------------------------------------------------------------------------------------
QQuickVTKRenderWindow* QQuickVTKRenderItem::renderWindow() const
{
  return this->m_renderWindow;
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderItem::setRenderWindow(QQuickVTKRenderWindow* w)
{
  if (this->m_renderWindow == w)
  {
    return;
  }

  if (this->m_renderWindow)
  {
    this->m_renderWindow->renderWindow()->RemoveRenderer(this->m_renderer);
  }

  this->m_renderWindow = w;
  if (this->m_renderWindow && this->m_renderWindow->renderWindow())
  {
    this->m_renderWindow->renderWindow()->AddRenderer(this->m_renderer);
  }

  this->m_renderWindow->render();
}

//-------------------------------------------------------------------------------------------------
vtkRenderer* QQuickVTKRenderItem::renderer() const
{
  return this->m_renderer;
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderItem::setViewport(const QRectF& rect)
{
  if (!this->m_renderWindow)
  {
    return;
  }
  double viewport[4];
  this->m_renderWindow->mapToViewport(rect, viewport);
  m_renderer->SetViewport(viewport);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderItem::sync()
{
  if (!this->isVisible())
  {
    return;
  }

  // Explicitly set the viewport for each render window
  QRectF rect(0, 0, this->width(), this->height());
  rect = this->mapRectToScene(rect);
  this->setViewport(rect);

  if (!this->m_renderWindow)
  {
    return;
  }

  // Forward the synchronize call to the window
  this->m_renderWindow->sync();
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderItem::paint()
{
  if (!this->isVisible())
  {
    return;
  }
  if (!this->m_renderWindow)
  {
    return;
  }

  // Forward the paint call to the window
  this->m_renderWindow->paint();
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderItem::cleanup()
{
  if (!this->isVisible())
  {
    return;
  }
  if (!this->m_renderWindow)
  {
    return;
  }

  this->m_renderer->ReleaseGraphicsResources(this->m_renderWindow->renderWindow());
  // Forward the cleanup call to the window
  this->m_renderWindow->cleanup();
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderItem::handleWindowChanged(QQuickWindow* w)
{
  if (this->window())
  {
    QObject::disconnect(
      this->window(), &QQuickWindow::beforeSynchronizing, this, &QQuickVTKRenderItem::sync);
    QObject::disconnect(
      window(), &QQuickWindow::beforeRendering, this, &QQuickVTKRenderItem::paint);
    QObject::disconnect(
      window(), &QQuickWindow::sceneGraphInvalidated, this, &QQuickVTKRenderItem::cleanup);
  }

  if (w)
  {
    QObject::connect(w, &QQuickWindow::beforeSynchronizing, this, &QQuickVTKRenderItem::sync,
      Qt::DirectConnection);
    QObject::connect(
      w, &QQuickWindow::beforeRendering, this, &QQuickVTKRenderItem::paint, Qt::DirectConnection);
    QObject::connect(w, &QQuickWindow::sceneGraphInvalidated, this, &QQuickVTKRenderItem::cleanup,
      Qt::DirectConnection);
  }
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderItem::mousePressEvent(QMouseEvent* event)
{
  if (!this->renderWindow())
  {
    return;
  }
  event->accept();
  this->renderWindow()->interactorAdapter()->QueueMouseEvent(this, event);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderItem::mouseMoveEvent(QMouseEvent* event)
{
  if (!this->renderWindow())
  {
    return;
  }
  event->accept();
  this->renderWindow()->interactorAdapter()->QueueMouseEvent(this, event);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderItem::mouseReleaseEvent(QMouseEvent* event)
{
  if (!this->renderWindow())
  {
    return;
  }
  event->accept();
  this->renderWindow()->interactorAdapter()->QueueMouseEvent(this, event);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderItem::wheelEvent(QWheelEvent* event)
{
  if (!this->renderWindow())
  {
    return;
  }
  event->accept();
  this->renderWindow()->interactorAdapter()->QueueWheelEvent(this, event);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderItem::hoverMoveEvent(QHoverEvent* event)
{
  if (!this->renderWindow())
  {
    return;
  }
  event->accept();
  this->renderWindow()->interactorAdapter()->QueueHoverEvent(this, event);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderItem::keyPressEvent(QKeyEvent* event)
{
  if (!this->renderWindow())
  {
    return;
  }
  event->accept();
  this->renderWindow()->interactorAdapter()->QueueKeyEvent(event);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderItem::keyReleaseEvent(QKeyEvent* event)
{
  if (!this->renderWindow())
  {
    return;
  }
  event->accept();
  this->renderWindow()->interactorAdapter()->QueueKeyEvent(event);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderItem::focusInEvent(QFocusEvent* event)
{
  if (!this->renderWindow())
  {
    return;
  }
  event->accept();
  this->renderWindow()->interactorAdapter()->QueueFocusEvent(event);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderItem::focusOutEvent(QFocusEvent* event)
{
  if (!this->renderWindow())
  {
    return;
  }
  event->accept();
  this->renderWindow()->interactorAdapter()->QueueFocusEvent(event);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKRenderItem::geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry)
{
  if (!this->renderWindow())
  {
    return;
  }
  this->renderWindow()->interactorAdapter()->QueueGeometryChanged(newGeometry, oldGeometry);

  Superclass::geometryChanged(newGeometry, oldGeometry);
}

//-------------------------------------------------------------------------------------------------
bool QQuickVTKRenderItem::event(QEvent* e)
{
  return Superclass::event(e);
}

//-------------------------------------------------------------------------------------------------
vtkSmartPointer<vtkImageData> QQuickVTKRenderItem::captureScreenshot()
{
  if (!this->renderWindow())
  {
    return nullptr;
  }
  return this->renderWindow()->captureScreenshot(this->m_renderer->GetViewport());
}
