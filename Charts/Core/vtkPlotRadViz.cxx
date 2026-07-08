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
#include "vtkMathUtilities.h"
#include "vtkMultiVariateToRGBA.h"
#include "vtkObjectFactory.h"
#include "vtkPen.h"
#include "vtkPointData.h"
#include "vtkPoints2D.h"
#include "vtkRenderingCoreEnums.h"
#include "vtkSmartPointer.h"
#include "vtkSmartPointerBase.h"
#include "vtkStringArray.h"
#include "vtkTable.h"
#include "vtkTextProperty.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>

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
  vtkInternals(vtkObject* parent)
    : Parent(parent)
  {
    this->Center[0] = this->Center[1] = 0;
    this->Radius = 0;
  }
  /**
   * Translates and scales GBC and vertex coordinates from the unit circle
   * centered at (0,0) to the plot circle centered at (Center[0], Center[1])
   * with radius Radius.
   */
  template <class TArray>
  void operator()(TArray* inputArray, vtkPoints2D* points)
  {
    auto input = vtk::DataArrayTupleRange(inputArray);
    auto output = vtkAOSDataArrayTemplate<float>::FastDownCast(points->GetData())->GetPointer(0);
    points->SetNumberOfPoints(input.size());
    for (int i = 0; i < input.size(); ++i)
    {
      output[2 * i] = this->Center[0] + input[i][0] * this->Radius;
      output[2 * i + 1] = this->Center[1] + input[i][1] * this->Radius;
    }
  }

  double GetAngle(double vertex[2], double mousePos[2])
  {
    mousePos[0] -= this->Center[0];
    mousePos[1] -= this->Center[1];
    vertex[0] -= this->Center[0];
    vertex[1] -= this->Center[1];
    double vertexRad = std::atan2(vertex[1], vertex[0]);
    double mousePosRad = std::atan2(mousePos[1], mousePos[0]);
    vtkDebugWithObjectMacro(this->Parent,
      "mouse angle: " << vtkMath::DegreesFromRadians(mousePosRad)
                      << " vertex angle: " << vtkMath::DegreesFromRadians(vertexRad));
    double angle = vtkMath::DegreesFromRadians(mousePosRad - vertexRad);
    return angle;
  }

  void SetCenterAndRadius(int dimensions[4])
  {
    this->Center[0] = dimensions[0] + 0.5 * dimensions[2];
    this->Center[1] = dimensions[1] + 0.5 * dimensions[3];
    this->Radius = dimensions[2] < dimensions[3] ? 0.5 * dimensions[2] : 0.5 * dimensions[3];
  }

  /**
   * The center of the GBC circle in window space
   */
  float Center[2];
  /**
   * The radius of the GBC circle in window space
   */
  float Radius;
  /**
   * PlotRadViz object, used for debug or error logging
   */
  vtkObject* Parent;
};

//------------------------------------------------------------------------------
vtkPlotRadViz::vtkPlotRadViz()
  : Internals(new vtkInternals(this))
{
  this->Dimensions[0] = this->Dimensions[1] = this->Dimensions[2] = this->Dimensions[3] = 0;
  this->VerticesPen = vtkSmartPointer<vtkPen>::New();
  this->VerticesPen->SetWidth(8);

  this->LensPen = vtkSmartPointer<vtkPen>::New();

  this->ActiveColor = vtkColor4ub(255, 0, 0, 255); // red
  this->LensCenter[0] = this->LensCenter[1] = 0;
  this->LensRadius = 0;
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
  this->VerticesRotation = 0;
  this->OutsideLensAlphaCap = 1;
}

//------------------------------------------------------------------------------
const float* vtkPlotRadViz::GetCenter() const
{
  return this->Internals->Center;
}

