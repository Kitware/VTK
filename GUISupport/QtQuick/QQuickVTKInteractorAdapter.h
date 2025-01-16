// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class QQuickVTKInteractorAdapter
 * @brief Interactor that handles relaying custom QML handler events to VTK
 */

#ifndef QQuickVTKInteractorAdapter_h
#define QQuickVTKInteractorAdapter_h

#include "QVTKInteractorAdapter.h"
#include "vtkGUISupportQtQuickModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

// Forward declarations
class vtkRenderWindowInteractor;

/**
 * \class QQuickVTKInteractorAdapter
 * \brief Interactor that handles relaying custom QML handler events to VTK
 *
 * Handles translating the QQuickVTKPinchEvent::QQuickVTKPinch events to VTK events such that:
 *
 *   - QQUICKVTK_TRANSLATE ->   vtkCommand::PanEvent
 *   - QQUICKVTK_SCALE     ->   vtkCommand::PinchEvent
 *   - QQUICKVTK_ROTATE    ->   vtkCommand::RotateEvent
 *
 *  For more information on QQuickVTKPinchEvent, refer to QQuickVTKItem documentation.
 *
 *  \sa QQuickVTKItem::pinchHandlerTranslate, QQuickVTKItem::pinchHandlerScale,
 *  QQuickVTKItem::pinchHandlerRotate,
 *  \sa QQuickVTKPinchEvent
 */
class VTKGUISUPPORTQTQUICK_EXPORT QQuickVTKInteractorAdapter : public QVTKInteractorAdapter
{
  Q_OBJECT
  typedef QVTKInteractorAdapter Superclass;

public:
  QQuickVTKInteractorAdapter(QObject* parent = nullptr);

  // Description:
  // Process a QEvent and send it to the interactor
  // returns whether the event was recognized and processed
  bool ProcessEvent(QEvent* e, vtkRenderWindowInteractor* iren) override;

private:
  Q_DISABLE_COPY(QQuickVTKInteractorAdapter)
};

VTK_ABI_NAMESPACE_END

#endif // QQuickVTKInteractorAdapter_h
