// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCameraWidget.h"
#include "vtkCallbackCommand.h"
#include "vtkCameraInterpolator.h"
#include "vtkCameraRepresentation.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCameraWidget);

//------------------------------------------------------------------------------
vtkCameraWidget::vtkCameraWidget() = default;

//------------------------------------------------------------------------------
vtkCameraWidget::~vtkCameraWidget() = default;

//------------------------------------------------------------------------------
void vtkCameraWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkCameraRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkCameraWidget::SelectRegion(double eventPos[2])
{
  if (!this->WidgetRep)
  {
    return;
  }

  double x = eventPos[0];
  if (x < 0.3333)
  {
    reinterpret_cast<vtkCameraRepresentation*>(this->WidgetRep)->AddCameraToPath();
  }
  else if (x < 0.666667)
  {
    reinterpret_cast<vtkCameraRepresentation*>(this->WidgetRep)->AnimatePath(this->Interactor);
  }
  else if (x < 1.0)
  {
    reinterpret_cast<vtkCameraRepresentation*>(this->WidgetRep)->InitializePath();
  }

  this->Superclass::SelectRegion(eventPos);
}

//------------------------------------------------------------------------------
void vtkCameraWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
