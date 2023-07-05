// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkProgressBarWidget.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProgressBarRepresentation.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkProgressBarWidget);

//------------------------------------------------------------------------------
vtkProgressBarWidget::vtkProgressBarWidget()
{
  this->Selectable = 0;
}

//------------------------------------------------------------------------------
vtkProgressBarWidget::~vtkProgressBarWidget() = default;

//------------------------------------------------------------------------------
void vtkProgressBarWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkProgressBarRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkProgressBarWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
