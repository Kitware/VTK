// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGridAxesActor2D.h"

#include "vtkActor2D.h"
#include "vtkAxis.h"
#include "vtkBatchedLabeledDataMapper.h"
#include "vtkBillboardTextActor3D.h"
#include "vtkContext2D.h"
#include "vtkContextScene.h"
#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkTextRenderer.h"

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
int JustificationToTextAnchor(int hJustify, int vJustify)
{
  using TA = vtkBatchedLabeledDataMapper;
  if (hJustify == VTK_TEXT_LEFT)
  {
    if (vJustify == VTK_TEXT_BOTTOM)
      return TA::LowerLeft;
    if (vJustify == VTK_TEXT_TOP)
      return TA::UpperLeft;
    return TA::LeftEdge;
  }
  else if (hJustify == VTK_TEXT_RIGHT)
  {
    if (vJustify == VTK_TEXT_BOTTOM)
      return TA::LowerRight;
    if (vJustify == VTK_TEXT_TOP)
      return TA::UpperRight;
    return TA::RightEdge;
  }
  else
  {
    if (vJustify == VTK_TEXT_BOTTOM)
      return TA::LowerEdge;
    if (vJustify == VTK_TEXT_TOP)
      return TA::UpperEdge;
    return TA::Center;
  }
}
}

class vtkGridAxesActor2D::vtkLabels
{
public:
  // One batch mapper+actor pair per edge for tick labels.
  vtkSmartPointer<vtkBatchedLabeledDataMapper> TickMappers[4];
  vtkSmartPointer<vtkActor2D> TickActors[4];
  vtkSmartPointer<vtkPolyData> TickPolyData[4];
  vtkSmartPointer<vtkStringArray> TickLabelArrays[4];
  int NumTicks[4] = { 0, 0, 0, 0 };

  // Title labels remain one-per-edge billboard actors.
  vtkNew<vtkBillboardTextActor3D> TitleLabels[4];
  vtkVector2i Justifications[4];

  vtkLabels()
  {
    for (int cc = 0; cc < 4; cc++)
    {
      this->TitleLabels[cc]->SetVisibility(0);

      this->TickPolyData[cc] = vtkSmartPointer<vtkPolyData>::New();
      this->TickLabelArrays[cc] = vtkSmartPointer<vtkStringArray>::New();
      this->TickLabelArrays[cc]->SetName("labels");

      this->TickMappers[cc] = vtkSmartPointer<vtkBatchedLabeledDataMapper>::New();
      this->TickMappers[cc]->SetLabelMode(VTK_LABEL_FIELD_DATA);
      this->TickMappers[cc]->SetFieldDataName("labels");
      this->TickMappers[cc]->SetInputData(this->TickPolyData[cc]);

      this->TickActors[cc] = vtkSmartPointer<vtkActor2D>::New();
      this->TickActors[cc]->SetMapper(this->TickMappers[cc]);
    }
  }

  void SyncPropertyKeys(vtkInformation* keys)
  {
    for (int cc = 0; cc < 4; cc++)
    {
      this->TickActors[cc]->SetPropertyKeys(keys);
      if (this->TitleLabels[cc]->GetVisibility())
      {
        this->TitleLabels[cc]->SetPropertyKeys(keys);
      }
    }
  }

  int RenderOpaqueGeometry(vtkViewport* viewport)
  {
    int counter = 0;
    for (int cc = 0; cc < 4; cc++)
    {
      if (this->NumTicks[cc] > 0)
      {
        counter += this->TickActors[cc]->RenderOpaqueGeometry(viewport);
      }
      if (this->TitleLabels[cc]->GetVisibility())
      {
        counter += this->TitleLabels[cc]->RenderOpaqueGeometry(viewport);
      }
    }
    return counter;
  }

  void UpdateGeometry(vtkViewport* viewport)
  {
    for (int cc = 0; cc < 4; cc++)
    {
      if (this->TitleLabels[cc]->GetVisibility())
      {
        this->TitleLabels[cc]->UpdateGeometry(viewport);
      }
    }
  }

