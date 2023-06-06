/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlotPoints3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPlotPoints3D.h"
#include "vtkArrayDispatch.h"
#include "vtkArrayDispatch.txx"
#include "vtkChartXYZ.h"
#include "vtkContext2D.h"
#include "vtkContext3D.h"
#include "vtkDataArrayMeta.h"
#include "vtkDataArrayRange.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
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

namespace
{
struct FilterSelectedPoints
{
  template <typename ArrayType1, typename ArrayType2>
  void operator()(ArrayType1* points, ArrayType2* selectedPoints, vtkIdTypeArray* selectedIds)
  {
    using ValueT = vtk::GetAPIType<ArrayType1>;
    auto pointsRange = vtk::DataArrayTupleRange(points);
    auto selectedPointsRange = vtk::DataArrayTupleRange(selectedPoints);
    const vtkIdType nSelected = selectedIds->GetNumberOfTuples();
    const vtkIdType* ids = selectedIds->GetPointer(0);
    assert(pointsRange.GetTupleSize() == 3);
    assert(selectedPointsRange.GetTupleSize() == 3);
    for (vtkIdType i = 0; i < nSelected; ++i)
    {
      const vtkIdType& id = ids[i];
      std::copy(pointsRange[id].cbegin(), pointsRange[id].cend(), selectedPointsRange[i].begin());
    }
  }
};
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
  const std::uintptr_t cacheIdentifier = reinterpret_cast<std::uintptr_t>(this);

  if (numPoints > 0)
  {

    // Draw the points in 3d.
    context->ApplyPen(this->Pen);
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
    if (this->Selection->GetMTime() > this->SelectedPointsBuildTime ||
      this->GetMTime() > this->SelectedPointsBuildTime)
    {
      const vtkIdType nSelected = this->Selection->GetNumberOfTuples();
      this->SelectedPoints->SetNumberOfPoints(nSelected);
      using DispatchT = vtkArrayDispatch::Dispatch2BySameValueType<vtkArrayDispatch::Reals>;
      ::FilterSelectedPoints worker;
      if (!DispatchT::Execute(this->Points->GetData(), this->SelectedPoints->GetData(), worker,
            this->Selection.Get()))
      {
        worker(this->Points->GetData(), this->SelectedPoints->GetData(), this->Selection.Get());
      }
      this->SelectedPointsBuildTime.Modified();
    }

    // Now to render the selected points.
    if (this->SelectedPoints->GetNumberOfPoints() > 0)
    {
      context->ApplyPen(this->SelectionPen);
      context->DrawPoints(this->SelectedPoints->GetData(), nullptr, cacheIdentifier);
    }
  }

  return true;
}
VTK_ABI_NAMESPACE_END
