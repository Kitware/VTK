// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPlaybackWidget.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPlaybackRepresentation.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPlaybackWidget);

//------------------------------------------------------------------------------
vtkPlaybackWidget::vtkPlaybackWidget() = default;

//------------------------------------------------------------------------------
vtkPlaybackWidget::~vtkPlaybackWidget() = default;

//------------------------------------------------------------------------------
void vtkPlaybackWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkPlaybackRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkPlaybackWidget::SelectRegion(double eventPos[2])
{
  if (!this->WidgetRep)
  {
    return;
  }

  double x = eventPos[0];
  if (x < 0.16667)
  {
    reinterpret_cast<vtkPlaybackRepresentation*>(this->WidgetRep)->JumpToBeginning();
  }
  else if (x <= 0.333333)
  {
    reinterpret_cast<vtkPlaybackRepresentation*>(this->WidgetRep)->BackwardOneFrame();
  }
  else if (x <= 0.500000)
  {
    reinterpret_cast<vtkPlaybackRepresentation*>(this->WidgetRep)->Stop();
  }
  else if (x < 0.666667)
  {
    reinterpret_cast<vtkPlaybackRepresentation*>(this->WidgetRep)->Play();
  }
  else if (x <= 0.833333)
  {
    reinterpret_cast<vtkPlaybackRepresentation*>(this->WidgetRep)->ForwardOneFrame();
  }
  else if (x <= 1.00000)
  {
    reinterpret_cast<vtkPlaybackRepresentation*>(this->WidgetRep)->JumpToEnd();
  }
}

//------------------------------------------------------------------------------
void vtkPlaybackWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
