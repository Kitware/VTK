// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// VTK includes
#include "QQuickVTKPinchEvent.h"

// Qt includes
#include <QEvent>

VTK_ABI_NAMESPACE_BEGIN

//-------------------------------------------------------------------------------------------------
const QEvent::Type QQuickVTKPinchEvent::QQuickVTKPinch =
  static_cast<QEvent::Type>(QEvent::registerEventType());

//-------------------------------------------------------------------------------------------------
QQuickVTKPinchEvent::QQuickVTKPinchEvent(QEvent::Type type, PinchTypes pinchType,
  const QPointF& pos, const QVector2D& trans, double scale, double angle)
  : QEvent(type)
  , m_pinchEventType(pinchType)
  , m_position(pos)
  , m_translation(trans)
  , m_scale(scale)
  , m_angle(angle)
{
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKPinchEvent::setPinchEventType(QQuickVTKPinchEvent::PinchTypes typ)
{
  if (typ < QQUICKVTK_TRANSLATE || typ > QQUICKVTK_NONE)
  {
    return;
  }
  this->m_pinchEventType = typ;
}

//-------------------------------------------------------------------------------------------------
QQuickVTKPinchEvent::PinchTypes QQuickVTKPinchEvent::pinchEventType()
{
  return this->m_pinchEventType;
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKPinchEvent::setPosition(QPointF pos)
{
  this->m_position = pos;
}

//-------------------------------------------------------------------------------------------------
QPointF QQuickVTKPinchEvent::position()
{
  return this->m_position;
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKPinchEvent::setTranslation(QVector2D trans)
{
  this->m_translation = trans;
}

//-------------------------------------------------------------------------------------------------
QVector2D QQuickVTKPinchEvent::translation()
{
  return this->m_translation;
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKPinchEvent::setScale(double scale)
{
  this->m_scale = scale;
}

//-------------------------------------------------------------------------------------------------
double QQuickVTKPinchEvent::scale()
{
  return this->m_scale;
}

//-------------------------------------------------------------------------------------------------
void QQuickVTKPinchEvent::setAngle(double angle)
{
  this->m_angle = angle;
}

//-------------------------------------------------------------------------------------------------
double QQuickVTKPinchEvent::angle()
{
  return this->m_angle;
}

VTK_ABI_NAMESPACE_END
