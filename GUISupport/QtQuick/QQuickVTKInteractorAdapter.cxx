// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// VTK includes
#include "QQuickVTKInteractorAdapter.h"
#include "QQuickVTKPinchEvent.h"
#include "QVTKInteractorAdapter.h"
#include "QVTKInteractorInternal.h"
#include "vtkRenderWindowInteractor.h"

VTK_ABI_NAMESPACE_BEGIN

//-------------------------------------------------------------------------------------------------
QQuickVTKInteractorAdapter::QQuickVTKInteractorAdapter(QObject* parentObject)
  : Superclass(parentObject)
{
}

//-------------------------------------------------------------------------------------------------
bool QQuickVTKInteractorAdapter::ProcessEvent(QEvent* e, vtkRenderWindowInteractor* iren)
{
  if (iren == nullptr || e == nullptr)
    return false;

  const QEvent::Type t = e->type();

  if (t == QQuickVTKPinchEvent::QQuickVTKPinch)
  {
    QQuickVTKPinchEvent* e2 = static_cast<QQuickVTKPinchEvent*>(e);
    if (e2->pinchEventType() != QQuickVTKPinchEvent::QQUICKVTK_NONE)
    {
      iren->SetEventInformationFlipY(e2->position().x() * this->DevicePixelRatio +
          QVTKInteractorAdapter::DevicePixelRatioTolerance,
        e2->position().y() * this->DevicePixelRatio +
          QVTKInteractorAdapter::DevicePixelRatioTolerance);
    }
    switch (e2->pinchEventType())
    {
      case QQuickVTKPinchEvent::QQUICKVTK_TRANSLATE:
      {
        double trans[2] = { e2->translation().x() * this->DevicePixelRatio +
            QVTKInteractorAdapter::DevicePixelRatioTolerance,
          -1.0 * e2->translation().y() * this->DevicePixelRatio +
            QVTKInteractorAdapter::DevicePixelRatioTolerance };
        iren->SetTranslation(trans);
        iren->InvokeEvent(vtkCommand::StartPanEvent, nullptr);
        iren->InvokeEvent(vtkCommand::PanEvent, nullptr);
        iren->InvokeEvent(vtkCommand::EndPanEvent, nullptr);
        break;
      }
      case QQuickVTKPinchEvent::QQUICKVTK_SCALE:
      {
        iren->SetScale(1.0);
        iren->SetScale(e2->scale());
        iren->InvokeEvent(vtkCommand::StartPinchEvent, nullptr);
        iren->InvokeEvent(vtkCommand::PinchEvent, nullptr);
        iren->InvokeEvent(vtkCommand::EndPinchEvent, nullptr);
        break;
      }
      case QQuickVTKPinchEvent::QQUICKVTK_ROTATE:
      {
        iren->SetRotation(0.0);
        iren->SetRotation(-1.0 * e2->angle());
        iren->InvokeEvent(vtkCommand::StartRotateEvent, nullptr);
        iren->InvokeEvent(vtkCommand::RotateEvent, nullptr);
        iren->InvokeEvent(vtkCommand::EndRotateEvent, nullptr);
        break;
      }
      default:
      {
        return this->Superclass::ProcessEvent(e, iren);
      }
    }
    e2->accept();
    return true;
  }
  else
  {
    return this->Superclass::ProcessEvent(e, iren);
  }
}

VTK_ABI_NAMESPACE_END
