// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class QQuickVTKInteractorAdapter
 * @brief Interactor that handles relaying custom QML handler events to VTK
 */

#ifndef QQUICK_VTK_INTERACTOR_ADAPTER_H
#define QQUICK_VTK_INTERACTOR_ADAPTER_H

#include "QVTKInteractorAdapter.h"
#include "vtkGUISupportQtQuickModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN

// Forward declarations
class vtkRenderWindowInteractor;

/**
 * \class QQuickVTKInteractorAdapter
 * \brief
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
  virtual bool ProcessEvent(QEvent* e, vtkRenderWindowInteractor* iren) override;

private:
  Q_DISABLE_COPY(QQuickVTKInteractorAdapter)
};

VTK_ABI_NAMESPACE_END

#endif // end QQUICK_VTK_INTERACTOR_ADAPTER_H
// VTK-HeaderTest-Exclude: QQuickVTKInteractorAdapter.h
