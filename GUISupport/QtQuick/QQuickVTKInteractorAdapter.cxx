/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QQuickVTKInteractorAdapter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "QQuickVTKInteractorAdapter.h"

// Qt includes
#include <QEvent>
#include <QQuickItem>
#include <QQuickWindow>

// VTK includes
#include "vtkRenderWindowInteractor.h"

//-------------------------------------------------------------------------------------------------
QQuickVTKInteractorAdapter::QQuickVTKInteractorAdapter(QObject* parent)
  : Superclass(parent)
{
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKInteractorAdapter::setQQuickWindow(QQuickWindow* win)
{
  m_qwindow = win;
}

//-------------------------------------------------------------------------------------------------
QPointF QQuickVTKInteractorAdapter::mapEventPosition(QQuickItem* item, const QPointF& localPos)
{
  // Account for the difference in coordinate reference systems.
  // Qt uses quadrant IV and VTK uses quadrant I. So the point should be
  // translated to the right position along Y axis.
  return item->mapToScene(localPos);
}

//-------------------------------------------------------------------------------------------------
QPointF QQuickVTKInteractorAdapter::mapEventPositionFlipY(QQuickItem* item, const QPointF& localPos)
{
  QPointF mappedPos = QQuickVTKInteractorAdapter::mapEventPosition(item, localPos);
  mappedPos.setY(item->window()->height() - mappedPos.y() + 1);
  return mappedPos;
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKInteractorAdapter::QueueHoverEvent(QQuickItem* item, QHoverEvent* e)
{
  QHoverEvent* newEvent = new QHoverEvent(e->type(), this->mapEventPosition(item, e->posF()),
    this->mapEventPosition(item, e->oldPosF()), e->modifiers());
  QueueEvent(newEvent);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKInteractorAdapter::QueueKeyEvent(QQuickItem* item, QKeyEvent* e)
{
  Q_UNUSED(item);
  QKeyEvent* newEvent = new QKeyEvent(e->type(), e->key(), e->modifiers(), e->nativeScanCode(),
    e->nativeVirtualKey(), e->nativeModifiers(), e->text(), e->isAutoRepeat(), e->count());
  QueueEvent(newEvent);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKInteractorAdapter::QueueFocusEvent(QQuickItem* item, QFocusEvent* e)
{
  Q_UNUSED(item);
  QFocusEvent* newEvent = new QFocusEvent(e->type(), e->reason());
  QueueEvent(newEvent);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKInteractorAdapter::QueueMouseEvent(QQuickItem* item, QMouseEvent* e)
{
  QMouseEvent* newEvent = new QMouseEvent(e->type(), this->mapEventPosition(item, e->localPos()),
    this->mapEventPosition(item, e->windowPos()), this->mapEventPosition(item, e->screenPos()),
    e->button(), e->buttons(), e->modifiers());
  QueueEvent(newEvent);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKInteractorAdapter::QueueGeometryChanged(
  const QRectF& newGeometry, const QRectF& oldGeometry)
{
  QResizeEvent* newEvent =
    new QResizeEvent(newGeometry.size().toSize(), oldGeometry.size().toSize());
  QueueEvent(newEvent);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKInteractorAdapter::QueueWheelEvent(QQuickItem* item, QWheelEvent* e)
{
  QPointF p, gp;
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
  p = e->position();
  gp = e->globalPosition();
#else
  p = e->posF();
  gp = e->globalPosF();
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
  QWheelEvent* newEvent = new QWheelEvent(this->mapEventPosition(item, p),
    this->mapEventPosition(item, gp), e->pixelDelta(), e->angleDelta(), e->buttons(),
    e->modifiers(), e->phase(), e->inverted(), e->source());
#else
  QWheelEvent* newEvent = new QWheelEvent(this->mapEventPosition(item, p),
    this->mapEventPosition(item, gp), e->pixelDelta(), e->angleDelta(), //
    0, Qt::Horizontal, // Qt4 compatibility arguments
    e->buttons(), e->modifiers(), e->phase(), e->source(), e->inverted());
#endif
  QueueEvent(newEvent);
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKInteractorAdapter::QueueEvent(QEvent* e)
{
  m_queuedEvents << e;
  if (m_qwindow)
  {
    m_qwindow->update();
  }
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKInteractorAdapter::ProcessEvents(vtkRenderWindowInteractor* interactor)
{
  if (interactor)
  {
    for (QEvent* e : this->m_queuedEvents)
    {
      ProcessEvent(e, interactor);
    }
    qDeleteAll(m_queuedEvents);
    m_queuedEvents.clear();
  }
}