  void GetActors(vtkPropCollection* props)
  {
    for (int cc = 0; cc < 4; cc++)
    {
      if (this->NumTicks[cc] > 0)
      {
        this->TickActors[cc]->GetActors(props);
      }
      if (this->TitleLabels[cc]->GetVisibility())
      {
        this->TitleLabels[cc]->GetActors(props);
      }
    }
  }

  int HasTranslucentPolygonalGeometry()
  {
    for (int cc = 0; cc < 4; cc++)
    {
      if (this->TitleLabels[cc]->GetVisibility())
      {
        if (this->TitleLabels[cc]->HasTranslucentPolygonalGeometry())
        {
          return 1;
        }
      }
    }
    return 0;
  }

  int RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
  {
    int counter = 0;
    for (int cc = 0; cc < 4; cc++)
    {
      if (this->TitleLabels[cc]->GetVisibility())
      {
        counter += this->TitleLabels[cc]->RenderTranslucentPolygonalGeometry(viewport);
      }
    }
    return counter;
  }

  int RenderOverlay(vtkViewport* viewport)
  {
    int counter = 0;
    for (int cc = 0; cc < 4; cc++)
    {
      if (this->TitleLabels[cc]->GetVisibility())
      {
        counter += this->TitleLabels[cc]->RenderOverlay(viewport);
      }
    }
    return counter;
  }

  void ReleaseGraphicsResources(vtkWindow* win)
  {
    for (int cc = 0; cc < 4; cc++)
    {
      this->TickMappers[cc]->ReleaseGraphicsResources(win);
      this->TitleLabels[cc]->ReleaseGraphicsResources(win);
    }
  }
};

vtkStandardNewMacro(vtkGridAxesActor2D);
//----------------------------------------------------------------------------
vtkGridAxesActor2D::vtkGridAxesActor2D()
  : Labels(new vtkGridAxesActor2D::vtkLabels())
  , DoRender(false)
{
  this->PlaneActor.TakeReference(vtkGridAxesPlaneActor2D::New(this->Helper.Get()));
  for (int cc = 0; cc < 3; cc++)
  {
    this->AxisHelpers[cc]->SetScene(this->AxisHelperScene.GetPointer());
    this->AxisHelpers[cc]->SetPosition(vtkAxis::LEFT);
    this->AxisHelpers[cc]->SetBehavior(vtkAxis::FIXED);
    this->TitleTextProperty[cc] = vtkSmartPointer<vtkTextProperty>::New();
    this->LabelTextProperty[cc] = vtkSmartPointer<vtkTextProperty>::New();
  }
}

//----------------------------------------------------------------------------
vtkGridAxesActor2D::~vtkGridAxesActor2D()
{
  delete this->Labels;
  this->Labels = nullptr;
}

//----------------------------------------------------------------------------
void vtkGridAxesActor2D::GetActors(vtkPropCollection* props)
{
  if (this->GetVisibility())
  {
    vtkViewport* vp = nullptr;
    if (this->NumberOfConsumers)
    {
      vp = vtkViewport::SafeDownCast(this->Consumers[0]);
      if (vp)
      {
        this->UpdateGeometry(vp, true);
      }
    }
    this->PlaneActor->GetActors(props);
    this->Labels->GetActors(props);
  }
}

//----------------------------------------------------------------------------
vtkMTimeType vtkGridAxesActor2D::GetMTime()
{
  vtkMTimeType mtime = this->Superclass::GetMTime();
  for (int cc = 0; cc < 3; cc++)
  {
    mtime = std::max(mtime, this->LabelTextProperty[cc]->GetMTime());
    mtime = std::max(mtime, this->TitleTextProperty[cc]->GetMTime());
  }

  return mtime;
}

