// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTensorProbeWidget.h"

#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkEllipsoidTensorProbeRepresentation.h"
#include "vtkEvent.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"
#include "vtkWidgetEventTranslator.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTensorProbeWidget);

//------------------------------------------------------------------------------
vtkTensorProbeWidget::vtkTensorProbeWidget()
{
  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent, vtkWidgetEvent::Select,
    this, vtkTensorProbeWidget::SelectAction);

  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
    vtkWidgetEvent::EndSelect, this, vtkTensorProbeWidget::EndSelectAction);

  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtkTensorProbeWidget::MoveAction);

  this->Selected = 0;
}

//------------------------------------------------------------------------------
vtkTensorProbeWidget::~vtkTensorProbeWidget() = default;

//------------------------------------------------------------------------------
void vtkTensorProbeWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkEllipsoidTensorProbeRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkTensorProbeWidget::SelectAction(vtkAbstractWidget* w)
{
  vtkTensorProbeWidget* self = reinterpret_cast<vtkTensorProbeWidget*>(w);

  if (!self->Selected)
  {
    vtkTensorProbeRepresentation* rep =
      reinterpret_cast<vtkTensorProbeRepresentation*>(self->WidgetRep);

    int pos[2];
    self->Interactor->GetEventPosition(pos);

    if (rep->SelectProbe(pos))
    {
      self->LastEventPosition[0] = pos[0];
      self->LastEventPosition[1] = pos[1];
      self->Selected = 1;
      self->EventCallbackCommand->SetAbortFlag(1);
    }
  }
}

//------------------------------------------------------------------------------
void vtkTensorProbeWidget::EndSelectAction(vtkAbstractWidget* w)
{
  vtkTensorProbeWidget* self = reinterpret_cast<vtkTensorProbeWidget*>(w);

  if (self->Selected)
  {
    self->Selected = 0;
    self->EventCallbackCommand->SetAbortFlag(1);
    self->LastEventPosition[0] = -1;
    self->LastEventPosition[1] = -1;
  }
}

//------------------------------------------------------------------------------
void vtkTensorProbeWidget::MoveAction(vtkAbstractWidget* w)
{
  vtkTensorProbeWidget* self = reinterpret_cast<vtkTensorProbeWidget*>(w);

  if (self->Selected)
  {
    vtkTensorProbeRepresentation* rep =
      reinterpret_cast<vtkTensorProbeRepresentation*>(self->WidgetRep);

    int pos[2];
    self->Interactor->GetEventPosition(pos);

    int delta0 = pos[0] - self->LastEventPosition[0];
    int delta1 = pos[1] - self->LastEventPosition[1];
    double motionVector[2] = { static_cast<double>(delta0), static_cast<double>(delta1) };

    self->LastEventPosition[0] = pos[0];
    self->LastEventPosition[1] = pos[1];

    if (rep->Move(motionVector))
    {
      self->EventCallbackCommand->SetAbortFlag(1);
      self->Render();
    }
  }
}

//------------------------------------------------------------------------------
void vtkTensorProbeWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
