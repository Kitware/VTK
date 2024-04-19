// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLogoWidget.h"
#include "vtkCallbackCommand.h"
#include "vtkLogoRepresentation.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLogoWidget);

//------------------------------------------------------------------------------
vtkLogoWidget::vtkLogoWidget()
{
  this->Selectable = 0;
}

//------------------------------------------------------------------------------
vtkLogoWidget::~vtkLogoWidget() = default;

//------------------------------------------------------------------------------
void vtkLogoWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkLogoRepresentation::New();
  }
}

//------------------------------------------------------------------------------
void vtkLogoWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
