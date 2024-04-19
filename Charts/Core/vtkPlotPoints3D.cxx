// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPlotPoints3D.h"
#include "vtkChartXYZ.h"
#include "vtkContext2D.h"
#include "vtkContext3D.h"
#include "vtkContextDevice3D.h"
#include "vtkContextScene.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPlot.h"
#include "vtkPoints.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPlotPoints3D);

//------------------------------------------------------------------------------
vtkPlotPoints3D::vtkPlotPoints3D()
{
  this->Pen->SetWidth(5);
  this->Pen->SetColor(0, 0, 0, 255);
  this->SelectionPen->SetWidth(7);
  this->SelectedPoints->SetDataType(this->Points->GetDataType());
}

//------------------------------------------------------------------------------
vtkPlotPoints3D::~vtkPlotPoints3D() = default;

//------------------------------------------------------------------------------
void vtkPlotPoints3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkPlotPoints3D::Paint(vtkContext2D* painter)
{
  const vtkIdType numPoints = this->Points->GetNumberOfPoints();
  if (!this->Visible || !numPoints)
  {
    return false;
  }

  // Get the 3D context.
  vtkContext3D* context = painter->GetContext3D();

  if (!context)
  {
    return false;
  }

  this->Update();

  if (numPoints > 0)
  {

    // Draw the points in 3d.
    context->ApplyPen(this->Pen);
    const std::uintptr_t cacheIdentifier = reinterpret_cast<std::uintptr_t>(this);
    if (this->NumberOfComponents == 0)
    {
      context->DrawPoints(this->Points->GetData(), nullptr, cacheIdentifier);
    }
    else
    {
      context->DrawPoints(this->Points->GetData(), this->Colors, cacheIdentifier);
    }
  }

  // Now add some decorations for our selected points...
  if (this->Selection && this->Selection->GetNumberOfTuples())
  {
    if (this->Selection->GetMTime() > this->SelectedPointsBuildTime)
    {
      const vtkIdType nSelected = this->Selection->GetNumberOfTuples();
      this->SelectedPoints->SetNumberOfPoints(nSelected);
      vtkPlot::FilterSelectedPoints(
        this->Points->GetData(), this->SelectedPoints->GetData(), this->Selection);
      this->SelectedPointsBuildTime.Modified();
    }

    const std::uintptr_t cacheIdentifier =
      reinterpret_cast<std::uintptr_t>(this->SelectedPoints.Get());
    // Now to render the selected points.
    if (this->SelectedPoints->GetNumberOfPoints() > 0)
    {
      context->ApplyPen(this->SelectionPen);
      context->DrawPoints(this->SelectedPoints->GetData(), nullptr, cacheIdentifier);
    }
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkPlotPoints3D::ReleaseGraphicsCache()
{
  // Superclass clears cache related to cacheIdentifier=static_cast<uintptr_t>(this)
  // but not SelectedPoints.
  this->Superclass::ReleaseGraphicsCache();
  // Removes cache related to SelectedPoints.
  if (auto lastPainter = this->Scene->GetLastPainter())
  {
    if (auto ctx3D = lastPainter->GetContext3D())
    {
      if (auto device3D = ctx3D->GetDevice())
      {
        device3D->ReleaseCache(reinterpret_cast<std::uintptr_t>(this->SelectedPoints.Get()));
      }
    }
  }
}

VTK_ABI_NAMESPACE_END
