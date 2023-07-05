// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkContinuousValueWidgetRepresentation.h"
#include "vtkCommand.h"
#include "vtkEvent.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkWindow.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkContinuousValueWidgetRepresentation::vtkContinuousValueWidgetRepresentation()
{
  this->Value = 0;
}

//------------------------------------------------------------------------------
vtkContinuousValueWidgetRepresentation::~vtkContinuousValueWidgetRepresentation() = default;

//------------------------------------------------------------------------------
void vtkContinuousValueWidgetRepresentation::PlaceWidget(double* vtkNotUsed(bds[6]))
{
  // Position the handles at the end of the lines
  this->BuildRepresentation();
}

void vtkContinuousValueWidgetRepresentation::SetValue(double) {}

//------------------------------------------------------------------------------
void vtkContinuousValueWidgetRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  // Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Value: " << this->GetValue() << "\n";
}
VTK_ABI_NAMESPACE_END
