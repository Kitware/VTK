// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCameraOrientationRepresentation.h"

#include "vtkActor.h"
#include "vtkAssemblyPath.h"
#include "vtkBoundingBox.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDiskSource.h"
#include "vtkDoubleArray.h"
#include "vtkEllipticalButtonSource.h"
#include "vtkFreeTypeTools.h"
#include "vtkImageData.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkObject.h"
#include "vtkObjectFactory.h"
#include "vtkPickingManager.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp3D.h"
#include "vtkPropCollection.h"
#include "vtkPropPicker.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkTubeFilter.h"
#include "vtkVectorText.h"

#include <algorithm>
#include <cstddef>
#include <type_traits>

#define GETLABELPROPERTY(DIM, DIR)                                                                 \
  vtkTextProperty* vtkCameraOrientationRepresentation::Get##DIM##DIR##LabelProperty()              \
  {                                                                                                \
    const auto& dim = to_underlying(HandleDimType::DIM);                                           \
    const auto& dir = to_underlying(HandleDirType::DIR);                                           \
    return this->AxisVectorTextProperties[dim][dir];                                               \
  }

//-----------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkCameraOrientationRepresentation);

//-----------------------------------------------------------------------------
namespace
{
const double xyzBaseColor[3][3] = { { 0.870, 0.254, 0.188 }, { 0.952, 0.752, 0.090 },
  { 0.654, 0.823, 0.549 } };
const double minusXyzTextBgColor[3][3] = { { 0.655, 0.157, 0.106 }, { 0.898, 0.698, 0.047 },
  { 0.49, 0.737, 0.333 } };

enum class HandleDirType : int
{
  Plus,
  Minus
};

enum class HandleDimType : int
{
  X,
  Y,
  Z
};

template <typename EnumT>
constexpr typename std::underlying_type<EnumT>::type to_underlying(const EnumT& e) noexcept
{
  return static_cast<typename std::underlying_type<EnumT>::type>(e);
}
}

//-----------------------------------------------------------------------------
vtkCameraOrientationRepresentation::vtkCameraOrientationRepresentation()
{
  this->InteractionState = to_underlying(InteractionStateType::Outside);
  this->PickingManaged = true;

  this->Points->SetDataTypeToDouble();
  this->Points->SetNumberOfPoints(13);

  // 1. Shafts
  vtkNew<vtkCellArray> lines;
  lines->InsertNextCell({ 0, 1 });
  lines->InsertNextCell({ 0, 3 });
  lines->InsertNextCell({ 0, 5 });
  this->Skeleton->SetLines(lines);

  // 2. Handles
  this->Skeleton->SetPoints(this->Points);
  this->CreateDefaultGeometry();
  this->PositionHandles();

  // 3. init container source shape.
  this->ContainerSource->SetCircumferentialResolution(this->ContainerCircumferentialResolution);
  this->ContainerSource->SetRadialResolution(this->ContainerRadialResolution);
  this->ContainerSource->SetInnerRadius(0.);
  this->ContainerSource->SetOuterRadius(this->TotalLength);

  // 4. init handle source shapes.
  for (int ax = 0; ax < 3; ++ax)
  {
    for (int dir = 0; dir < 2; ++dir)
    {
      const auto& handleSrc = this->HandleSources[ax][dir];
      handleSrc->SetCircumferentialResolution(this->HandleCircumferentialResolution);
      handleSrc->SetShoulderResolution(8);
      handleSrc->SetTextureResolution(32);
      handleSrc->SetTextureStyleToFitImage(); // prevents horizontally stretched "x", "y", "z"
      handleSrc->SetRadialRatio(dir == to_underlying(HandleDirType::Plus) ? 1.2 : 1.0);
      handleSrc->SetDepth(0.05);
    }
  }

  // 5. Init shafts
  this->ShaftGlyphs->SetRadius(0.02);
  this->ShaftGlyphs->SetNumberOfSides(this->ShaftResolution);
  this->ShaftGlyphs->SetInputData(this->Skeleton);

  // 6. properties
  this->CreateDefaultProperties();

  // 7. Picker
  for (int ax = 0; ax < 3; ++ax)
  {
    for (int dir = 0; dir < 2; ++dir)
    {
      this->HandlePicker->AddPickList(this->Handles[ax][dir]);
    }
  }
  this->HandlePicker->PickFromListOn();
}

