// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPlotRadViz.h"

#include "vtkAOSDataArrayTemplate.h"
#include "vtkArrayDispatch.h"
#include "vtkBrush.h"
#include "vtkCellData.h"
#include "vtkContext2D.h"
#include "vtkContextMapper2D.h"
#include "vtkDataArrayRange.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkMultiVariateToRGBA.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"
#include "vtkRenderingCoreEnums.h"
#include "vtkTable.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotRadViz);

class vtkPlotRadViz::vtkInternals
{
public:
  vtkInternals()
  {
    this->CenterX = 0;
    this->CenterY = 0;
    this->Radius = 0;
  }
  /**
   * Used to translate and scale GBCs
   */
  template <class TArray>
  void operator()(TArray* inputArray, vtkPoints2D* points)
  {
    auto input = vtk::DataArrayTupleRange(inputArray);
    auto output = vtkAOSDataArrayTemplate<float>::FastDownCast(points->GetData())->GetPointer(0);
    points->SetNumberOfPoints(input.size());
    for (int i = 0; i < input.size(); ++i)
    {
      output[2 * i] = this->CenterX + input[i][0] * this->Radius;
      output[2 * i + 1] = this->CenterY + input[i][1] * this->Radius;
    }
  }

  float CenterX;
  float CenterY;
  float Radius;
};

//------------------------------------------------------------------------------
vtkPlotRadViz::vtkPlotRadViz()
  : Internals(new vtkInternals())
{
  this->Dimensions[0] = this->Dimensions[1] = this->Dimensions[2] = this->Dimensions[3] = 0;
}

void vtkPlotRadViz::SetInputData(vtkDataSet* dataset)
{
  this->Data->SetInputDataObject(0, dataset);
}

//------------------------------------------------------------------------------
bool vtkPlotRadViz::Paint(vtkContext2D* painter)
{
  const int segmentCount = 100;

  vtkNew<vtkMultiVariateToRGBA> computeGBC;
  computeGBC->SetInputDataObject(0, this->Data->GetInputDataObject(0, 0));
  computeGBC->SetStoreGBC(true);
  computeGBC->SetStoreRGBA(false);
  computeGBC->Update();
  vtkTable* table = vtkTable::SafeDownCast(computeGBC->GetOutputDataObject(0));
  vtkDataSet* dataset = vtkDataSet::SafeDownCast(computeGBC->GetOutputDataObject(0));
  vtkFloatArray* a = nullptr;
  if (table)
  {
    a = vtkFloatArray::FastDownCast(table->GetColumnByName("gbc"));
    if (!a)
    {
      vtkErrorMacro("Cannot find gbc array");
      return false;
    }
  }
  else if (dataset)
  {
    vtkDataSetAttributes* pd = dataset->GetPointData();
    vtkDataSetAttributes* cd = dataset->GetCellData();
    a = vtkFloatArray::FastDownCast(pd->GetArray("gbc"));
    if (!a)
    {
      a = vtkFloatArray::FastDownCast(cd->GetArray("gbc"));
    }
    if (!a)
    {
      vtkErrorMacro("Cannot find gbc array");
      return false;
    }
  }
  else
  {
    vtkErrorMacro("vtkMultiVariateToRGBA output is not a table or a dataset");
    return false;
  }

  // draw the color wheel
  vtkNew<vtkBrush> brush;
  painter->ApplyBrush(brush);
  for (int i = 0; i < segmentCount; ++i)
  {
    std::array<float, 3> hsv = { static_cast<float>(i) / segmentCount, 1.0, 1.0 };
    std::array<float, 3> rgbFloat;
    vtkMath::HSVToRGB(hsv.data(), rgbFloat.data());
    std::array<unsigned char, 3> rgb;
    std::transform(rgbFloat.begin(), rgbFloat.end(), rgb.begin(), [](float v) { return v * 255; });
    painter->GetBrush()->SetColor(rgb.data());
    painter->DrawEllipseWedge(this->Internals->CenterX, this->Internals->CenterY,
      this->Internals->Radius, this->Internals->Radius, 0.0, 0.0, 360.0 * i / segmentCount,
      360.0 * (i + 1) / segmentCount);
  }

  // draw the points
  const int marker = VTK_MARKER_CIRCLE;
  vtkNew<vtkPoints2D> points;
  points->SetNumberOfPoints(a->GetNumberOfTuples());
  if (!vtkArrayDispatch::Dispatch::Execute(a, *this->Internals, points))
  {
    (*this->Internals)(a, points);
  }
  painter->GetPen()->SetColor({ 0, 0, 0 });
  painter->GetPen()->SetOpacity(20);
  painter->GetPen()->SetWidth(6);
  painter->DrawMarkers(marker, true /*highlight*/, points);

  this->PaintChildren(painter);
  return true;
}

//------------------------------------------------------------------------------

void vtkPlotRadViz::SetDimensions(int arg1, int arg2, int arg3, int arg4)
{
  if (arg1 != this->Dimensions[0] || arg2 != this->Dimensions[1] || arg3 != this->Dimensions[2] ||
    arg4 != this->Dimensions[3])
  {
    this->Dimensions[0] = arg1;
    this->Dimensions[1] = arg2;
    this->Dimensions[2] = arg3;
    this->Dimensions[3] = arg4;

    this->Internals->CenterX = this->Dimensions[0] + 0.5 * this->Dimensions[2];
    this->Internals->CenterY = this->Dimensions[1] + 0.5 * this->Dimensions[3];
    this->Internals->Radius = this->Dimensions[2] < this->Dimensions[3] ? 0.5 * this->Dimensions[2]
                                                                        : 0.5 * this->Dimensions[3];
    this->Modified();
  }
}

void vtkPlotRadViz::SetDimensions(const int arg[4])
{
  this->SetDimensions(arg[0], arg[1], arg[2], arg[3]);
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
