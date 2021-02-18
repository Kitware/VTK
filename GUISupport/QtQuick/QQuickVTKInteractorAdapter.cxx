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
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>

//-----------------------------------------------------------------------------
QQuickVTKInteractorAdapter::QQuickVTKInteractorAdapter(QObject* parent)
  : Superclass(parent)
{
}

//-----------------------------------------------------------------------------
void QQuickVTKInteractorAdapter::setQQuickWindow(QQuickWindow* win)
{
  m_qwindow = win;
}

//-----------------------------------------------------------------------------
QPointF QQuickVTKInteractorAdapter::mapAndFlipEventPosition(
  QQuickItem* item, vtkRenderWindowInteractor* interactor, const QPointF& localPos)
{
  QPointF eventPos = QQuickVTKInteractorAdapter::mapEventPosition(item, localPos);
  eventPos.setY(interactor->GetSize()[1] - eventPos.y() - 1);
  return eventPos;
}

//-----------------------------------------------------------------------------
QPointF QQuickVTKInteractorAdapter::mapEventPosition(QQuickItem* item, const QPointF& localPos)
{
  // Account for the difference in coordinate reference systems.
  // Qt uses quadrant IV and VTK uses quadrant I. So the point should be
  // translated to the right position along Y axis.
  QPointF itemOriginInScene = item->mapToItem(NULL, QPointF());
  QPointF itemHeightInScene = item->mapToScene(QPointF(0, item->height()));
  QPointF eventPos = item->mapToScene(localPos);
  eventPos.ry() -= (item->window()->height() - itemHeightInScene.y()) + itemOriginInScene.y();
  return eventPos;
}

//-----------------------------------------------------------------------------
void QQuickVTKInteractorAdapter::QueueHoverEvent(QQuickItem* item, QHoverEvent* e)
{
  // Treat Qt hover event as mouse move event. Actual Qt mouse move events occur
  // only when the mouse button is pressed.
  QPointF pos = mapEventPosition(item, e->posF());
  QPointF oldPos = mapEventPosition(item, e->oldPosF());

  QHoverEvent* newEvent = new QHoverEvent(e->type(), pos, oldPos, e->modifiers());
  QueueEvent(newEvent);
}

//-----------------------------------------------------------------------------
void QQuickVTKInteractorAdapter::QueueKeyEvent(QKeyEvent* e)
{
  QKeyEvent* newEvent = new QKeyEvent(e->type(), e->key(), e->modifiers(), e->nativeScanCode(),
    e->nativeVirtualKey(), e->nativeModifiers(), e->text(), e->isAutoRepeat(), e->count());
  QueueEvent(newEvent);
}

//-----------------------------------------------------------------------------
void QQuickVTKInteractorAdapter::QueueFocusEvent(QFocusEvent* e)
{
  QFocusEvent* newEvent = new QFocusEvent(e->type(), e->reason());
  QueueEvent(newEvent);
}

//-----------------------------------------------------------------------------
void QQuickVTKInteractorAdapter::QueueMouseEvent(QQuickItem* item, QMouseEvent* e)
{
  QPointF pos = mapEventPosition(item, e->pos());

  QMouseEvent* newEvent = new QMouseEvent(
    e->type(), pos, e->windowPos(), e->screenPos(), e->button(), e->buttons(), e->modifiers());
  QueueEvent(newEvent);
}

//-----------------------------------------------------------------------------
void QQuickVTKInteractorAdapter::QueueGeometryChanged(
  const QRectF& newGeometry, const QRectF& oldGeometry)
{
  QResizeEvent* newEvent =
    new QResizeEvent(newGeometry.size().toSize(), oldGeometry.size().toSize());
  QueueEvent(newEvent);
}

//-----------------------------------------------------------------------------
void QQuickVTKInteractorAdapter::QueueWheelEvent(QQuickItem* item, QWheelEvent* e)
{
  QPointF pos = mapEventPosition(item, e->position());
  QPointF globalPos = mapEventPosition(item, e->globalPosition());

  QWheelEvent* newEvent = new QWheelEvent(pos, globalPos, e->pixelDelta(), e->angleDelta(),
    e->buttons(), e->modifiers(), e->phase(), e->inverted(), e->source());
  QueueEvent(newEvent);
}

//-----------------------------------------------------------------------------
void QQuickVTKInteractorAdapter::QueueEnterEvent(QEnterEvent* e)
{
  QEnterEvent* newEvent = new QEnterEvent(e->localPos(), e->windowPos(), e->screenPos());
  QueueEvent(newEvent);
}

//-----------------------------------------------------------------------------
void QQuickVTKInteractorAdapter::QueueEvent(QEvent* e)
{
  m_queuedEvents << e;
  if (m_qwindow)
  {
    m_qwindow->update();
  }
}

//-----------------------------------------------------------------------------
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