//-----------------------------------------------------------------------------
vtkCameraOrientationRepresentation::~vtkCameraOrientationRepresentation() = default;

//-----------------------------------------------------------------------------
void vtkCameraOrientationRepresentation::PositionHandles()
{
  // transform skeleton.
  auto data = vtkDoubleArray::SafeDownCast(this->Points->GetData());
  for (int i = 0; i < 13; ++i)
  {
    double pos[3] = {}, newPos[3] = {};
    data->GetTypedTuple(i, pos);
    this->Transform->TransformPoint(pos, newPos);
    data->SetTypedTuple(i, newPos);
  }

  // update handle positions to transformed coords.
  const double* const points = data->GetPointer(0);
  for (int ax = 0, ptId = 7; ax < 3; ++ax)
  {
    for (int dir = 0; dir < 2; ++dir, ++ptId)
    {
      const auto& handleSrc = this->HandleSources[ax][dir];
      handleSrc->SetWidth(this->NormalizedHandleDia);
      handleSrc->SetHeight(this->NormalizedHandleDia);
      handleSrc->SetCenter(points + static_cast<std::ptrdiff_t>(ptId * 3));
    }
  }

  // now compute new back, up vectors from originals.
  double up[3] = { 0., 1., 0. }, back[3] = { 0., 0., -1. };

  // get world positions of +x, +y, +z handles
  // dot them with og back and up to obtain new directions.
  const double* xyzHandles[3] = { nullptr, nullptr, nullptr };
  for (int i = 0, j = 1; i < 3 && j < 7; ++i, j += 2)
  {
    xyzHandles[i] = points + static_cast<std::ptrdiff_t>(j * 3);
    this->Back[i] = vtkMath::Dot(xyzHandles[i], back);
    this->Up[i] = vtkMath::Dot(xyzHandles[i], up);
  }
  vtkMath::Normalize(this->Back);
  vtkMath::Normalize(this->Up);

  this->Points->Modified();
  this->Skeleton->Modified();
}

//-----------------------------------------------------------------------------
void vtkCameraOrientationRepresentation::CreateDefaultGeometry()
{
  // 1. Set positions of 6 handles.
  const double sl = this->TotalLength * (1. - this->NormalizedHandleDia); // shaft length
  const double hr = this->TotalLength * this->NormalizedHandleDia * 0.5;  // handle radius
  auto data = vtkDoubleArray::SafeDownCast(this->Points->GetData());
  data->FillValue(0);
  double* const coords = data->GetPointer(0);
  // 2. Shafts
  for (int i = 3; i < 18; i += 7)
  {
    coords[i] = sl;
    coords[i + 3] = -sl;
  }

  // 3. Handle centers
  for (int i = 21; i < 36; i += 7)
  {
    coords[i] = sl + hr;
    coords[i + 3] = -sl - hr;
  }

  this->Points->Modified();
  this->Skeleton->Modified();
}

