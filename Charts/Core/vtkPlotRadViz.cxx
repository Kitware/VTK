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
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkMath.h"
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
#include <vtk_fmt.h>
// clang-format off
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ranges.h)
// clang-format on

#include <algorithm>
#include <cmath>

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

/**
 * Applies 'change' to 'filter' and modifies 'plot' only if the filter
 * changed, so unchanged or rejected settings leave the plot unmodified.
 */
template <typename Change>
void ChangeFilter(vtkPlotRadViz* plot, vtkMultiVariateToRGBA* filter, Change&& change)
{
  vtkMTimeType mtime = filter->GetMTime();
  change(filter);
  if (filter->GetMTime() != mtime)
  {
    plot->Modified();
  }
}
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkPlotRadViz);

class vtkPlotRadViz::vtkInternals
{
public:
  vtkInternals()
  {
    this->SceneCenter[0] = this->SceneCenter[1] = 0;
    this->SceneRadius = 0;
  }
  /**
   * Translates and scales GBC and vertex coordinates from the unit circle
   * centered at (0,0) to the plot circle centered at (SceneCenter[0], SceneCenter[1])
   * with radius SceneRadius.
   */
  template <class TArray>
  void operator()(TArray* inputArray, vtkPoints2D* points)
  {
    auto input = vtk::DataArrayTupleRange(inputArray);
    points->SetNumberOfPoints(input.size());
    auto output = vtkAOSDataArrayTemplate<float>::FastDownCast(points->GetData())->GetPointer(0);
    for (int i = 0; i < input.size(); ++i)
    {
      output[2 * i] = this->SceneCenter[0] + input[i][0] * this->SceneRadius;
      output[2 * i + 1] = this->SceneCenter[1] + input[i][1] * this->SceneRadius;
    }
  }

  void SetSceneCenterAndRadius(int dimensions[4])
  {
    this->SceneCenter[0] = dimensions[0] + 0.5 * dimensions[2];
    this->SceneCenter[1] = dimensions[1] + 0.5 * dimensions[3];
    this->SceneRadius = dimensions[2] < dimensions[3] ? 0.5 * dimensions[2] : 0.5 * dimensions[3];
  }

  /**
   * The center of the GBC circle in scene coordinates
   */
  double SceneCenter[2];
  /**
   * The radius of the GBC circle in scene coordinates
   */
  double SceneRadius;
};

//------------------------------------------------------------------------------
vtkPlotRadViz::vtkPlotRadViz()
  : Internals(new vtkInternals())
{
  this->Pen->SetColor(255, 255, 255, 255);
  this->Pen->SetWidth(6);
  this->VerticesPen->SetColor(255, 255, 255, 255);
  this->VerticesPen->SetWidth(10);
  this->LensPen->SetColor(255, 0, 0, 128);
  this->LensPen->SetWidth(8);
  this->LabelsProperties->SetJustificationToCentered();
  this->LabelsProperties->SetColor(0.0, 0.0, 0.0);
  this->LabelsProperties->SetFontSize(16);
  this->LabelsProperties->SetFontFamilyToArial();
  this->LabelsProperties->BoldOn();
  // by default process active point/cell scalars if the input is a dataset
  // otherwise we process all columns of a table
  this->GetData()->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);
  // the plot renders the data it is given, so normalization is local. Painting
  // is not collective, so the global controller cannot be used here.
  this->ComputeGBC->SetController(nullptr);
}

vtkPlotRadViz::~vtkPlotRadViz()

{
  delete this->Internals;
}

//------------------------------------------------------------------------------
const double* vtkPlotRadViz::GetSceneCenter() const
{
  return this->Internals->SceneCenter;
}

//------------------------------------------------------------------------------
double vtkPlotRadViz::GetSceneRadius() const
{
  return this->Internals->SceneRadius;
}

//------------------------------------------------------------------------------
double* vtkPlotRadViz::GetSceneLensCenter()
{
  // clamped like vtkMultiVariateToRGBA does, so the lens is drawn where it applies
  const double origin[2] = { 0, 0 };
  std::array<double, 2> center =
    vtkMultiVariateToRGBA::GetLensCenter(this->LensRadius, 1, this->LensCenter, origin);
  for (int i = 0; i < 2; ++i)
  {
    this->SceneLensCenter[i] =
      this->Internals->SceneCenter[i] + center[i] * this->Internals->SceneRadius;
  }
  return this->SceneLensCenter;
}

