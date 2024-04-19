// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtk3DCursorWidget.h"
#include "vtk3DCursorRepresentation.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtk3DCursorWidget);

//------------------------------------------------------------------------------
vtk3DCursorWidget::vtk3DCursorWidget()
{
  // This is the "main" callback of this class. Set as passive observer to have top priority
  // and ignore focus. This allows to interact with other widgets keeping the cursor position
  // updated.
  this->EventCallbackCommand->SetPassiveObserver(true);

  // Define the events-callback mapping for this widget
  this->CallbackMapper->SetCallbackMethod(
    vtkCommand::MouseMoveEvent, vtkWidgetEvent::Move, this, vtk3DCursorWidget::MoveAction);
}

//------------------------------------------------------------------------------
void vtk3DCursorWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtk3DCursorRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtk3DCursorWidget::MoveAction(vtkAbstractWidget* w)
{
  vtk3DCursorWidget* self = reinterpret_cast<vtk3DCursorWidget*>(w);
  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];

  double eventPosition[2];
  eventPosition[0] = static_cast<double>(X);
  eventPosition[1] = static_cast<double>(Y);
  self->WidgetRep->WidgetInteraction(eventPosition);

  self->Render();
}

//------------------------------------------------------------------------------
void vtk3DCursorWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