//-----------------------------------------------------------------------------
void vtkCameraOrientationRepresentation::CreateDefaultProperties()
{
  // 1. Fill color arrays.
  this->AxesColors->SetNumberOfComponents(3);
  this->AxesColors->SetNumberOfTuples(3);
  for (int ax = 0; ax < 3; ++ax)
  {
    this->AxesColors->SetTypedTuple(ax, xyzBaseColor[ax]);
  }

  // 2. Set color arrays.
  this->Skeleton->GetCellData()->SetScalars(this->AxesColors);

  // 3. assign mappers to those that require.
  vtkNew<vtkPolyDataMapper> containerMapper;
  containerMapper->SetInputConnection(this->ContainerSource->GetOutputPort());
  this->Container->SetMapper(containerMapper);
  this->Container->SetVisibility(false);

  vtkNew<vtkPolyDataMapper> shaftMapper;
  shaftMapper->SetColorModeToDirectScalars();
  shaftMapper->SetInputConnection(this->ShaftGlyphs->GetOutputPort());
  this->Shafts->SetMapper(shaftMapper);

  for (int ax = 0; ax < 3; ++ax)
  {
    for (int dir = 0; dir < 2; ++dir)
    {
      const auto& handle = this->Handles[ax][dir];
      const auto& handleSrc = this->HandleSources[ax][dir];
      const auto& labelTextProperty = this->AxisVectorTextProperties[ax][dir];
      const auto& labelTexture = this->LabelTextures[ax][dir];
      const auto& labelImage = this->LabelImages[ax][dir];

      vtkNew<vtkPolyDataMapper> handleMapper;
      handleMapper->SetInputConnection(handleSrc->GetOutputPort());
      handle->SetMapper(handleMapper);

      if (dir == to_underlying(HandleDirType::Minus))
      {
        labelTextProperty->SetBackgroundColor(minusXyzTextBgColor[ax]);
      }
      else if (dir == to_underlying(HandleDirType::Plus))
      {
        labelTextProperty->SetBackgroundColor(xyzBaseColor[ax]);
      }
      labelTextProperty->SetBackgroundOpacity(1.0);
      labelTextProperty->SetJustificationToCentered();
      labelTextProperty->SetVerticalJustificationToCentered();
      labelTextProperty->SetFontFamilyToArial();
      labelTextProperty->SetFontSize(128);
      labelTextProperty->BoldOn();
      labelTexture->SetInputData(labelImage);
    }
  }

  // 4. Container is transparent
  this->Container->GetProperty()->SetColor(0.2, 0.2, 0.2);
  this->Container->GetProperty()->SetOpacity(0.1);

  // 5. Remove reflections, shadows on the container, handles and shafts
  this->Container->GetProperty()->SetAmbient(1);
  this->Container->GetProperty()->SetDiffuse(0);
  this->Shafts->GetProperty()->SetAmbient(1);
  this->Shafts->GetProperty()->SetDiffuse(0);
  for (int ax = 0; ax < 3; ++ax)
  {
    for (int dir = 0; dir < 2; ++dir)
    {
      const auto& handle = this->Handles[ax][dir];
      handle->GetProperty()->SetAmbient(1);
      handle->GetProperty()->SetDiffuse(0);
    }
  }
}

//-----------------------------------------------------------------------------
void vtkCameraOrientationRepresentation::ShallowCopy(vtkProp* prop)
{
  vtkCameraOrientationRepresentation* a = vtkCameraOrientationRepresentation::SafeDownCast(prop);
  if (a != nullptr)
  {
    for (int ax = 0; ax < 3; ++ax)
    {
      for (int dir = 0; dir < 2; ++dir)
      {
        a->Points->ShallowCopy(this->Points);
        a->Skeleton->ShallowCopy(this->Skeleton);
        a->AxesColors->ShallowCopy(this->AxesColors);
        a->Shafts->ShallowCopy(this->Shafts);
        a->Handles[ax][dir]->ShallowCopy(this->Handles[ax][dir]);
        a->Container->ShallowCopy(this->Container);
        a->AxisVectorTextProperties[ax][dir]->ShallowCopy(this->AxisVectorTextProperties[ax][dir]);
      }
    }
  }

  // Now do superclass
  this->Superclass::ShallowCopy(prop);
}

//------------------------------------------------------------------------------
void vtkCameraOrientationRepresentation::ApplyInteractionState(const InteractionStateType& state)
{
  // Depending on state, show/hide parts of representation
  switch (state)
  {
    case InteractionStateType::Hovering:
      this->Container->SetVisibility(true);
      break;
    case InteractionStateType::Rotating:
      this->Container->SetVisibility(true);
      break;
    default: // outside
      this->Container->SetVisibility(false);
      break;
  }
  this->InteractionState = to_underlying(state);
}

//-----------------------------------------------------------------------------
void vtkCameraOrientationRepresentation::ApplyInteractionState(const int& state)
{
  // Clamp to allowable values
  const int clamped = state < 0 ? 0 : (state > 2 ? 2 : state);
  this->ApplyInteractionState(static_cast<InteractionStateType>(clamped));
}

//-----------------------------------------------------------------------------
void vtkCameraOrientationRepresentation::GetActors(vtkPropCollection* ac)
{
  if (ac != nullptr && this->GetVisibility())
  {
    ac->AddItem(this->Container);
    for (int ax = 0; ax < 3; ++ax)
    {
      ac->AddItem(this->Shafts);
      for (int dir = 0; dir < 2; ++dir)
      {
        ac->AddItem(this->Handles[ax][dir]);
      }
    }
  }
  this->Superclass::GetActors(ac);
}