//------------------------------------------------------------------------------
float vtkPlotRadViz::GetRadius() const
{
  return this->Internals->Radius;
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::SetLensRadius(float lensRadius)
{
  vtkDebugMacro(<< " setting LensRadius to " << lensRadius);
  float newLensRadius = vtkMultiVariateToRGBA::GetLensRadius(
    lensRadius, this->Internals->Radius, this->LensCenter, this->Internals->Center);

  if (newLensRadius != this->LensRadius)
  {
    this->LensRadius = newLensRadius;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::SetLensCenter(const float* lensCenter)
{
  vtkDebugMacro(<< " setting LensLensCenter to [" << lensCenter[0] << ", " << lensCenter[1] << ")");
  std::array<float, 2> newLensCenter = vtkMultiVariateToRGBA::GetLensCenter(
    this->LensRadius, this->Internals->Radius, lensCenter, this->Internals->Center);
  if (this->LensCenter[0] != newLensCenter[0] || this->LensCenter[1] != newLensCenter[1])
  {
    std::copy(newLensCenter.begin(), newLensCenter.end(), this->LensCenter);
    this->Modified();
  }
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
vtkDataArray* vtkPlotRadViz::GetArray(vtkTable* table, const char* name)
{
  auto a = vtkDataArray::FastDownCast(table->GetColumnByName(name));
  if (!a)
  {
    throw std::runtime_error("Cannot find " + std::string(name) + " array");
  }
  return a;
}

//------------------------------------------------------------------------------
vtkDataArray* vtkPlotRadViz::GetArray(vtkDataSet* dataset, const char* name)
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
std::tuple<vtkDataArray*, vtkDataArray*, vtkDataArray*> vtkPlotRadViz::GetVerticesGbcRgba(
  vtkDataObject* output)
{
  auto table = vtkTable::SafeDownCast(output);
  auto dataset = vtkDataSet::SafeDownCast(output);
  vtkDataArray* gbc = nullptr;
  vtkDataArray* rgba = nullptr;
  if (table)
  {
    gbc = this->GetArray(table, "gbc");
    if (this->ScalarVisibility)
    {
      rgba = this->GetArray(table, "rgba");
    }
  }
  else if (dataset)
  {
    gbc = this->GetArray(dataset, "gbc");
    if (this->ScalarVisibility)
    {
      rgba = this->GetArray(dataset, "rgba");
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
    computeGBC->SetLensRadius(this->LensRadius / this->Internals->Radius);
    computeGBC->SetOutsideLensAlphaCap(this->OutsideLensAlphaCap);
    std::array<float, 2> lensCenter = { (this->LensCenter[0] - this->Internals->Center[0]) /
        this->Internals->Radius,
      (this->LensCenter[1] - this->Internals->Center[1]) / this->Internals->Radius };
    computeGBC->SetLensCenter(lensCenter.data());
    if (array)
    {
      // set the array to process and association to vtkMultiVariateToRGBA
      computeGBC->SetInputArrayToProcess(array->GetName(), association);
    }
    computeGBC->Update();
    vtkDataObject* output = computeGBC->GetOutputDataObject(0);
    auto [vertices, gbc, rgba] = this->GetVerticesGbcRgba(output);
    if (this->ColorWheelVisibility)
    {
      this->DrawColorWheel(painter);
    }
    // draw the data points
    if (this->ScalarVisibility)
    {
      this->DrawColoredMarkers(painter, gbc, this->Pen, rgba);
    }
    else
    {
      this->DrawMarkers(painter, gbc, this->Pen);
    }
    // draw the vertices
    this->Vertices = this->DrawMarkers(painter, vertices, this->VerticesPen, ActiveVertexIndex);
    this->DrawLabels(painter, vertices, this->VerticesPen);
    this->DrawLens(painter);
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
  double cos_a = (xy[0] - this->Internals->Center[0]) / this->Internals->Radius;
  double x = cos_a * radius;
  double sin_a = (xy[1] - this->Internals->Center[1]) / this->Internals->Radius;
  double y = sin_a * radius;
  xy[0] += x;
  xy[1] += y;
}

void vtkPlotRadViz::DrawLens(vtkContext2D* painter)
{
  // draw the center of the lens. DrawMarkers expects points in the [-1, 1] space.
  vtkNew<vtkFloatArray> center;
  center->SetNumberOfComponents(3);
  center->SetNumberOfTuples(1);
  center->SetTypedTuple(0,
    std::array<float, 3>{
      (this->LensCenter[0] - this->Internals->Center[0]) / this->Internals->Radius,
      (this->LensCenter[1] - this->Internals->Center[1]) / this->Internals->Radius, 0 }
      .data());
  this->DrawMarkers(painter, center, this->VerticesPen, this->ActiveLensInterior ? 0 : -1);
  // draw the lens circle
  painter->ApplyPen(this->LensPen);
  if (this->ActiveLensOutline)
  {
    painter->GetPen()->SetColor(this->GetActiveColor());
  }
  painter->GetBrush()->SetColor(0, 0, 0, 0); // transparent brush (only outline)
  painter->DrawArc(this->LensCenter[0], this->LensCenter[1], this->LensRadius, 0, 360);
}

void vtkPlotRadViz::DrawColorWheel(vtkContext2D* painter)
{
  painter->GetBrush()->SetColor(0, 0, 0, 255); // opaque
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
    painter->DrawEllipseWedge(this->Internals->Center[0], this->Internals->Center[1],
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

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPoints2D> vtkPlotRadViz::DrawMarkers(
  vtkContext2D* painter, vtkDataArray* points, vtkPen* pen, int activeIndex)
{
  const int markerStyle = VTK_MARKER_CIRCLE;
  auto transformedPoints = this->TransformPoints(points);
  painter->ApplyPen(pen);
  vtkFloatArray* dataArray = vtkFloatArray::SafeDownCast(transformedPoints->GetData());
  float* data = dataArray->GetPointer(0);
  painter->DrawMarkers(
    markerStyle, false /*highlight*/, data, transformedPoints->GetNumberOfPoints());
  if (activeIndex >= 0)
  {
    painter->DrawMarkers(
      markerStyle, false /*highlight*/, data + 2 * activeIndex, 1, this->ActiveColor.GetData(), 4);
  }
  return transformedPoints;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPoints2D> vtkPlotRadViz::DrawColoredMarkers(
  vtkContext2D* painter, vtkDataArray* points, vtkPen* pen, vtkDataArray* colors)
{
  const int markerStyle = VTK_MARKER_CIRCLE;
  auto transformedPoints = this->TransformPoints(points);
  painter->ApplyPen(pen);
  auto transformedColors = this->TransformColors(colors);
  painter->DrawMarkers(markerStyle, false /*highlight*/, transformedPoints, transformedColors);
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
    this->Internals->SetCenterAndRadius(this->Dimensions);
    std::copy(this->Internals->Center, this->Internals->Center + 2, this->LensCenter);
    this->LensRadius = this->Internals->Radius;

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
