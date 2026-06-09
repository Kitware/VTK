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
#include "vtkMath.h"
#include "vtkMathUtilities.h"
#include "vtkMultiVariateToRGBA.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"
#include "vtkRenderingCoreEnums.h"
#include "vtkSmartPointerBase.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTextProperty.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>
#include <vtk_fmt.h>
// clang-format off
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ranges.h)
// clang-format on

VTK_ABI_NAMESPACE_BEGIN

namespace
{
/**
 * Transform colors from doubles between 0 and 1 to unsigned char between 0 and 255.
 */
class ColorTransform
{
public:
  ColorTransform() = default;
  template <class TArray>
  void operator()(TArray* inputArray, vtkUnsignedCharArray* colors)
  {
    using ValueType = typename TArray::ValueType;
    auto input = vtk::DataArrayValueRange(inputArray);
    auto output = vtk::DataArrayValueRange(colors);
    std::transform(input.begin(), input.end(), output.begin(), [](ValueType c) { return c * 255; });
  }
};
}

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
   * Used to translate and scale GBCs and Vertices from
   * from circle centered at (0,0) with radius 1 to the circle centered at (CenterX, CenterY)
   * and radius Radius.
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

  double GetAngle(double vertex[2], double mousePos[2])
  {
    mousePos[0] -= this->CenterX;
    mousePos[1] -= this->CenterY;
    vertex[0] -= this->CenterX;
    vertex[1] -= this->CenterY;
    double vertexRad = std::atan2(vertex[1], vertex[0]);
    double mousePosRad = std::atan2(mousePos[1], mousePos[0]);
    fmt::print("vtkPlotRadViz::GetAngle\n");
    fmt::print("center: [{}, {}]\n", this->CenterX, this->CenterY);
    fmt::print(
      "mouse: [{}, {}], vertex: [{}, {}]\n", mousePos[0], mousePos[1], vertex[0], vertex[1]);
    fmt::print("mouse: {}, vertex: {}\n", vtkMath::DegreesFromRadians(mousePosRad),
      vtkMath::DegreesFromRadians(vertexRad));
    double angle = vtkMath::DegreesFromRadians(mousePosRad - vertexRad);
    return angle;
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
  this->ColorWheelSegmentCount = 120;
  this->ColorWheelStartAngle = 90;
  this->ColorWheelClockwiseOrdering = true;
  this->ColorWheelVisibility = true;
  this->ScalarVisibility = false;
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::SetVerticesRotation(double rotation)
{
  vtkDebugMacro(<< " setting VerticesRotation to " << rotation);
  rotation = std::fmod(rotation, 360);
  if (this->VerticesRotation != rotation)
  {
    this->VerticesRotation = rotation;
    this->Modified();
  }
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
vtkDataArray* vtkPlotRadViz::GetOutputArray(vtkTable* table, const char* name)
{
  auto a = vtkDataArray::FastDownCast(table->GetColumnByName(name));
  if (!a)
  {
    throw std::runtime_error("Cannot find " + std::string(name) + " array");
  }
  return a;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkPlotRadViz::GetOutputArray(vtkDataSet* dataset, const char* name)
{
  vtkDataSetAttributes* pd = dataset->GetPointData();
  vtkDataSetAttributes* cd = dataset->GetCellData();
  auto a = vtkDataArray::FastDownCast(pd->GetArray(name));
  if (!a)
  {
    a = vtkDataArray::FastDownCast(cd->GetArray(name));
  }
  if (!a)
  {
    throw std::runtime_error("Cannot find " + std::string(name) + " array");
  }
  return a;
}

//------------------------------------------------------------------------------
std::tuple<vtkDataArray*, vtkDataArray*, vtkDataArray*> vtkPlotRadViz::GetOutputArrays(
  vtkDataObject* output)
{
  auto table = vtkTable::SafeDownCast(output);
  auto dataset = vtkDataSet::SafeDownCast(output);
  vtkDataArray* gbc = nullptr;
  vtkDataArray* rgba = nullptr;
  if (table)
  {
    gbc = this->GetOutputArray(table, "gbc");
    if (this->ScalarVisibility)
    {
      rgba = this->GetOutputArray(table, "rgba");
    }
  }
  else if (dataset)
  {
    gbc = this->GetOutputArray(dataset, "gbc");
    if (this->ScalarVisibility)
    {
      rgba = this->GetOutputArray(dataset, "rgba");
    }
  }
  else
  {
    throw std::runtime_error("vtkMultiVariateToRGBA output is not a table or a dataset");
  }
  vtkDataArray* vertices = vtkDataArray::FastDownCast(output->GetFieldData()->GetArray("vertices"));
  if (!vertices)
  {
    throw std::runtime_error("Cannot find vertices array");
  }
  return { vertices, gbc, rgba };
}

//------------------------------------------------------------------------------
bool vtkPlotRadViz::Paint(vtkContext2D* painter)
{
  try
  {
    vtkDebugMacro(<< "Paint event called.");
    fmt::print("==================== vtkPlotRadViz::Paint ====================\n");
    vtkDataObject* input = this->Data->GetInputDataObject(0, 0);
    vtkTable* table = vtkTable::SafeDownCast(input);
    vtkDataSet* dataset = vtkDataSet::SafeDownCast(input);
    vtkDataArray* array = nullptr;
    int association;
    if (dataset)
    {
      // get the array to process and association from vtkContextMapper2D
      array = this->GetData()->GetInputArrayToProcess(0, input, association);
    }
    ComputeAutoLabels(input, array);

    vtkNew<vtkMultiVariateToRGBA> computeGBC;
    computeGBC->SetInputDataObject(0, input);
    computeGBC->SetStoreGBC(true);
    computeGBC->SetStoreVertices(true);
    computeGBC->SetVerticesRotation(this->VerticesRotation);
    computeGBC->SetStoreRGBA(this->ScalarVisibility);
    if (array)
    {
      // set the array to process and association to vtkMultiVariateToRGBA
      computeGBC->SetInputArrayToProcess(array->GetName(), association);
    }
    computeGBC->Update();
    vtkDataObject* output = computeGBC->GetOutputDataObject(0);
    auto [vertices, gbc, rgba] = this->GetOutputArrays(output);
    if (this->ColorWheelVisibility)
    {
      this->DrawColorWheel(painter);
    }
    this->DrawMarkers(painter, gbc, this->Pen, rgba);
    this->Vertices = this->DrawMarkers(painter, vertices, this->VerticesPen);
    this->DrawLabels(painter, vertices, this->VerticesPen);
    this->PaintChildren(painter);
    return true;
  }
  catch (std::runtime_error& e)
  {
    vtkErrorMacro(<< e.what());
    return false;
  }
}

vtkSmartPointer<vtkPoints2D> vtkPlotRadViz::TransformPoints(vtkDataArray* points)
{
  typedef vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals> Dispatcher;
  auto transformedPoints = vtkSmartPointer<vtkPoints2D>::New();
  transformedPoints->SetNumberOfPoints(points->GetNumberOfTuples());
  if (!Dispatcher::Execute(points, *this->Internals, transformedPoints))
  {
    vtkErrorMacro("Invalid type for the colors array: " << points->GetClassName());
  }
  return transformedPoints;
}

vtkSmartPointer<vtkUnsignedCharArray> vtkPlotRadViz::TransformColors(vtkDataArray* colors)
{
  typedef vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::Reals> Dispatcher;
  auto transformedColors = vtkSmartPointer<vtkUnsignedCharArray>::New();
  transformedColors->SetNumberOfComponents(colors->GetNumberOfComponents());
  transformedColors->SetNumberOfTuples(colors->GetNumberOfTuples());
  ColorTransform worker;
  if (!Dispatcher::Execute(colors, worker, transformedColors))
  {
    vtkErrorMacro("Invalid type for the colors array: " << colors->GetClassName());
  }
  return transformedColors;
}

double vtkPlotRadViz::GetAngle(double vertex[2], double mousePos[2])
{
  return this->Internals->GetAngle(vertex, mousePos);
}

std::pair<int, int> vtkPlotRadViz::GetJustification(double xy[2])
{
  if (vtkMathUtilities::FuzzyCompare(xy[1], 0.0))
  {
    if (xy[0] > 0)
    {
      // angle: 0
      return { VTK_TEXT_LEFT, VTK_TEXT_CENTERED };
    }
    else
    {
      // angle: 180
      return { VTK_TEXT_RIGHT, VTK_TEXT_CENTERED };
    }
  }
  else if (vtkMathUtilities::FuzzyCompare(xy[0], 0.0))
  {
    if (xy[1] > 0)
    {
      // angle: 90
      return { VTK_TEXT_CENTERED, VTK_TEXT_BOTTOM };
    }
    else
    {
      // angle: 270
      return { VTK_TEXT_CENTERED, VTK_TEXT_TOP };
    }
  }
  else if (xy[0] > 0)
  {
    if (xy[1] > 0)
    {
      // first quadrant (0 < angle < 90)
      return { VTK_TEXT_LEFT, VTK_TEXT_BOTTOM };
    }
    else
    {
      // forth quadrant (270 < angle < 360)
      return { VTK_TEXT_LEFT, VTK_TEXT_TOP };
    }
  }
  else
  {
    if (xy[1] > 0)
    {
      // second quadrant (90 < angle < 180)
      return { VTK_TEXT_RIGHT, VTK_TEXT_BOTTOM };
    }
    else
    {
      // third quadrant (180 < angle < 270)
      return { VTK_TEXT_RIGHT, VTK_TEXT_TOP };
    }
  }
}

void vtkPlotRadViz::ShiftLabelByPenWidth(float width, double xy[2])
{
  double radius = (width / 2) * 1.2;
  double cos_a = (xy[0] - this->Internals->CenterX) / this->Internals->Radius;
  double x = cos_a * radius;
  double sin_a = (xy[1] - this->Internals->CenterY) / this->Internals->Radius;
  double y = sin_a * radius;
  xy[0] += x;
  xy[1] += y;
}

void vtkPlotRadViz::DrawColorWheel(vtkContext2D* painter)
{
  for (int i = 0; i < this->ColorWheelSegmentCount; ++i)
  {
    std::array<float, 3> hsv = { static_cast<float>(i) / this->ColorWheelSegmentCount, 1.0, 1.0 };
    std::array<float, 3> rgbFloat;
    vtkMath::HSVToRGB(hsv.data(), rgbFloat.data());
    std::array<unsigned char, 3> rgb;
    std::transform(rgbFloat.begin(), rgbFloat.end(), rgb.begin(), [](float v) { return v * 255; });
    painter->GetBrush()->SetColor(rgb.data());

    double shiftToCenterColor = 180.0 / this->ColorWheelSegmentCount;
    double start = 360.0 * i / this->ColorWheelSegmentCount - shiftToCenterColor;
    double end = 360.0 * (i + 1) / this->ColorWheelSegmentCount - shiftToCenterColor;
    if (this->ColorWheelClockwiseOrdering)
    {
      // DrawEllipseWedge wants start < end.
      double start_ = 360 - end;
      double end_ = 360 - start;
      start = start_;
      end = end_;
    }
    start += this->ColorWheelStartAngle;
    end += this->ColorWheelStartAngle;
    painter->DrawEllipseWedge(this->Internals->CenterX, this->Internals->CenterY,
      this->Internals->Radius, this->Internals->Radius, 0.0, 0.0, start, end);
  }
}

void vtkPlotRadViz::DrawLabels(vtkContext2D* painter, vtkDataArray* points, vtkPen* pen)
{
  assert(this->GetNumberOfLabels() == points->GetNumberOfTuples());
  auto transformedPoints = this->TransformPoints(points);
  for (int i = 0; i < points->GetNumberOfTuples(); ++i)
  {
    std::vector<double> xy(points->GetNumberOfComponents());
    points->GetTuple(i, xy.data());
    auto [just, vertJust] = this->GetJustification(xy.data());
    this->LabelsProperties->SetJustification(just);
    this->LabelsProperties->SetVerticalJustification(vertJust);
    painter->ApplyTextProp(this->LabelsProperties);
    double transformedXy[2];
    transformedPoints->GetPoint(i, transformedXy);
    std::string label = this->GetLabel(i);
    this->ShiftLabelByPenWidth(pen->GetWidth(), transformedXy);
    painter->DrawString(transformedXy[0], transformedXy[1], label.c_str());
  }
}

vtkSmartPointer<vtkPoints2D> vtkPlotRadViz::DrawMarkers(
  vtkContext2D* painter, vtkDataArray* points, vtkPen* pen, vtkDataArray* colors)
{
  const int markerStyle = VTK_MARKER_CIRCLE;
  auto transformedPoints = this->TransformPoints(points);
  painter->ApplyPen(pen);
  if (colors)
  {
    auto transformedColors = this->TransformColors(colors);
    painter->DrawMarkers(markerStyle, false /*highlight*/, transformedPoints, transformedColors);
  }
  else
  {
    painter->DrawMarkers(markerStyle, false /*highlight*/, transformedPoints);
  }
  return transformedPoints;
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
    fmt::print("Dimensions: {}\n", this->Dimensions);
    fmt::print("Center: [{}, {}], Radius: {}\n", this->Internals->CenterX, this->Internals->CenterY,
      this->Internals->Radius);
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
