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
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTextProperty.h"

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
   * Used to translate and scale GBCs and Vertices
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
  this->VerticesPen = vtkSmartPointer<vtkPen>::New();
  this->VerticesPen->SetWidth(8);
  this->LabelsProperties = vtkSmartPointer<vtkTextProperty>::New();
  this->LabelsProperties->SetJustificationToCentered();
  this->LabelsProperties->SetColor(0.0, 0.0, 0.0);
  this->LabelsProperties->SetFontSize(12);
  this->LabelsProperties->SetFontFamilyToArial();
  // by default process active point/cell scalars if the input is a dataset
  // otherwise we process all columns of a table
  this->GetData()->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);
}

void vtkPlotRadViz::SetInputData(vtkDataSet* dataset)
{
  this->Data->SetInputDataObject(0, dataset);
}

void vtkPlotRadViz::ComputeAutoLabels(vtkDataObject* input, vtkDataArray* array)
{
  vtkTable* table = vtkTable::SafeDownCast(input);
  vtkDataSet* dataset = vtkDataSet::SafeDownCast(input);
  if (table)
  {
    this->AutoLabels = vtkSmartPointer<vtkStringArray>::New();
    int nColumns = table->GetNumberOfColumns();
    this->AutoLabels->SetNumberOfValues(nColumns);
    for (int i = 0; i < nColumns; ++i)
    {
      const char* name = table->GetColumnName(i);
      if (name)
      {
        this->AutoLabels->SetValue(i, name);
      }
    }
  }
  else if (dataset)
  {
    this->AutoLabels = vtkSmartPointer<vtkStringArray>::New();
    int nComponents = array->GetNumberOfComponents();
    this->AutoLabels->SetNumberOfValues(nComponents);
    for (int i = 0; i < nComponents; ++i)
    {
      const char* name = array->GetComponentName(i);
      if (name)
      {
        this->AutoLabels->SetValue(i, name);
      }
    }
  }
  // different type of data, no auto labels
}

//------------------------------------------------------------------------------
bool vtkPlotRadViz::Paint(vtkContext2D* painter)
{
  vtkDebugMacro(<< "Paint event called.");
  const int segmentCount = 100;
  vtkDataObject* input = this->Data->GetInputDataObject(0, 0);
  vtkTable* table = vtkTable::SafeDownCast(input);
  vtkDataSet* dataset = vtkDataSet::SafeDownCast(input);
  vtkDataArray* array = nullptr;
  int association;
  if (dataset)
  {
    array = this->GetData()->GetInputArrayToProcess(0, input, association);
  }
  ComputeAutoLabels(input, array);

  vtkNew<vtkMultiVariateToRGBA> computeGBC;
  computeGBC->SetInputDataObject(0, input);
  computeGBC->SetStoreGBC(true);
  computeGBC->SetStoreVertices(true);
  computeGBC->SetStoreRGBA(false);
  if (array)
  {
    // pass the array to process to computeGBC for a dataset
    computeGBC->SetInputArrayToProcess(array->GetName(), association);
  }
  computeGBC->Update();
  vtkDataObject* output = computeGBC->GetOutputDataObject(0);
  table = vtkTable::SafeDownCast(output);
  dataset = vtkDataSet::SafeDownCast(output);
  vtkFloatArray* gbc = nullptr;
  if (table)
  {
    gbc = vtkFloatArray::FastDownCast(table->GetColumnByName("gbc"));
    if (!gbc)
    {
      vtkErrorMacro("Cannot find gbc array");
      return false;
    }
  }
  else if (dataset)
  {
    vtkDataSetAttributes* pd = dataset->GetPointData();
    vtkDataSetAttributes* cd = dataset->GetCellData();
    gbc = vtkFloatArray::FastDownCast(pd->GetArray("gbc"));
    if (!gbc)
    {
      gbc = vtkFloatArray::FastDownCast(cd->GetArray("gbc"));
    }
    if (!gbc)
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
  vtkFloatArray* vertices =
    vtkFloatArray::FastDownCast(output->GetFieldData()->GetArray("vertices"));
  if (!vertices)
  {
    vtkErrorMacro("Cannot find vertices array");
    return false;
  }

  // paint the color wheel
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

  this->DrawMarkers(painter, gbc, this->Pen);
  this->DrawMarkers(painter, vertices, this->VerticesPen);
  this->DrawLabels(painter, vertices);
  this->PaintChildren(painter);
  return true;
}

vtkSmartPointer<vtkPoints2D> vtkPlotRadViz::Transform(vtkFloatArray* points)
{
  auto transformedPoints = vtkSmartPointer<vtkPoints2D>::New();
  transformedPoints->SetNumberOfPoints(points->GetNumberOfTuples());
  if (!vtkArrayDispatch::Dispatch::Execute(points, *this->Internals, transformedPoints))
  {
    (*this->Internals)(points, transformedPoints);
  }
  return transformedPoints;
}

void vtkPlotRadViz::DrawLabels(vtkContext2D* painter, vtkFloatArray* points)
{
  assert(this->GetNumberOfLabels() == points->GetNumberOfTuples());
  auto transformedPoints = this->Transform(points);
  painter->ApplyTextProp(this->LabelsProperties);
  for (int i = 0; i < points->GetNumberOfTuples(); ++i)
  {
    double xy[2];
    transformedPoints->GetPoint(i, xy);
    std::string label = this->GetLabel(i);
    painter->DrawString(xy[0], xy[1], label.c_str());
  }
}

void vtkPlotRadViz::DrawMarkers(vtkContext2D* painter, vtkFloatArray* points, vtkPen* pen)
{
  const int markerStyle = VTK_MARKER_CIRCLE;
  auto transformedPoints = this->Transform(points);
  painter->ApplyPen(pen);
  painter->DrawMarkers(markerStyle, false /*highlight*/, transformedPoints);
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
