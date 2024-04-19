// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// this class is deprecated, don't warn about deprecated classes it uses
#define VTK_DEPRECATION_LEVEL 0

#include "QQuickVTKInteractorAdapter.h"

// Qt includes
#include <QEvent>
#include <QQuickItem>
#include <QQuickWindow>

// VTK includes
#include "vtkRenderWindowInteractor.h"

//-------------------------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QPointF posf = e->posF();
#else
  QPointF posf = e->position();
#endif
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
  QHoverEvent* newEvent = new QHoverEvent(e->type(), this->mapEventPosition(item, posf),
    this->mapEventPosition(item, e->oldPosF()), e->modifiers());
#else
  QHoverEvent* newEvent = new QHoverEvent(e->type(), this->mapEventPosition(item, posf),
    e->globalPosition(), this->mapEventPosition(item, e->oldPosF()), e->modifiers());
#endif
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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QPointF localpos = e->localPos();
  QPointF windowpos = e->windowPos();
  QPointF screenpos = e->screenPos();
#else
  QPointF localpos = e->position();
  QPointF windowpos = e->scenePosition();
  QPointF screenpos = e->globalPosition();
#endif
  QMouseEvent* newEvent = new QMouseEvent(e->type(), this->mapEventPosition(item, localpos),
    this->mapEventPosition(item, windowpos), this->mapEventPosition(item, screenpos), e->button(),
    e->buttons(), e->modifiers());
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
VTK_ABI_NAMESPACE_END