//------------------------------------------------------------------------------
double vtkPlotRadViz::GetSceneLensRadius()
{
  return this->LensRadius * this->Internals->SceneRadius;
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::SetSceneLensRadius(double lensRadius)
{
  vtkDebugMacro(<< " setting SceneLensRadius to " << lensRadius);
  if (this->Internals->SceneRadius <= 0)
  {
    vtkWarningMacro("Use SetLensRadius before the plot is painted.");
    return;
  }
  const double origin[2] = { 0, 0 };
  this->SetLensRadius(vtkMultiVariateToRGBA::GetLensRadius(
    lensRadius / this->Internals->SceneRadius, 1, this->LensCenter, origin));
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::SetSceneLensCenter(const double* lensCenter)
{
  vtkDebugMacro(<< " setting SceneLensCenter to [" << lensCenter[0] << ", " << lensCenter[1]
                << ")");
  if (this->Internals->SceneRadius <= 0)
  {
    vtkWarningMacro("Use SetLensCenter before the plot is painted.");
    return;
  }
  const double origin[2] = { 0, 0 };
  const double center[2] = { (lensCenter[0] - this->Internals->SceneCenter[0]) /
      this->Internals->SceneRadius,
    (lensCenter[1] - this->Internals->SceneCenter[1]) / this->Internals->SceneRadius };
  std::array<double, 2> newCenter =
    vtkMultiVariateToRGBA::GetLensCenter(this->LensRadius, 1, center, origin);
  this->SetLensCenter(newCenter[0], newCenter[1]);
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::SetVerticesRotation(double rotation)
{
  vtkDebugMacro(<< " setting VerticesRotation to " << rotation);
  rotation = std::fmod(rotation, 360);
  if (rotation < 0)
  {
    rotation += 360;
  }
  if (this->VerticesRotation != rotation)
  {
    this->VerticesRotation = rotation;
    this->Modified();
  }
}

vtkStringArray* vtkPlotRadViz::GetLabels()
{
  return this->Labels;
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::SetInputDataSet(vtkDataSet* dataset)
{
  this->Data->SetInputDataObject(0, dataset);
  this->ComputeGBC->SetInputDataObject(0, dataset);
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::SetInputData(vtkTable* table)
{
  this->Superclass::SetInputData(table);
  this->ComputeGBC->SetInputDataObject(0, table);
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::SetInputArray(int index, const vtkStdString& name)
{
  if (index != 0)
  {
    this->Superclass::SetInputArray(index, name);
    return;
  }
  // vtkPlot uses the rows association, which a dataset does not have
  if (name.empty())
  {
    // the default, saved as an empty name by serialization
    this->Data->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, vtkDataSetAttributes::SCALARS);
  }
  else
  {
    this->Data->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS, name.c_str());
  }
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::SetVariableVisibility(const std::string& name, bool visible)
{
  ::ChangeFilter(this, this->ComputeGBC,
    [&](vtkMultiVariateToRGBA* f) { f->SetVariableVisibility(name, visible); });
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::SetVariableVisibility(int index, bool visible)
{
  ::ChangeFilter(this, this->ComputeGBC,
    [&](vtkMultiVariateToRGBA* f) { f->SetVariableVisibility(index, visible); });
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::SetVariableVisibilityAll(bool visible)
{
  ::ChangeFilter(this, this->ComputeGBC,
    [&](vtkMultiVariateToRGBA* f) { f->SetVariableVisibilityAll(visible); });
}

//------------------------------------------------------------------------------
bool vtkPlotRadViz::GetVariableVisibility(const std::string& name)
{
  this->UpdateFilterInput();
  return this->ComputeGBC->GetVariableVisibility(name);
}

//------------------------------------------------------------------------------
bool vtkPlotRadViz::GetVariableVisibility(int index)
{
  this->UpdateFilterInput();
  return this->ComputeGBC->GetVariableVisibility(index);
}

//------------------------------------------------------------------------------
vtkStringArray* vtkPlotRadViz::GetVisibleVariables()
{
  this->UpdateFilterInput();
  return this->ComputeGBC->GetVisibleVariables();
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::SetVisibleVariables(vtkStringArray* names)
{
  ::ChangeFilter(
    this, this->ComputeGBC, [&](vtkMultiVariateToRGBA* f) { f->SetVisibleVariables(names); });
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::SetVisibleVariableIndices(const std::vector<int>& indices)
{
  ::ChangeFilter(this, this->ComputeGBC,
    [&](vtkMultiVariateToRGBA* f) { f->SetVisibleVariableIndices(indices); });
}

//------------------------------------------------------------------------------
std::vector<int> vtkPlotRadViz::GetVisibleVariableIndices()
{
  this->UpdateFilterInput();
  return this->ComputeGBC->GetVisibleVariableIndices();
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::ResetVisibleVariableIndices()
{
  ::ChangeFilter(
    this, this->ComputeGBC, [&](vtkMultiVariateToRGBA* f) { f->ResetVisibleVariableIndices(); });
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::AddVisibleVariableIndex(int index)
{
  ::ChangeFilter(
    this, this->ComputeGBC, [&](vtkMultiVariateToRGBA* f) { f->AddVisibleVariableIndex(index); });
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::UpdateFilterInput()
{
  // vtkPlot::Update only updates a table input, so update a dataset's producer here
  int producerPort = 0;
  if (vtkAlgorithm* producer = this->Data->GetInputAlgorithm(0, 0, producerPort))
  {
    producer->Update(producerPort);
  }
  this->ComputeGBC->SetInputDataObject(0, this->Data->GetInputDataObject(0, 0));
  // the array selection of vtkContextMapper2D, which may be unnamed
  vtkInformation* plotArray = this->Data->GetInputArrayInformation(0);
  int association = plotArray->Get(vtkDataObject::FIELD_ASSOCIATION());
  if (const char* name = plotArray->Get(vtkDataObject::FIELD_NAME()))
  {
    this->ComputeGBC->SetInputArrayToProcess(0, 0, 0, association, name);
  }
  else
  {
    // e.g. active scalars: the filter finds the same, possibly unnamed, array
    this->ComputeGBC->SetInputArrayToProcess(
      0, 0, 0, association, plotArray->Get(vtkDataObject::FIELD_ATTRIBUTE_TYPE()));
  }
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::ComputeVisibleLabels()
{
  this->VisibleLabels = vtkSmartPointer<vtkStringArray>::New();
  vtkSmartPointer<vtkStringArray> allNames = this->ComputeGBC->GetAllVariableNames();
  if (!this->Labels || this->Labels->GetNumberOfValues() != allNames->GetNumberOfValues())
  {
    // a copy, as the filter refills the array it returns on every call
    this->VisibleLabels->DeepCopy(this->ComputeGBC->GetVisibleVariables());
    return;
  }
  // user labels are one per input variable, keep the visible ones
  for (int index : this->ComputeGBC->GetVisibleVariableIndices())
  {
    this->VisibleLabels->InsertNextValue(this->Labels->GetValue(index));
  }
}

//------------------------------------------------------------------------------
vtkDataArray* vtkPlotRadViz::GetArray(vtkDataSetAttributes* attributes, const char* name)
{
  auto a = vtkDataArray::FastDownCast(attributes->GetAbstractArray(name));
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
  vtkDataSetAttributes* attributes = nullptr;
  if (auto* table = vtkTable::SafeDownCast(output))
  {
    attributes = table->GetRowData();
  }
  else if (auto* dataset = vtkDataSet::SafeDownCast(output))
  {
    // the filter adds them next to the array it processes, which is the mapper's array
    int association = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    this->GetData()->GetInputArrayToProcess(0, dataset, association);
    attributes = association == vtkDataObject::FIELD_ASSOCIATION_CELLS
      ? static_cast<vtkDataSetAttributes*>(dataset->GetCellData())
      : static_cast<vtkDataSetAttributes*>(dataset->GetPointData());
  }
  else
  {
    throw std::runtime_error("vtkMultiVariateToRGBA output is not a table or a dataset");
  }
  vtkDataArray* gbc = this->GetArray(attributes, "gbc");
  vtkDataArray* rgba = this->GetArray(attributes, "rgba");
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
  // no vertices to interact with unless they are drawn below
  this->Vertices = nullptr;
  if (!this->Visible)
  {
    return false;
  }
  try
  {
    // vtkMultiVariateToRGBA does the variable visibility filtering itself
    // (see SetVariableVisibility), so the raw input is passed as-is.
    this->UpdateFilterInput();
    vtkDataObject* input = this->Data->GetInputDataObject(0, 0);
    if (!input)
    {
      // no input, we don't render anything
      return true;
    }
    if (auto* table = vtkTable::SafeDownCast(input))
    {
      // vtkMultiVariateToRGBA rejects them, so report it instead of drawing nothing
      for (int index : this->ComputeGBC->GetVisibleVariableIndices())
      {
        vtkAbstractArray* column = table->GetColumn(index);
        if (column->GetNumberOfComponents() != 1)
        {
          throw std::runtime_error(std::string("Column has several components: ") +
            this->ComputeGBC->GetAllVariableNames()->GetValue(index));
        }
      }
    }
    // counts the variables of the input and array set above
    if (this->ComputeGBC->GetVisibleVariables()->GetNumberOfTuples() < 3)
    {
      // RadViz does not work for < 3 components, so we don't render anything
      return true;
    }
    this->ComputeVisibleLabels();
    this->UpdateCenterAndRadius(painter);
    if (this->Internals->SceneRadius <= 0)
    {
      // the labels leave no room for the plot, and the label positions would divide by 0
      return true;
    }

    this->ComputeGBC->SetStoreGBC(true);
    this->ComputeGBC->SetStoreVertices(true);
    this->ComputeGBC->SetVerticesRotation(this->VerticesRotation);
    this->ComputeGBC->SetVerticesClockwiseOrdering(this->VerticesClockwiseOrdering);
    // without scalar colors, only alpha is computed, which the lens caps (OutsideLensAlpha)
    this->ComputeGBC->SetStoreRGBA(this->ScalarVisibility);
    this->ComputeGBC->SetStoreA(!this->ScalarVisibility);
    this->ComputeGBC->SetLensRadius(this->LensRadius);
    this->ComputeGBC->SetOutsideLensAlpha(this->OutsideLensAlpha);
    this->ComputeGBC->SetLensCenter(this->LensCenter);
    if (!this->ComputeGBC->Update())
    {
      // the filter already reported why
      return false;
    }
    vtkDataObject* output = this->ComputeGBC->GetOutputDataObject(0);
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
      // Points share a single color (the pen's), but reuse the already
      // computed per-point alpha (rgba's A component) so the lens still
      // fades out points outside it.
      auto colors = this->ApplyPenColor(this->Pen, rgba);
      this->DrawColoredMarkers(painter, gbc, this->Pen, colors);
    }
    if (this->GetLensVisibility())
    {
      this->DrawLens(painter);
    }
    // draw the vertices
    this->Vertices = this->DrawMarkers(painter, vertices, this->VerticesPen, ActiveVertexIndex);
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
    vtkErrorMacro("Invalid type for the points array: " << points->GetClassName());
  }
  return transformedPoints;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkDoubleArray> vtkPlotRadViz::ApplyPenColor(vtkPen* pen, vtkDataArray* rgba)
{
  vtkColor4ub penColor = pen->GetColorObject();
  double r = penColor.GetRed() / 255.0;
  double g = penColor.GetGreen() / 255.0;
  double b = penColor.GetBlue() / 255.0;
  vtkIdType numberOfTuples = rgba->GetNumberOfTuples();
  auto colors = vtkSmartPointer<vtkDoubleArray>::New();
  colors->SetNumberOfComponents(4);
  colors->SetNumberOfTuples(numberOfTuples);
  for (vtkIdType i = 0; i < numberOfTuples; ++i)
  {
    colors->SetTuple4(i, r, g, b, rgba->GetComponent(i, 3));
  }
  return colors;
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

std::pair<int, int> vtkPlotRadViz::GetJustification(double xy[2])
{
  // Center the label around each cardinal angle (0, 90, 180, 270) for a
  // window of 30 degrees (+/- 15 degrees), instead of only for the exact
  // angle. Outside of these windows, justify based on the quadrant.
  constexpr double CARDINAL_ANGLE_WINDOW = 15.0;
  double angle = vtkMath::DegreesFromRadians(std::atan2(xy[1], xy[0]));
  if (angle < 0.0)
  {
    angle += 360.0;
  }

  if (angle <= CARDINAL_ANGLE_WINDOW || angle >= 360.0 - CARDINAL_ANGLE_WINDOW)
  {
    // around angle: 0
    return { VTK_TEXT_LEFT, VTK_TEXT_CENTERED };
  }
  else if (std::abs(angle - 180.0) <= CARDINAL_ANGLE_WINDOW)
  {
    // around angle: 180
    return { VTK_TEXT_RIGHT, VTK_TEXT_CENTERED };
  }
  else if (std::abs(angle - 90.0) <= CARDINAL_ANGLE_WINDOW)
  {
    // around angle: 90
    return { VTK_TEXT_CENTERED, VTK_TEXT_BOTTOM };
  }
  else if (std::abs(angle - 270.0) <= CARDINAL_ANGLE_WINDOW)
  {
    // around angle: 270
    return { VTK_TEXT_CENTERED, VTK_TEXT_TOP };
  }
  else if (angle < 90.0)
  {
    // first quadrant (0 < angle < 90)
    return { VTK_TEXT_LEFT, VTK_TEXT_BOTTOM };
  }
  else if (angle < 180.0)
  {
    // second quadrant (90 < angle < 180)
    return { VTK_TEXT_RIGHT, VTK_TEXT_BOTTOM };
  }
  else if (angle < 270.0)
  {
    // third quadrant (180 < angle < 270)
    return { VTK_TEXT_RIGHT, VTK_TEXT_TOP };
  }
  else
  {
    // forth quadrant (270 < angle < 360)
    return { VTK_TEXT_LEFT, VTK_TEXT_TOP };
  }
}

void vtkPlotRadViz::ShiftLabelByPenWidth(double width, double xy[2])
{
  double radius = (width / 2) * 1.2;
  double cos_a = (xy[0] - this->Internals->SceneCenter[0]) / this->Internals->SceneRadius;
  double x = cos_a * radius;
  double sin_a = (xy[1] - this->Internals->SceneCenter[1]) / this->Internals->SceneRadius;
  double y = sin_a * radius;
  xy[0] += x;
  xy[1] += y;
}

void vtkPlotRadViz::DrawLens(vtkContext2D* painter)
{
  const double* lensCenter = this->GetSceneLensCenter();
  painter->ApplyPen(this->LensPen);
  if (this->ActiveLensInterior)
  {
    // the center dot shows that the lens can be dragged
    float center[2] = { static_cast<float>(lensCenter[0]), static_cast<float>(lensCenter[1]) };
    painter->GetPen()->SetColor(this->GetActiveColor());
    painter->DrawMarkers(VTK_MARKER_CIRCLE, false /*highlight*/, center, 1);
    painter->ApplyPen(this->LensPen);
  }
  // draw the lens circle
  if (this->ActiveLensOutline)
  {
    painter->GetPen()->SetColor(this->GetActiveColor());
  }
  painter->GetBrush()->SetColor(0, 0, 0, 0); // transparent brush (only outline)
  painter->DrawArc(lensCenter[0], lensCenter[1], this->GetSceneLensRadius(), 0, 360);
}

int vtkPlotRadViz::GetColorWheelStartAngle()
{
  return this->ComputeGBC->GetColorWheelStartAngle();
}

//------------------------------------------------------------------------------
bool vtkPlotRadViz::GetColorWheelClockwiseOrdering()
{
  return this->ComputeGBC->GetColorWheelClockwiseOrdering();
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::DrawColorWheel(vtkContext2D* painter)
{
  const int startAngle = this->GetColorWheelStartAngle();
  const bool clockwiseOrdering = this->GetColorWheelClockwiseOrdering();
  painter->GetBrush()->SetColor(0, 0, 0, 255); // opaque
  // The hue varies with the angle and the saturation with the radius, using
  // the same HSL conversion as vtkMultiVariateToRGBA, so the wheel matches
  // the computed point colors.
  for (int i = 0; i < this->ColorWheelSegmentCount; ++i)
  {
    double hue = static_cast<double>(i) / this->ColorWheelSegmentCount;
    double shiftToCenterColor = 180.0 / this->ColorWheelSegmentCount;
    double start = 360.0 * i / this->ColorWheelSegmentCount - shiftToCenterColor;
    double end = 360.0 * (i + 1) / this->ColorWheelSegmentCount - shiftToCenterColor;
    if (clockwiseOrdering)
    {
      // DrawEllipseWedge wants start < end.
      double start_ = 360 - end;
      double end_ = 360 - start;
      start = start_;
      end = end_;
    }
    start += startAngle;
    end += startAngle;
    for (int j = 0; j < this->ColorWheelRingCount; ++j)
    {
      double saturation = (j + 0.5) / this->ColorWheelRingCount;
      std::array<double, 3> rgb;
      vtkMultiVariateToRGBA::HSLToRGB(
        hue, saturation, vtkMultiVariateToRGBA::ColorLightness, rgb.data());
      std::array<unsigned char, 3> rgbChar;
      std::transform(rgb.begin(), rgb.end(), rgbChar.begin(),
        [](double v) { return static_cast<unsigned char>(v * 255); });
      painter->GetBrush()->SetColor(rgbChar.data());
      double outerRadius = this->Internals->SceneRadius * (j + 1) / this->ColorWheelRingCount;
      double innerRadius = this->Internals->SceneRadius * j / this->ColorWheelRingCount;
      painter->DrawEllipseWedge(this->Internals->SceneCenter[0], this->Internals->SceneCenter[1],
        outerRadius, outerRadius, innerRadius, innerRadius, start, end);
    }
  }
}

void vtkPlotRadViz::DrawLabels(vtkContext2D* painter, vtkDataArray* points, vtkPen* pen)
{
  assert(this->VisibleLabels->GetNumberOfValues() == points->GetNumberOfTuples());
  auto transformedPoints = this->TransformPoints(points);
  // the painter copies the property, so the justification set below leaves it unchanged
  painter->ApplyTextProp(this->LabelsProperties);
  vtkTextProperty* textProperty = painter->GetTextProp();
  for (int i = 0; i < points->GetNumberOfTuples(); ++i)
  {
    std::vector<double> xy(points->GetNumberOfComponents());
    points->GetTuple(i, xy.data());
    auto [just, vertJust] = this->GetJustification(xy.data());
    textProperty->SetJustification(just);
    textProperty->SetVerticalJustification(vertJust);
    double transformedXy[2];
    transformedPoints->GetPoint(i, transformedXy);
    const std::string& label = this->VisibleLabels->GetValue(i);
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
  vtkFloatArray* dataArray = vtkFloatArray::SafeDownCast(transformedPoints->GetData());
  float* data = dataArray->GetPointer(0);
  int numberOfPoints = transformedPoints->GetNumberOfPoints();
  painter->ApplyPen(pen);
  float fillWidth = pen->GetWidth() - 2 * this->MarkerOutlineWidth;
  if (this->MarkerOutlineWidth > 0 && fillWidth > 0)
  {
    // the outline is a black marker showing around the smaller fill marker
    painter->GetPen()->SetColor(0, 0, 0);
    painter->DrawMarkers(markerStyle, false /*highlight*/, data, numberOfPoints);
    painter->ApplyPen(pen);
    painter->GetPen()->SetWidth(fillWidth);
  }
  painter->DrawMarkers(markerStyle, false /*highlight*/, data, numberOfPoints);
  // hiding a variable can leave a stale active index
  if (activeIndex >= 0 && activeIndex < numberOfPoints)
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
  auto transformedColors = this->TransformColors(colors);
  painter->ApplyPen(pen);
  float fillWidth = pen->GetWidth() - 2 * this->MarkerOutlineWidth;
  if (this->MarkerOutlineWidth > 0 && fillWidth > 0)
  {
    // the outline is a black marker (same alpha) showing around the smaller fill marker
    vtkNew<vtkUnsignedCharArray> outlineColors;
    outlineColors->DeepCopy(transformedColors);
    for (int c = 0; c < 3; ++c)
    {
      outlineColors->FillComponent(c, 0);
    }
    painter->DrawMarkers(markerStyle, false /*highlight*/, transformedPoints, outlineColors);
    painter->GetPen()->SetWidth(fillWidth);
  }
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
    this->Modified();
  }
}

void vtkPlotRadViz::SetDimensions(const int arg[4])
{
  this->SetDimensions(arg[0], arg[1], arg[2], arg[3]);
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::UpdateCenterAndRadius(vtkContext2D* painter)
{
  painter->ApplyTextProp(this->LabelsProperties);
  double marginX = 0.0;
  double marginY = 0.0;
  for (vtkIdType i = 0; i < this->VisibleLabels->GetNumberOfValues(); ++i)
  {
    float bounds[4];
    painter->ComputeStringBounds(this->VisibleLabels->GetValue(i), bounds);
    marginX = std::max(marginX, static_cast<double>(bounds[2]));
    marginY = std::max(marginY, static_cast<double>(bounds[3]));
  }
  double shift = (this->VerticesPen->GetWidth() / 2) * 1.2;
  marginX += shift;
  marginY += shift;

  int marginXi = static_cast<int>(std::lround(marginX));
  int marginYi = static_cast<int>(std::lround(marginY));

  int dimensions[4] = { this->Dimensions[0] + marginXi, this->Dimensions[1] + marginYi,
    std::max(this->Dimensions[2] - 2 * marginXi, 0),
    std::max(this->Dimensions[3] - 2 * marginYi, 0) };

  // the lens is stored in GBC space, so it follows the plot circle
  this->Internals->SetSceneCenterAndRadius(dimensions);
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::SetCustomRange(const std::vector<double>& customRange)
{
  ::ChangeFilter(
    this, this->ComputeGBC, [&](vtkMultiVariateToRGBA* f) { f->SetCustomRange(customRange); });
}

//------------------------------------------------------------------------------
std::vector<double> vtkPlotRadViz::GetCustomRange() VTK_FUTURE_CONST
{
  return this->ComputeGBC->GetCustomRange();
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::SetPartitionIndices(const std::vector<int>& partitionIndices)
{
  ::ChangeFilter(this, this->ComputeGBC,
    [&](vtkMultiVariateToRGBA* f) { f->SetPartitionIndices(partitionIndices); });
}

//------------------------------------------------------------------------------
std::vector<int> vtkPlotRadViz::GetPartitionIndices() VTK_FUTURE_CONST
{
  return this->ComputeGBC->GetPartitionIndices();
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::SetPartitionOffsets(const std::vector<int>& partitionOffsets)
{
  ::ChangeFilter(this, this->ComputeGBC,
    [&](vtkMultiVariateToRGBA* f) { f->SetPartitionOffsets(partitionOffsets); });
}

//------------------------------------------------------------------------------
std::vector<int> vtkPlotRadViz::GetPartitionOffsets() VTK_FUTURE_CONST
{
  return this->ComputeGBC->GetPartitionOffsets();
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::ClearPartitionIndices()
{
  ::ChangeFilter(
    this, this->ComputeGBC, [](vtkMultiVariateToRGBA* f) { f->ClearPartitionIndices(); });
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::AddPartitionIndex(int index)
{
  ::ChangeFilter(
    this, this->ComputeGBC, [&](vtkMultiVariateToRGBA* f) { f->AddPartitionIndex(index); });
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::ClearPartitionOffsets()
{
  ::ChangeFilter(
    this, this->ComputeGBC, [](vtkMultiVariateToRGBA* f) { f->ClearPartitionOffsets(); });
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::AddPartitionOffset(int offset)
{
  ::ChangeFilter(
    this, this->ComputeGBC, [&](vtkMultiVariateToRGBA* f) { f->AddPartitionOffset(offset); });
}

//------------------------------------------------------------------------------
void vtkPlotRadViz::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
