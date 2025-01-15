// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class QQuickVTKPinchEvent
 * @brief Custom multitouch pinch event handler for QML PinchHandler
 */

#ifndef QQuickVTKPinchEvent_h
#define QQuickVTKPinchEvent_h

#include "vtkGUISupportQtQuickModule.h" // For export macro

// Qt includes
#include <QEvent>
#include <QPointF>
#include <QVector2D>

VTK_ABI_NAMESPACE_BEGIN

class VTKGUISUPPORTQTQUICK_EXPORT QQuickVTKPinchEvent : public QEvent
{
public:
  enum PinchTypes
  {
    QQUICKVTK_TRANSLATE = 0,
    QQUICKVTK_SCALE,
    QQUICKVTK_ROTATE,
    QQUICKVTK_NONE
  };

  static const QEvent::Type QQuickVTKPinch;

  explicit QQuickVTKPinchEvent(QEvent::Type type, PinchTypes pinchType, const QPointF& position,
    const QVector2D& translation = QVector2D(0, 0), double scale = 1.0, double angle = 0.0);

  ///@{
  /**
   * Set/Get the pinch event type
   */
  virtual void setPinchEventType(PinchTypes t);
  virtual PinchTypes pinchEventType();
  ///@}

  ///@{
  /**
   * Set/get position
   */
  virtual void setPosition(QPointF pos);
  virtual QPointF position();
  ///@}

  ///@{
  /**
   * Set/get translation
   */
  virtual void setTranslation(QVector2D trans);
  virtual QVector2D translation();
  ///@}

  ///@{
  /**
   * Set/get scale
   */
  virtual void setScale(double scale);
  virtual double scale();
  ///@}

  ///@{
  /**
   * Set/get rotation angle (in degrees)
   */
  virtual void setAngle(double angle);
  virtual double angle();
  ///@}

private:
  PinchTypes m_pinchEventType = QQUICKVTK_NONE;
  QPointF m_position;
  QVector2D m_translation;
  double m_scale = 1.0;
  double m_angle = 0.0;
};

VTK_ABI_NAMESPACE_END

#endif // end QQuickVTKPinchEvent_h