//-----------------------------------------------------------------------------
void vtkCameraOrientationRepresentation::BuildRepresentation()
{
  // Rebuild only if necessary
  if ((this->GetMTime() > this->BuildTime) || (this->Transform->GetMTime() >= this->BuildTime))
  {
    this->CreateDefaultGeometry();
    this->PositionHandles();
    this->HighlightHandle();
    this->BuildTime.Modified();
  }
}

//-----------------------------------------------------------------------------
int vtkCameraOrientationRepresentation::ComputeInteractionState(int X, int Y, int modify /* = 0*/)
{
  // compute interaction state.
  if (modify)
  {
    if (!this->Renderer || !this->Renderer->IsInViewport(X, Y))
    {
      this->InteractionState = to_underlying(InteractionStateType::Outside);
    }
    else
    {
      this->InteractionState = to_underlying(InteractionStateType::Hovering);
    }
  }

  if (this->GetInteractionStateAsEnum() == InteractionStateType::Rotating)
  {
    return this->InteractionState;
  }

  this->PickedAxis = -1;
  this->PickedDir = -1;
  if (this->GetInteractionStateAsEnum() == InteractionStateType::Outside)
  {
    return this->InteractionState;
  }
  else
  {
    // do picking.
    vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0.0, this->HandlePicker);
    vtkActor* handle = nullptr;
    if (path != nullptr)
    {
      this->ValidPick = 1;
      handle = vtkActor::SafeDownCast(path->GetFirstNode()->GetViewProp());
      for (int ax = 0; ax < 3; ++ax)
      {
        for (int dir = 0; dir < 2; ++dir)
        {
          if (handle == this->Handles[ax][dir])
          {
            this->PickedDir = dir;
            this->PickedAxis = ax;
          }
        }
      }
    }
  }
  return this->InteractionState;
}

//-----------------------------------------------------------------------------
void vtkCameraOrientationRepresentation::StartWidgetInteraction(double eventPos[2])
{
  this->StartEventPosition[0] = eventPos[0];
  this->StartEventPosition[1] = eventPos[1];
  this->StartEventPosition[2] = 0;

  this->LastEventPosition[0] = eventPos[0];
  this->LastEventPosition[1] = eventPos[1];
  this->LastEventPosition[2] = 0;
}

//-----------------------------------------------------------------------------
void vtkCameraOrientationRepresentation::WidgetInteraction(double newEventPos[2])
{
  if (this->Renderer == nullptr)
  {
    return;
  }
  this->Rotate(newEventPos);

  this->LastEventPosition[0] = newEventPos[0];
  this->LastEventPosition[1] = newEventPos[1];
  this->LastEventPosition[2] = 0;
}

//-----------------------------------------------------------------------------
void vtkCameraOrientationRepresentation::EndWidgetInteraction(double newEventPos[2])
{
  if (this->GetInteractionStateAsEnum() == InteractionStateType::Rotating)
  {
    this->PickedAxis = -1;
    this->PickedDir = -1;
    this->LastPickedAx = -1;
    this->LastPickedDir = -1;
    return;
  }

  this->FinalizeHandlePicks();

  this->LastEventPosition[0] = newEventPos[0];
  this->LastEventPosition[1] = newEventPos[1];
  this->LastEventPosition[2] = 0;
}

void vtkCameraOrientationRepresentation::Rotate(double newEventPos[2])
{
  this->InteractionState = to_underlying(InteractionStateType::Rotating);

  int dx = newEventPos[0] - this->LastEventPosition[0];
  int dy = newEventPos[1] - this->LastEventPosition[1];

  const int* size = this->Renderer->GetSize();

  // permit 90 degree rotation across renderer w, h
  double delta_azimuth = -90.0 / size[0];
  double delta_elevation = -90.0 / size[1];

  this->Azimuth = dx * delta_azimuth * this->MotionFactor;
  this->Elevation = dy * delta_elevation * this->MotionFactor;
}