//----------------------------------------------------------------------------
void vtkGridAxesActor2D::SetTitle(int axis, const std::string& title)
{
  if (axis >= 0 && axis < 3 && this->Titles[axis] != title)
  {
    this->Titles[axis] = title;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
const std::string& vtkGridAxesActor2D::GetTitle(int axis)
{
  static std::string nullstring;
  return (axis >= 0 && axis < 3) ? this->Titles[axis] : nullstring;
}

//----------------------------------------------------------------------------
void vtkGridAxesActor2D::SetNotation(int axis, int notation)
{
  if (axis >= 0 && axis < 3 && this->AxisHelpers[axis]->GetNotation() != notation)
  {
    this->AxisHelpers[axis]->SetNotation(notation);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkGridAxesActor2D::GetNotation(int axis)
{
  return (axis >= 0 && axis < 3) ? this->AxisHelpers[axis]->GetNotation() : vtkAxis::AUTO;
}

//----------------------------------------------------------------------------
void vtkGridAxesActor2D::SetPrecision(int axis, int precision)
{
  if (axis >= 0 && axis < 3 && this->AxisHelpers[axis]->GetPrecision() != precision)
  {
    this->AxisHelpers[axis]->SetPrecision(precision);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkGridAxesActor2D::GetPrecision(int axis)
{
  return (axis >= 0 && axis < 3) ? this->AxisHelpers[axis]->GetPrecision() : vtkAxis::AUTO;
}

//----------------------------------------------------------------------------
void vtkGridAxesActor2D::SetTickLabelFunction(int axis, std::function<double(double)> func)
{
  if (axis >= 0 && axis < 3)
  {
    this->TickLabelFunction[axis] = func;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
std::function<double(double)> vtkGridAxesActor2D::GetTickLabelFunction(int axis)
{
  return (axis >= 0 && axis < 3) ? this->TickLabelFunction[axis] : nullptr;
}

//----------------------------------------------------------------------------
void vtkGridAxesActor2D::SetProperty(vtkProperty* property)
{
  this->PlaneActor->SetProperty(property);
}

//----------------------------------------------------------------------------
vtkProperty* vtkGridAxesActor2D::GetProperty()
{
  return this->PlaneActor->GetProperty();
}

//----------------------------------------------------------------------------
void vtkGridAxesActor2D::SetTitleTextProperty(int axis, vtkTextProperty* tprop)
{
  if (axis >= 0 && axis < 3 && this->TitleTextProperty[axis] != tprop && tprop != nullptr)
  {
    this->TitleTextProperty[axis] = tprop;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkTextProperty* vtkGridAxesActor2D::GetTitleTextProperty(int axis)
{
  return (axis >= 0 && axis < 3) ? this->TitleTextProperty[axis] : nullptr;
}

//----------------------------------------------------------------------------
void vtkGridAxesActor2D::SetLabelTextProperty(int axis, vtkTextProperty* tprop)
{
  if (axis >= 0 && axis < 3 && this->LabelTextProperty[axis] != tprop && tprop != nullptr)
  {
    this->LabelTextProperty[axis] = tprop;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkTextProperty* vtkGridAxesActor2D::GetLabelTextProperty(int axis)
{
  return (axis >= 0 && axis < 3) ? this->LabelTextProperty[axis] : nullptr;
}

//----------------------------------------------------------------------------
void vtkGridAxesActor2D::SetCustomTickPositions(int axis, vtkDoubleArray* positions)
{
  if (axis >= 0 && axis < 3)
  {
    this->AxisHelpers[axis]->SetCustomTickPositions(positions);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkGridAxesActor2D::RenderOpaqueGeometry(vtkViewport* viewport)
{
  vtkRenderWindow* rWin = vtkRenderWindow::SafeDownCast(viewport->GetVTKWindow());
  if (rWin == nullptr || rWin->GetDesiredUpdateRate() < 1.0)
  {
    this->Update(viewport);
  }

  if (!this->DoRender)
  {
    return 0;
  }

  this->UpdateTextActors(viewport);

  int counter = 0;
  this->Labels->SyncPropertyKeys(this->GetPropertyKeys());
  counter += this->Labels->RenderOpaqueGeometry(viewport);
  counter += this->PlaneActor->RenderOpaqueGeometry(viewport);
  return counter;
}

//----------------------------------------------------------------------------
void vtkGridAxesActor2D::UpdateGeometry(vtkViewport* viewport, bool doRegularUpdate)
{
  if (doRegularUpdate)
  {
    vtkRenderWindow* rWin = vtkRenderWindow::SafeDownCast(viewport->GetVTKWindow());
    if (rWin == nullptr || rWin->GetDesiredUpdateRate() < 1.0)
    {
      this->Update(viewport);
    }
  }

  this->UpdateTextActors(viewport);

  this->Labels->UpdateGeometry(viewport);
  this->PlaneActor->UpdateGeometry(viewport);
}

//----------------------------------------------------------------------------
int vtkGridAxesActor2D::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  if (!this->DoRender)
  {
    return 0;
  }

  int counter = 0;
  this->Labels->SyncPropertyKeys(this->GetPropertyKeys());
  counter += this->Labels->RenderTranslucentPolygonalGeometry(viewport);
  counter += this->PlaneActor->RenderTranslucentPolygonalGeometry(viewport);
  return counter;
}

//----------------------------------------------------------------------------
int vtkGridAxesActor2D::RenderOverlay(vtkViewport* viewport)
{
  if (!this->DoRender)
  {
    return 0;
  }

  int counter = 0;
  this->Labels->SyncPropertyKeys(this->GetPropertyKeys());
  counter += this->Labels->RenderOverlay(viewport);
  counter += this->PlaneActor->RenderOverlay(viewport);
  return counter;
}

//----------------------------------------------------------------------------
vtkTypeBool vtkGridAxesActor2D::HasTranslucentPolygonalGeometry()
{
  return (this->PlaneActor->HasTranslucentPolygonalGeometry() ||
           this->Labels->HasTranslucentPolygonalGeometry())
    ? 1
    : 0;
}

//----------------------------------------------------------------------------
void vtkGridAxesActor2D::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Labels->ReleaseGraphicsResources(win);
  this->PlaneActor->ReleaseGraphicsResources(win);
  this->Superclass::ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
bool vtkGridAxesActor2D::Update(vtkViewport* viewport)
{
  this->Helper->SetGridBounds(this->GridBounds);
  this->Helper->SetFace(this->Face);
  this->Helper->SetMatrix(this->GetMatrix());
  this->Helper->SetLabelMask(this->LabelMask);
  this->PlaneActor->SetUserMatrix(this->GetMatrix());

  vtkRenderer* renderer = vtkRenderer::SafeDownCast(viewport);
  assert(renderer != nullptr);

  // This is needed so the vtkAxis labels account for tile scaling.
  this->AxisHelperScene->SetRenderer(renderer);

  if ((this->Helper->UpdateForViewport(viewport) == false) ||
    (this->GetProperty()->GetBackfaceCulling() && this->Helper->GetBackface()) ||
    (this->GetProperty()->GetFrontfaceCulling() && !this->Helper->GetBackface()))
  {
    this->DoRender = false;
    return false;
  }
  this->DoRender = true;
  this->UpdateTextProperties(viewport);
  this->UpdateLabelPositions(viewport);

  return this->DoRender;
}

//----------------------------------------------------------------------------
void vtkGridAxesActor2D::UpdateTextProperties(vtkViewport*)
{
  // Update text properties.
  if (this->GetMTime() < this->UpdateLabelTextPropertiesMTime)
  {
    return;
  }
  for (int cc = 0; cc < 3; cc++)
  {
    // Pass the current text properties to the vtkAxis
    // objects so they can place the labels appropriately using the current
    // label text properties.
    this->AxisHelpers[cc]->GetLabelProperties()->ShallowCopy(this->LabelTextProperty[cc]);
  }

  const vtkVector2i& activeAxes = this->Helper->GetActiveAxes();
  for (int cc = 0; cc < 4; cc++)
  {
    this->Labels->TitleLabels[cc]->GetTextProperty()->ShallowCopy(
      this->TitleTextProperty[activeAxes[cc % 2]]);
  }

  this->UpdateLabelTextPropertiesMTime.Modified();
}

//----------------------------------------------------------------------------
void vtkGridAxesActor2D::UpdateLabelPositions(vtkViewport*)
{
  const vtkVector2i& activeAxes = this->Helper->GetActiveAxes();
  const vtkTuple<vtkVector2d, 4> axisVectors = this->Helper->GetViewportVectors();
  const vtkTuple<vtkVector2d, 4> axisNormals = this->Helper->GetViewportNormals();
  const vtkTuple<bool, 4>& labelVisibilties = this->Helper->GetLabelVisibilities();

  vtkAxis* activeAxisHelpers[2];
  activeAxisHelpers[0] = this->AxisHelpers[activeAxes[0]].GetPointer();
  activeAxisHelpers[1] = this->AxisHelpers[activeAxes[1]].GetPointer();

  // Determine the number of labels to place and the text to use for them.
  for (int cc = 0; cc < 2; cc++)
  {
    if (std::abs(axisVectors[cc].GetX()) > std::abs(axisVectors[cc].GetY()))
    {
      // normal is more horizontal hence axis is more vertical.
      activeAxisHelpers[cc]->SetPoint1(0, 0);
      activeAxisHelpers[cc]->SetPoint2(std::abs(axisVectors[cc].GetX()), 0);
      activeAxisHelpers[cc]->SetPosition(vtkAxis::BOTTOM);
    }
    else
    {
      activeAxisHelpers[cc]->SetPoint1(0, 0);
      activeAxisHelpers[cc]->SetPoint2(0, std::abs(axisVectors[cc].GetY()));
      activeAxisHelpers[cc]->SetPosition(vtkAxis::LEFT);
    }
  }
  activeAxisHelpers[0]->SetUnscaledRange(
    this->GridBounds[2 * activeAxes.GetX()], this->GridBounds[(2 * activeAxes.GetX()) + 1]);
  activeAxisHelpers[1]->SetUnscaledRange(
    this->GridBounds[2 * activeAxes.GetY()], this->GridBounds[(2 * activeAxes.GetY()) + 1]);

  activeAxisHelpers[0]->Update();
  activeAxisHelpers[1]->Update();

  // Tell the plane actor where we've decided to place the labels.
  this->PlaneActor->SetTickPositions(activeAxes[0], activeAxisHelpers[0]->GetTickPositions());
  this->PlaneActor->SetTickPositions(activeAxes[1], activeAxisHelpers[1]->GetTickPositions());

  //----------------------------------------------------------------------------------------
  // Now compute label justifications to control their placement.
  vtkVector2d xaxis(1, 0);
  vtkVector2d yaxis(0, 1);
  for (int cc = 0; cc < 4; cc++)
  {
    this->Labels->Justifications[cc].SetX(VTK_TEXT_CENTERED);
    this->Labels->Justifications[cc].SetY(VTK_TEXT_CENTERED);
    if (!labelVisibilties[cc])
    {
      continue;
    }

    const vtkVector2d& axisNormal = axisNormals[cc];
    double cosTheta = axisNormal.Dot(xaxis);
    double sinTheta = axisNormal.Dot(yaxis);

    if (std::abs(axisVectors[cc].GetX()) > std::abs(axisVectors[cc].GetY()))
    {
      // horizontal axis.

      // Sin() will be +'ve for labels on top, -'ve for labels on bottom of
      // axis. Thus vertical justification will be bottom and top respectively.
      this->Labels->Justifications[cc].SetY(sinTheta >= 0 ? VTK_TEXT_BOTTOM : VTK_TEXT_TOP);

      if (std::abs(cosTheta) < 0.342020143) // math.sin(math.radians(20))
      {
        // very vertical.
        this->Labels->Justifications[cc].SetX(VTK_TEXT_CENTERED);
      }
      else
      {
        // Cos() +'ve ==> labels right of axis and -'ve for labels on left i.e.
        // with horizontal justification set to left and right respectively.
        this->Labels->Justifications[cc].SetX(cosTheta >= 0 ? VTK_TEXT_LEFT : VTK_TEXT_RIGHT);
      }
    }
    else
    {
      // vertical axis.

      // Cos() will be +'ve for labels on right of axis, while -ve for labels on left of axis.
      this->Labels->Justifications[cc].SetX(cosTheta >= 0 ? VTK_TEXT_LEFT
                                                          : // anchor left i.e. label on right
          VTK_TEXT_RIGHT);                                  // anchor right

      if (std::abs(sinTheta) < 0.342020143) // math.sin(math.radians(20))
      {
        this->Labels->Justifications[cc].SetY(VTK_TEXT_CENTERED);
      }
      else
      {
        // Sin() +'ve => labels on top of axis, -'ve labels on bottom of axis.
        this->Labels->Justifications[cc].SetY(sinTheta >= 0 ? VTK_TEXT_BOTTOM
                                                            : // anchor bottom i.e. labels on top
            VTK_TEXT_TOP);                                    // anchor top.
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkGridAxesActor2D::UpdateTextActors(vtkViewport* viewport)
{
  const vtkTuple<vtkVector3d, 4>& gridPoints = this->Helper->GetPoints();
  const vtkVector2i& activeAxes = this->Helper->GetActiveAxes();
  const vtkTuple<bool, 4>& labelVisibilties = this->Helper->GetLabelVisibilities();
  const vtkTuple<vtkVector2d, 4>& axisNormals = this->Helper->GetViewportNormals();
  const vtkTuple<vtkVector3d, 4> facePoints = this->Helper->GetTransformedPoints();

  vtkAxis* activeAxisHelpers[2];
  activeAxisHelpers[0] = this->AxisHelpers[activeAxes[0]].GetPointer();
  activeAxisHelpers[1] = this->AxisHelpers[activeAxes[1]].GetPointer();

  vtkWindow* renWin = viewport->GetVTKWindow();
  int tileScale[2];
  renWin->GetTileScale(tileScale);

  for (int index = 0; index < 4; index++)
  {
    int axis = index % 2;
    int activeAxis = activeAxes[axis];
    vtkStringArray* labels = activeAxisHelpers[axis]->GetTickLabels();
    if (labels->GetNumberOfValues() == 0)
    {
      continue;
    }
    vtkDoubleArray* tickPositions = activeAxisHelpers[axis]->GetTickPositions();
    vtkIdType numTicks = labelVisibilties[index] ? tickPositions->GetNumberOfTuples() : 0;

    this->Labels->NumTicks[index] = static_cast<int>(numTicks);
    if (numTicks == 0)
    {
      continue;
    }

    /// XXX: improve this.
    vtkVector2i offset(vtkContext2D::FloatToInt((axisNormals[index].GetX() * 10 * tileScale[0]) +
                         this->LabelDisplayOffset[axis]),
      vtkContext2D::FloatToInt(
        (axisNormals[index].GetY() * 10 * tileScale[1]) + this->LabelDisplayOffset[axis]));

    // The geometry shader in vtk(OpenGL/WebGPU)BatchedLabeledDataMapperInternals offsets text
    // away from the anchor by `1 + descender` pixels for left/right text anchors and by
    // descender (or descender / 2) pixels for top/center vertical anchors, so the
    // frame edge sits at the anchor rather than the glyph edge. The old per-tick
    // vtkBillboardTextActor3D placement put the glyph edge at the anchor with no margin,
    // so without compensation the new tick labels drift toward the title. Compensate by
    // shifting DisplayOffset back the same amount in the opposite direction.
    vtkTextProperty* labelProp = activeAxisHelpers[axis]->GetLabelProperties();
    int descender = 0;
    if (vtkTextRenderer* tren = vtkTextRenderer::GetInstance())
    {
      vtkTextRenderer::Metrics metrics;
      if (tren->GetMetrics(labelProp, "Mp", metrics, renWin->GetDPI()))
      {
        descender = std::abs(metrics.Descent.GetY());
      }
    }
    const int hJust = this->Labels->Justifications[index].GetX();
    const int vJust = this->Labels->Justifications[index].GetY();
    int compX = 0, compY = 0;
    if (hJust == VTK_TEXT_LEFT)
    {
      compX = -(1 + descender);
    }
    else if (hJust == VTK_TEXT_RIGHT)
    {
      compX = (1 + descender);
    }
    if (vJust == VTK_TEXT_TOP)
    {
      compY = descender;
    }
    else if (vJust == VTK_TEXT_CENTERED)
    {
      compY = descender / 2;
    }

    // Configure the batch label mapper for this edge.
    vtkBatchedLabeledDataMapper* mapper = this->Labels->TickMappers[index];
    mapper->SetLabelTextProperty(labelProp);
    mapper->SetTextAnchor(JustificationToTextAnchor(hJust, vJust));
    int offsetArr[2] = { offset.GetX() + compX, offset.GetY() + compY };
    mapper->SetDisplayOffset(offsetArr);

    // Build polydata with tick world-coordinate positions and label strings.
    auto pts = vtkSmartPointer<vtkPoints>::New();
    pts->SetNumberOfPoints(numTicks);
    vtkStringArray* labelArr = this->Labels->TickLabelArrays[index];
    labelArr->SetNumberOfValues(numTicks);

    for (vtkIdType cc = 0; cc < numTicks; ++cc)
    {
      vtkVector3d tickPosition = gridPoints[index];
      tickPosition[activeAxes[axis]] = tickPositions->GetValue(cc);
      vtkVector3d tickWC = this->Helper->TransformPoint(tickPosition);
      pts->SetPoint(cc, tickWC.GetData());

      if (!labels->GetValue(cc).empty() && this->TickLabelFunction[activeAxis] != nullptr)
      {
        const double tickValue = this->TickLabelFunction[activeAxis](tickPositions->GetValue(cc));
        labelArr->SetValue(cc, activeAxisHelpers[axis]->GenerateSimpleLabel(tickValue).c_str());
      }
      else
      {
        labelArr->SetValue(cc, labels->GetValue(cc).c_str());
      }
    }

    vtkPolyData* pd = this->Labels->TickPolyData[index];
    pd->Initialize();
    pd->SetPoints(pts);
    pd->GetPointData()->AddArray(labelArr);
    pd->Modified();
  }

  for (int index = 0; index < 4; index++)
  {
    // Setup title text.
    vtkBillboardTextActor3D* titleActor = this->Labels->TitleLabels[index].GetPointer();
    const std::string& label = this->Titles[activeAxes[index % 2]];
    if (label.empty() == false && labelVisibilties[index])
    {
      vtkVector3d midPoint = (facePoints[index] + facePoints[(index + 1) % 4]) * 0.5;
      /// XXX: improve this.
      vtkVector2i offset(vtkContext2D::FloatToInt(axisNormals[index].GetX() * 30 * tileScale[0]),
        vtkContext2D::FloatToInt(axisNormals[index].GetY() * 30 * tileScale[1]));
      titleActor->SetInput(label.c_str());
      titleActor->GetTextProperty()->SetJustification(this->Labels->Justifications[index].GetX());
      titleActor->GetTextProperty()->SetVerticalJustification(
        this->Labels->Justifications[index].GetY());
      titleActor->SetPosition(midPoint.GetData());
      titleActor->SetDisplayOffset(offset.GetData());
      titleActor->SetVisibility(1);
      titleActor->SetForceOpaque(this->ForceOpaque);
    }
    else
    {
      titleActor->SetVisibility(0);
    }
  }
}

//----------------------------------------------------------------------------
void vtkGridAxesActor2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
