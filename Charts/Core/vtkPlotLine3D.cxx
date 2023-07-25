// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPlotLine3D.h"

#include "vtkAbstractArray.h"
#include "vtkContext2D.h"
#include "vtkContext3D.h"
#include "vtkFloatArray.h"
#include "vtkPen.h"
#include "vtkPoints.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPlotLine3D);

//------------------------------------------------------------------------------
vtkPlotLine3D::vtkPlotLine3D() = default;

//------------------------------------------------------------------------------
vtkPlotLine3D::~vtkPlotLine3D() = default;

//------------------------------------------------------------------------------
bool vtkPlotLine3D::Paint(vtkContext2D* painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  vtkDebugMacro(<< "Paint event called in vtkPlotLine3D.");

  if (!this->Visible || !this->Points->GetNumberOfPoints())
  {
    return false;
  }

  // Get the 3D context.
  vtkContext3D* context = painter->GetContext3D();
  if (context == nullptr)
  {
    return false;
  }

  // Draw the line between the points
  context->ApplyPen(this->Pen);
  float* points = vtkArrayDownCast<vtkFloatArray>(this->Points->GetData())->GetPointer(0);
  context->DrawPoly(points, this->Points->GetNumberOfPoints());

  return this->vtkPlotPoints3D::Paint(painter);
}

//------------------------------------------------------------------------------
void vtkPlotLine3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