//-----------------------------------------------------------------------------
void vtkCameraOrientationRepresentation::FinalizeHandlePicks()
{
  if (this->InteractionState ==
    to_underlying(vtkCameraOrientationRepresentation::InteractionStateType::Hovering))
  {
    if ((this->LastPickedAx == this->PickedAxis) && (this->LastPickedAx != -1))
    {
      if ((this->LastPickedDir == this->PickedDir) && (this->LastPickedDir != -1))
      {
        // Decent UX. Select the other direction grabber when +, - grabbers for same axis
        // overlap.
        this->PickedDir = !this->LastPickedDir;
      }
    }
  }

  switch (this->PickedDir)
  {
    case to_underlying(HandleDirType::Plus):
    {
      switch (this->PickedAxis)
      {
        case to_underlying(HandleDimType::X):
        {
          this->Back[0] = -1.;
          this->Back[1] = 0.;
          this->Back[2] = 0.;
          this->Up[0] = 0.;
          this->Up[1] = 0.;
          this->Up[2] = 1.;
          break;
        }
        case to_underlying(HandleDimType::Y):
        {
          this->Back[0] = 0.;
          this->Back[1] = -1.;
          this->Back[2] = 0.;
          this->Up[0] = 0.;
          this->Up[1] = 0.;
          this->Up[2] = 1.;
          break;
        }
        case to_underlying(HandleDimType::Z):
        {
          this->Back[0] = 0.;
          this->Back[1] = 0.;
          this->Back[2] = -1.;
          this->Up[0] = 0.;
          this->Up[1] = 1.;
          this->Up[2] = 0.;
          break;
        }
        default:
          break;
      }
      break;
    }
    case to_underlying(HandleDirType::Minus):
    {
      switch (this->PickedAxis)
      {
        case to_underlying(HandleDimType::X):
        {
          this->Back[0] = 1.;
          this->Back[1] = 0.;
          this->Back[2] = 0.;
          this->Up[0] = 0.;
          this->Up[1] = 0.;
          this->Up[2] = 1.;
          break;
        }
        case to_underlying(HandleDimType::Y):
        {
          this->Back[0] = 0.;
          this->Back[1] = 1.;
          this->Back[2] = 0.;
          this->Up[0] = 0.;
          this->Up[1] = 0.;
          this->Up[2] = 1.;
          break;
        }
        case to_underlying(HandleDimType::Z):
        {
          this->Back[0] = 0.;
          this->Back[1] = 0.;
          this->Back[2] = 1.;
          this->Up[0] = 0.;
          this->Up[1] = 1.;
          this->Up[2] = 0.;
          break;
        }
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
  this->LastPickedAx = this->PickedAxis;
  this->LastPickedDir = this->PickedDir;
}

//-----------------------------------------------------------------------------
double* vtkCameraOrientationRepresentation::GetBounds()
{
  vtkBoundingBox bbox;
  bbox.SetBounds(this->Container->GetBounds());
  bbox.AddBounds(this->Shafts->GetBounds());

  for (int ax = 0; ax < 3; ++ax)
  {
    for (int dir = 0; dir < 2; ++dir)
    {
      bbox.AddBounds(this->Handles[ax][dir]->GetBounds());
    }
  }
  bbox.GetBounds(this->Bounds);
  return this->Bounds;
}

//-----------------------------------------------------------------------------
vtkTransform* vtkCameraOrientationRepresentation::GetTransform()
{
  return this->Transform;
}

//-----------------------------------------------------------------------------
int vtkCameraOrientationRepresentation::RenderOpaqueGeometry(vtkViewport* vp)
{
  this->BuildRepresentation();

  int count = 0;

  if (this->Container->GetVisibility())
  {
    this->Container->SetPropertyKeys(this->GetPropertyKeys());
    this->Container->GetMapper()->Update();
    count += this->Container->RenderOpaqueGeometry(vp);
  }

  this->Shafts->SetPropertyKeys(this->GetPropertyKeys());
  this->Shafts->GetMapper()->Update();
  count += this->Shafts->RenderOpaqueGeometry(vp);

  for (int ax = 0; ax < 3; ++ax)
  {
    for (int dir = 0; dir < 2; ++dir)
    {
      const auto& handle = this->Handles[ax][dir];
      handle->SetPropertyKeys(this->GetPropertyKeys());
      handle->GetMapper()->Update();
      count += handle->RenderOpaqueGeometry(vp);
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
int vtkCameraOrientationRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* vp)
{
  int count = 0;

  if (this->Container->GetVisibility())
  {
    this->Container->SetPropertyKeys(this->GetPropertyKeys());
    this->Container->GetMapper()->Update();
    count += this->Container->RenderTranslucentPolygonalGeometry(vp);
  }

  this->Shafts->SetPropertyKeys(this->GetPropertyKeys());
  this->Shafts->GetMapper()->Update();
  count += this->Shafts->RenderTranslucentPolygonalGeometry(vp);

  for (int ax = 0; ax < 3; ++ax)
  {
    for (int dir = 0; dir < 2; ++dir)
    {
      const auto& handle = this->Handles[ax][dir];
      handle->SetPropertyKeys(this->GetPropertyKeys());
      handle->GetMapper()->Update();
      count += handle->RenderTranslucentPolygonalGeometry(vp);
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
vtkTypeBool vtkCameraOrientationRepresentation::HasTranslucentPolygonalGeometry()
{
  this->Container->GetMapper()->Update();
  int count = 0;
  if (this->Container->GetVisibility())
  {
    count |= this->Container->HasTranslucentPolygonalGeometry();
  }
  count |= this->Shafts->HasTranslucentPolygonalGeometry();
  for (int ax = 0; ax < 3; ++ax)
  {
    for (int dir = 0; dir < 2; ++dir)
    {
      const auto& handle = this->Handles[ax][dir];
      handle->GetMapper()->Update();
      count |= handle->HasTranslucentPolygonalGeometry();
    }
  }
  return count;
}

//-----------------------------------------------------------------------------
void vtkCameraOrientationRepresentation::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Container->ReleaseGraphicsResources(win);
  this->Shafts->ReleaseGraphicsResources(win);
  for (int ax = 0; ax < 3; ++ax)
  {
    for (int dir = 0; dir < 2; ++dir)
    {
      const auto& handle = this->Handles[ax][dir];
      handle->ReleaseGraphicsResources(win);
    }
  }
}

//-----------------------------------------------------------------------------
GETLABELPROPERTY(X, Plus)
//-----------------------------------------------------------------------------
GETLABELPROPERTY(Y, Plus)
//-----------------------------------------------------------------------------
GETLABELPROPERTY(Z, Plus)
//-----------------------------------------------------------------------------
GETLABELPROPERTY(X, Minus)
//-----------------------------------------------------------------------------
GETLABELPROPERTY(Y, Minus)
//-----------------------------------------------------------------------------
GETLABELPROPERTY(Z, Minus)

//-----------------------------------------------------------------------------
void vtkCameraOrientationRepresentation::SetContainerVisibility(bool state)
{
  this->Container->SetVisibility(state);
  this->Modified();
}

//-----------------------------------------------------------------------------
bool vtkCameraOrientationRepresentation::GetContainerVisibility()
{
  return this->Container->GetVisibility();
}

//-----------------------------------------------------------------------------
vtkProperty* vtkCameraOrientationRepresentation::GetContainerProperty()
{
  return this->Container->GetProperty();
}

//-----------------------------------------------------------------------------
void vtkCameraOrientationRepresentation::HighlightHandle()
{
  int dpi = 100;
  if (this->Renderer == nullptr)
  {
    return;
  }

  if (this->Renderer->GetRenderWindow() != nullptr)
  {
    dpi = this->Renderer->GetRenderWindow()->GetDPI();
  }

  vtkFreeTypeTools::GetInstance()->ScaleToPowerTwoOff();

  for (int ax = 0; ax < 3; ++ax)
  {
    for (int dir = 0; dir < 2; ++dir)
    {
      const auto& handle = this->Handles[ax][dir];
      const auto& handleSrc = this->HandleSources[ax][dir];
      const auto& labelTextProperty = this->AxisVectorTextProperties[ax][dir];
      const auto& labelTexture = this->LabelTextures[ax][dir];
      const auto& labelImage = this->LabelImages[ax][dir];

      const bool isPicked = this->PickedDir == dir && this->PickedAxis == ax;
      const bool renderText = (dir == to_underlying(HandleDirType::Plus)) || isPicked;

      if (isPicked)
      {
        labelTextProperty->SetColor(1., 1., 1.);
      }
      else
      {
        labelTextProperty->SetColor(0., 0., 0.);
      }

      if (renderText)
      {
        labelTextProperty->SetOpacity(1.0);
      }
      else
      {
        labelTextProperty->SetOpacity(0.0);
      }

      int textDims[2] = {};
      vtkFreeTypeTools::GetInstance()->RenderString(
        labelTextProperty, this->AxisLabelsText[ax][dir], dpi, labelImage, textDims);
      // Resize texture region in the button.
      handleSrc->SetTextureDimensions(textDims);
      // assign texture and render.
      labelTexture->SetInputData(labelImage);
      handle->SetTexture(labelTexture);
    }
  }
}

//-----------------------------------------------------------------------------
void vtkCameraOrientationRepresentation::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (pm != nullptr)
  {
    pm->AddPicker(this->HandlePicker, this);
  }
}

//-----------------------------------------------------------------------------
void vtkCameraOrientationRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "Positioning:" << endl;
  os << indent << "Size: " << this->Size[0] << " " << this->Size[1] << endl;
  os << indent << "Padding: " << this->Padding[0] << " " << this->Padding[1] << endl;
  switch (this->AnchorPosition)
  {
    case AnchorType::LowerLeft:
      os << indent << "LowerLeft" << endl;
      break;
    case AnchorType::LowerRight:
      os << indent << "LowerRight" << endl;
      break;
    case AnchorType::UpperLeft:
      os << indent << "UpperLeft" << endl;
      break;
    case AnchorType::UpperRight:
      os << indent << "UpperRight" << endl;
      break;
  }

  os << indent << "Geometry:" << endl;
  os << indent << "Bounds: " << this->Bounds[0] << " " << this->Bounds[1] << " " << this->Bounds[2]
     << " " << this->Bounds[3] << " " << this->Bounds[4] << " " << this->Bounds[5] << endl;
  os << indent << "Back: " << this->Back[0] << " " << this->Back[1] << " " << this->Back[2] << endl;
  os << indent << "Up: " << this->Up[0] << " " << this->Up[1] << " " << this->Up[2] << endl;
  os << indent << "Azimuth: " << this->Azimuth << endl;
  os << indent << "Elevation: " << this->Elevation << endl;
  os << indent << "MotionFactor: " << this->MotionFactor << endl;
  os << indent << "TotalLength: " << this->TotalLength << endl;
  os << indent << "NormalizedHandleDia: " << this->NormalizedHandleDia << endl;
  os << indent << "ShaftResolution: " << this->ShaftResolution << endl;
  os << indent << "HandleCircumferentialResolution: " << this->HandleCircumferentialResolution
     << endl;
  os << indent << "ContainerCircumferentialResolution: " << this->ContainerCircumferentialResolution
     << endl;
  os << indent << "ContainerRadialResolution: " << this->ContainerRadialResolution << endl;

  os << "TextLabels:" << endl;
  os << "PlusX: " << AxisLabelsText[0][0] << endl;
  os << "MinusX: " << AxisLabelsText[0][1] << endl;
  os << "PlusY: " << AxisLabelsText[1][0] << endl;
  os << "MinusY: " << AxisLabelsText[1][1] << endl;
  os << "PlusZ: " << AxisLabelsText[2][0] << endl;
  os << "MinusZ: " << AxisLabelsText[2][1] << endl;

  os << indent << "Picking:" << endl;
  os << indent << "PickedAxis: " << this->PickedAxis << endl;
  os << indent << "PickedDir: " << this->PickedDir << endl;
  os << indent << "LastPickedAxis: " << this->PickedAxis << endl;
  os << indent << "LastPickedDir: " << this->PickedDir << endl;

  os << indent << "Interaction:" << endl;
  os << indent << "LastEventPosition: " << this->LastEventPosition[0] << " "
     << this->LastEventPosition[1] << " " << this->LastEventPosition[2] << endl;
  switch (this->GetInteractionStateAsEnum())
  {
    case InteractionStateType::Outside:
      os << indent << "InteractionState: "
         << "Outside" << endl;
      break;
    case InteractionStateType::Hovering:
      os << indent << "InteractionState: "
         << "Hovering" << endl;
      break;
    case InteractionStateType::Rotating:
      os << indent << "InteractionState: "
         << "Rotating" << endl;
      break;
    default:
      break;
  }
  os << indent << "Transform:" << endl;
  this->Transform->PrintSelf(os, indent);
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
