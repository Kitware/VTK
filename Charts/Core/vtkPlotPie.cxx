// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPlotPie.h"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkArrayDispatch.h"
#include "vtkBrush.h"
#include "vtkColorSeries.h"
#include "vtkContext2D.h"
#include "vtkContextMapper2D.h"
#include "vtkDataArrayRange.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPoints2D.h"
#include "vtkRect.h"
#include "vtkTable.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
namespace
{
struct CopyPointsFunctor
{
  template <class TArray>
  void operator()(TArray* array, vtkPoints2D* points)
  {
    vtkIdType n = array->GetNumberOfTuples();
    points->SetNumberOfPoints(n);
    auto a = vtk::DataArrayValueRange<1>(array);
    using ValueType = vtk::GetAPIType<TArray>;
    ValueType sum = 0;
    for (int i = 0; i < n; ++i)
    {
      sum += a[i];
    }
    float* data = vtkAOSDataArrayTemplate<float>::FastDownCast(points->GetData())->GetPointer(0);
    float startAngle = 0.0;

    for (int i = 0; i < n; ++i)
    {
      data[2 * i] = startAngle;
      data[2 * i + 1] = startAngle + ((static_cast<float>(a[i]) / sum) * 360.0);
      startAngle = data[2 * i + 1];
    }
  }
};
}

class vtkPlotPiePrivate
{
public:
  vtkPlotPiePrivate()
  {
    this->CenterX = 0;
    this->CenterY = 0;
    this->Radius = 0;
  }

  float CenterX;
  float CenterY;
  float Radius;
};

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotPie);

//------------------------------------------------------------------------------
vtkPlotPie::vtkPlotPie()
{
  this->ColorSeries = vtkSmartPointer<vtkColorSeries>::New();
  this->Points = nullptr;
  this->Private = new vtkPlotPiePrivate();
  this->Dimensions[0] = this->Dimensions[1] = this->Dimensions[2] = this->Dimensions[3] = 0;
}

//------------------------------------------------------------------------------
vtkPlotPie::~vtkPlotPie()
{
  delete this->Private;
  if (this->Points)
  {
    this->Points->Delete();
    this->Points = nullptr;
  }
  this->Private = nullptr;
}

//------------------------------------------------------------------------------
bool vtkPlotPie::Paint(vtkContext2D* painter)
{
  float* data =
    vtkAOSDataArrayTemplate<float>::FastDownCast(this->Points->GetData())->GetPointer(0);
  vtkNew<vtkBrush> brush;
  painter->ApplyBrush(brush);

  for (int i = 0; i < this->Points->GetNumberOfPoints(); ++i)
  {
    painter->GetBrush()->SetColor(this->ColorSeries->GetColorRepeating(i).GetData());

    if (data[2 * i + 1] != data[2 * i])
    {
      painter->DrawEllipseWedge(this->Private->CenterX, this->Private->CenterY,
        this->Private->Radius, this->Private->Radius, 0.0, 0.0, data[2 * i], data[2 * i + 1]);
    }
  }

  this->PaintChildren(painter);
  return true;
}

//------------------------------------------------------------------------------

bool vtkPlotPie::PaintLegend(vtkContext2D* painter, const vtkRectf& rect, int legendIndex)
{
  if (this->ColorSeries)
    this->Brush->SetColor(this->ColorSeries->GetColorRepeating(legendIndex).GetData());

  painter->ApplyPen(this->Pen);
  painter->ApplyBrush(this->Brush);
  painter->DrawRect(rect[0], rect[1], rect[2], rect[3]);
  return true;
}

//------------------------------------------------------------------------------

void vtkPlotPie::SetDimensions(int arg1, int arg2, int arg3, int arg4)
{
  if (arg1 != this->Dimensions[0] || arg2 != this->Dimensions[1] || arg3 != this->Dimensions[2] ||
    arg4 != this->Dimensions[3])
  {
    this->Dimensions[0] = arg1;
    this->Dimensions[1] = arg2;
    this->Dimensions[2] = arg3;
    this->Dimensions[3] = arg4;

    this->Private->CenterX = this->Dimensions[0] + 0.5 * this->Dimensions[2];
    this->Private->CenterY = this->Dimensions[1] + 0.5 * this->Dimensions[3];
    this->Private->Radius = this->Dimensions[2] < this->Dimensions[3] ? 0.5 * this->Dimensions[2]
                                                                      : 0.5 * this->Dimensions[3];
    this->Modified();
  }
}

void vtkPlotPie::SetDimensions(const int arg[4])
{
  this->SetDimensions(arg[0], arg[1], arg[2], arg[3]);
}

//------------------------------------------------------------------------------
void vtkPlotPie::SetColorSeries(vtkColorSeries* colorSeries)
{
  if (this->ColorSeries == colorSeries)
  {
    return;
  }
  this->ColorSeries = colorSeries;
  this->Modified();
}

//------------------------------------------------------------------------------
vtkColorSeries* vtkPlotPie::GetColorSeries()
{
  return this->ColorSeries;
}

//------------------------------------------------------------------------------
vtkIdType vtkPlotPie::GetNearestPoint(const vtkVector2f& point,
  const vtkVector2f& vtkNotUsed(tolerance), vtkVector2f* value, vtkIdType* vtkNotUsed(segmentId))
{
  float x = point.GetX() - this->Private->CenterX;
  float y = point.GetY() - this->Private->CenterY;

  if (sqrt((x * x) + (y * y)) <= this->Private->Radius)
  {
    float* angles =
      vtkAOSDataArrayTemplate<float>::FastDownCast(this->Points->GetData())->GetPointer(0);
    float pointAngle = vtkMath::DegreesFromRadians(atan2(y, x));
    if (pointAngle < 0)
    {
      pointAngle = 180.0 + (180.0 + pointAngle);
    }
    float* lbound =
      std::lower_bound(angles, angles + (this->Points->GetNumberOfPoints() * 2), pointAngle);
    // Location in the array
    int ret = lbound - angles;
    // There are two of each angle in the array (start,end for each point)
    ret = ret / 2;

    vtkTable* table = this->Data->GetInput();
    vtkDataArray* data = this->Data->GetInputArrayToProcess(0, table);
    value->SetX(ret);
    value->SetY(data->GetTuple1(ret));
    return ret;
  }

  return -1;
}

//------------------------------------------------------------------------------
void vtkPlotPie::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool vtkPlotPie::UpdateCache()
{
  if (!this->Superclass::UpdateCache())
  {
    return false;
  }

  vtkTable* table = this->Data->GetInput();

  // Get the x and y arrays (index 0 and 1 respectively)
  vtkDataArray* data = this->Data->GetInputArrayToProcess(0, table);

  if (!data)
  {
    vtkErrorMacro(<< "No data set (index 0).");
    return false;
  }

  if (!this->Points)
  {
    this->Points = vtkPoints2D::New();
  }

  CopyPointsFunctor functor;
  if (!vtkArrayDispatch::Dispatch::Execute(data, functor, this->Points))
  {
    functor(data, this->Points);
  }

  this->BuildTime.Modified();
  return true;
}
VTK_ABI_NAMESPACE_END
