// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkPlot3D.h"

#include "vtkChartXYZ.h"
#include "vtkDataArray.h"
#include "vtkIdTypeArray.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPoints.h"
#include "vtkStringArray.h"
#include "vtkTable.h"

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkPlot3D::vtkPlot3D()
{
  this->Points->SetDataTypeToFloat();
  this->Pen = vtkSmartPointer<vtkPen>::New();
  this->Pen->SetWidth(2.0);
  this->SelectionPen = vtkSmartPointer<vtkPen>::New();
  this->SelectionPen->SetColor(255, 50, 0, 150);
  this->SelectionPen->SetWidth(4.0);
  this->NumberOfComponents = 0;
  this->Chart = nullptr;
}

//------------------------------------------------------------------------------
vtkPlot3D::~vtkPlot3D() = default;

//------------------------------------------------------------------------------
void vtkPlot3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkPlot3D::SetPen(vtkPen* pen)
{
  if (this->Pen != pen)
  {
    this->Pen = pen;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkPen* vtkPlot3D::GetSelectionPen()
{
  return this->SelectionPen;
}

//------------------------------------------------------------------------------
void vtkPlot3D::SetSelectionPen(vtkPen* pen)
{
  if (this->SelectionPen != pen)
  {
    this->SelectionPen = pen;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
vtkPen* vtkPlot3D::GetPen()
{
  return this->Pen;
}

//------------------------------------------------------------------------------
void vtkPlot3D::SetInputData(vtkTable* input)
{
  assert(input->GetNumberOfColumns() >= 3);

  // assume the 4th column is color info if available
  if (input->GetNumberOfColumns() > 3)
  {
    this->SetInputData(input, input->GetColumnName(0), input->GetColumnName(1),
      input->GetColumnName(2), input->GetColumnName(3));
  }
  else
  {
    this->SetInputData(
      input, input->GetColumnName(0), input->GetColumnName(1), input->GetColumnName(2));
  }
}

//------------------------------------------------------------------------------
void vtkPlot3D::SetInputData(
  vtkTable* input, vtkIdType xColumn, vtkIdType yColumn, vtkIdType zColumn)
{
  this->SetInputData(input, input->GetColumnName(xColumn), input->GetColumnName(yColumn),
    input->GetColumnName(zColumn));
}

//------------------------------------------------------------------------------
void vtkPlot3D::SetInputData(
  vtkTable* input, const vtkStdString& xName, const vtkStdString& yName, const vtkStdString& zName)
{
  // Copy the points into our data structure for rendering - pack x, y, z...
  vtkDataArray* xArr = vtkArrayDownCast<vtkDataArray>(input->GetColumnByName(xName.c_str()));
  vtkDataArray* yArr = vtkArrayDownCast<vtkDataArray>(input->GetColumnByName(yName.c_str()));
  vtkDataArray* zArr = vtkArrayDownCast<vtkDataArray>(input->GetColumnByName(zName.c_str()));

  // Ensure that we have valid data arrays, and that they are of the same length.
  assert(xArr);
  assert(yArr);
  assert(zArr);
  assert(xArr->GetNumberOfTuples() == yArr->GetNumberOfTuples() &&
    xArr->GetNumberOfTuples() == zArr->GetNumberOfTuples());

  size_t n = xArr->GetNumberOfTuples();
  this->Points->SetNumberOfPoints(n);
  this->Points->GetData()->CopyComponent(0, xArr, 0);
  this->Points->GetData()->CopyComponent(1, yArr, 0);
  this->Points->GetData()->CopyComponent(2, zArr, 0);
  this->PointsBuildTime.Modified();

  // This removes the colors from our points.
  // They will be (re-)added by SetColors if necessary.
  this->NumberOfComponents = 0;

  this->XAxisLabel = xName;
  this->YAxisLabel = yName;
  this->ZAxisLabel = zName;
  this->ComputeDataBounds();
}

//------------------------------------------------------------------------------
void vtkPlot3D::SetInputData(vtkTable* input, const vtkStdString& xName, const vtkStdString& yName,
  const vtkStdString& zName, const vtkStdString& colorName)
{
  this->SetInputData(input, xName, yName, zName);

  vtkDataArray* colorArr =
    vtkArrayDownCast<vtkDataArray>(input->GetColumnByName(colorName.c_str()));
  this->SetColors(colorArr);
}

//------------------------------------------------------------------------------
void vtkPlot3D::SetColors(vtkDataArray* colorArr)
{
  assert(colorArr);
  const vtkIdType numPoints = this->Points->GetNumberOfPoints();
  assert((unsigned int)colorArr->GetNumberOfTuples() == numPoints);

  this->NumberOfComponents = 3;

  // generate a color lookup table
  vtkNew<vtkLookupTable> lookupTable;
  double min = VTK_DOUBLE_MAX;
  double max = VTK_DOUBLE_MIN;

  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    double value = colorArr->GetComponent(i, 0);
    if (value > max)
    {
      max = value;
    }
    if (value < min)
    {
      min = value;
    }
  }

  lookupTable->SetNumberOfTableValues(256);
  lookupTable->SetRange(min, max);
  lookupTable->Build();
  this->Colors->Reset();
  // important! The number of components lets graphics code know if alpha channel is present or
  // not. (rgba vs rgb)
  this->Colors->SetNumberOfComponents(this->NumberOfComponents);
  this->Colors->SetNumberOfTuples(numPoints);

  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    double value = colorArr->GetComponent(i, 0);
    this->Colors->SetTypedTuple(i, lookupTable->MapValue(value));
  }

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkPlot3D::ComputeDataBounds()
{
  double bounds[6] = {};
  vtkMath::UninitializeBounds(bounds);
  this->Points->GetBounds(bounds);

  const double& xMin = bounds[0];
  const double& xMax = bounds[1];
  const double& yMin = bounds[2];
  const double& yMax = bounds[3];
  const double& zMin = bounds[4];
  const double& zMax = bounds[5];

  this->DataBounds.clear();
  this->DataBounds.resize(8);
  float* data = this->DataBounds[0].GetData();

  // point 1: xMin, yMin, zMin
  data[0] = xMin;
  data[1] = yMin;
  data[2] = zMin;

  // point 2: xMin, yMin, zMax
  data[3] = xMin;
  data[4] = yMin;
  data[5] = zMax;

  // point 3: xMin, yMax, zMin
  data[6] = xMin;
  data[7] = yMax;
  data[8] = zMin;

  // point 4: xMin, yMax, zMax
  data[9] = xMin;
  data[10] = yMax;
  data[11] = zMax;

  // point 5: xMax, yMin, zMin
  data[12] = xMax;
  data[13] = yMin;
  data[14] = zMin;

  // point 6: xMax, yMin, zMax
  data[15] = xMax;
  data[16] = yMin;
  data[17] = zMax;

  // point 7: xMax, yMax, zMin
  data[18] = xMax;
  data[19] = yMax;
  data[20] = zMin;

  // point 8: xMax, yMax, zMax
  data[21] = xMax;
  data[22] = yMax;
  data[23] = zMax;
}

//------------------------------------------------------------------------------
void vtkPlot3D::SetChart(vtkChartXYZ* chart)
{
  this->Chart = chart;
}

//------------------------------------------------------------------------------
std::string vtkPlot3D::GetXAxisLabel()
{
  return this->XAxisLabel;
}

//------------------------------------------------------------------------------
std::string vtkPlot3D::GetYAxisLabel()
{
  return this->YAxisLabel;
}

//------------------------------------------------------------------------------
std::string vtkPlot3D::GetZAxisLabel()
{
  return this->ZAxisLabel;
}

//------------------------------------------------------------------------------
void vtkPlot3D::SetSelection(vtkIdTypeArray* id)
{
  if (id == this->Selection)
  {
    return;
  }
  this->Selection = id;
  this->Modified();
}

//------------------------------------------------------------------------------
vtkIdTypeArray* vtkPlot3D::GetSelection()
{
  return this->Selection;
}

//------------------------------------------------------------------------------
std::vector<vtkVector3f> vtkPlot3D::GetPoints()
{
  std::vector<vtkVector3f> points;
  const vtkIdType numPoints = this->Points->GetNumberOfPoints();
  for (vtkIdType i = 0; i < numPoints; ++i)
  {
    double p[3] = {};
    this->Points->GetPoint(i, p);
    points.emplace_back(p[0], p[1], p[2]);
  }
  return points;
}
VTK_ABI_NAMESPACE_END
