// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkStandardRenderView.h"

#include "vtkAxesActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkDataRepresentation.h"
#include "vtkDoubleArray.h"
#include "vtkHardwareSelector.h"
#include "vtkInteractorObserver.h"
#include "vtkInteractorStyleRubberBand3D.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLight.h"
#include "vtkLightKit.h"
#include "vtkObjectFactory.h"
#include "vtkOrientationMarkerWidget.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"

#include <algorithm>
#include <cstring>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkStandardRenderView);

//------------------------------------------------------------------------------
vtkStandardRenderView::vtkStandardRenderView()
{
  this->FirstRender = true;
  this->InteractionMode = INTERACTION_MODE_3D;
  this->SelectionMode = SELECTION_MODE_SURFACE;
  this->FieldAssociation = vtkDataObject::FIELD_ASSOCIATION_CELLS;
  this->UseLightKitFlag = false;

  // Set up trackball camera interaction style
  vtkInteractorStyleTrackballCamera* style = vtkInteractorStyleTrackballCamera::New();
  this->GetInteractor()->SetInteractorStyle(style);
  style->Delete();

  // Set sensible defaults
  this->GetRenderWindow()->SetSize(1024, 768);

  // Gradient background
  this->Renderer->SetBackground(0.32, 0.34, 0.43);
  this->Renderer->SetBackground2(0.0, 0.0, 0.17);
  this->Renderer->SetGradientBackground(true);

  // Hardware selector for surface selection
  this->HardwareSelector->SetRenderer(this->Renderer);
  this->HardwareSelector->SetFieldAssociation(this->FieldAssociation);

  // Orientation axes marker
  vtkNew<vtkAxesActor> axes;
  this->OrientationWidget->SetOrientationMarker(axes);
  this->OrientationWidget->SetInteractor(this->GetInteractor());
  this->OrientationWidget->SetDefaultRenderer(this->Renderer);
  this->OrientationWidget->SetViewport(0.0, 0.0, 0.2, 0.2);
  this->OrientationWidget->SetEnabled(1);
  this->OrientationWidget->SetInteractive(false);
}

//------------------------------------------------------------------------------
vtkStandardRenderView::~vtkStandardRenderView()
{
  if (this->UseLightKitFlag)
  {
    this->LightKit->RemoveLightsFromRenderer(this->Renderer);
  }
  this->OrientationWidget->SetEnabled(0);
}

//------------------------------------------------------------------------------
vtkMTimeType vtkStandardRenderView::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  // The background colors live on the renderer and the size and title on the
  // render window; neither is touched by rendering itself.
  mTime = std::max(mTime, this->Renderer->GetMTime());
  mTime = std::max(mTime, this->GetRenderWindow()->GetMTime());
  mTime = std::max(mTime, this->LightKit->GetMTime());
  mTime = std::max(mTime, this->OrientationWidget->GetMTime());
  mTime = std::max(mTime, this->HardwareSelector->GetMTime());
  return mTime;
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetBackground(double r, double g, double b)
{
  double* current = this->GetBackground();
  if (current[0] == r && current[1] == g && current[2] == b)
  {
    return;
  }
  this->Renderer->SetBackground(r, g, b);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkStandardRenderView::GetBackground()
{
  return this->Renderer->GetBackground();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetBackground2(double r, double g, double b)
{
  double* current = this->GetBackground2();
  if (current[0] == r && current[1] == g && current[2] == b)
  {
    return;
  }
  this->Renderer->SetBackground2(r, g, b);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkStandardRenderView::GetBackground2()
{
  return this->Renderer->GetBackground2();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetGradientBackground(bool val)
{
  if (this->GetGradientBackground() == val)
  {
    return;
  }
  this->Renderer->SetGradientBackground(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkStandardRenderView::GetGradientBackground()
{
  return this->Renderer->GetGradientBackground();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetWindowSize(int w, int h)
{
  int* current = this->GetWindowSize();
  if (current[0] == w && current[1] == h)
  {
    return;
  }
  this->GetRenderWindow()->SetSize(w, h);
  this->Modified();
}

//------------------------------------------------------------------------------
int* vtkStandardRenderView::GetWindowSize()
{
  return this->GetRenderWindow()->GetSize();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetWindowTitle(const char* title)
{
  const char* current = this->GetWindowTitle();
  if (current == title || (current && title && strcmp(current, title) == 0))
  {
    return;
  }
  this->GetRenderWindow()->SetWindowName(title);
  this->Modified();
}

//------------------------------------------------------------------------------
const char* vtkStandardRenderView::GetWindowTitle()
{
  return this->GetRenderWindow()->GetWindowName();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetOrientationAxesVisibility(bool val)
{
  if (this->GetOrientationAxesVisibility() == val)
  {
    return;
  }
  this->OrientationWidget->SetEnabled(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkStandardRenderView::GetOrientationAxesVisibility()
{
  return this->OrientationWidget->GetEnabled() != 0;
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetOrientationAxesInteractive(bool val)
{
  if (this->GetOrientationAxesInteractive() == val)
  {
    return;
  }
  this->OrientationWidget->SetInteractive(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkStandardRenderView::GetOrientationAxesInteractive()
{
  return this->OrientationWidget->GetInteractive() != 0;
}

//------------------------------------------------------------------------------
vtkOrientationMarkerWidget* vtkStandardRenderView::GetOrientationMarkerWidget()
{
  return this->OrientationWidget;
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetUseLightKit(bool val)
{
  if (this->UseLightKitFlag == val)
  {
    return;
  }
  this->UseLightKitFlag = val;
  if (val)
  {
    // Remove any existing lights (e.g. the default headlight that VTK
    // auto-creates when the renderer has no lights) before adding the
    // light kit, so we don't accumulate extra lights on toggle.
    this->Renderer->RemoveAllLights();
    this->LightKit->AddLightsToRenderer(this->Renderer);
  }
  else
  {
    this->LightKit->RemoveLightsFromRenderer(this->Renderer);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkStandardRenderView::GetUseLightKit()
{
  return this->UseLightKitFlag;
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetKeyLightIntensity(double val)
{
  if (this->GetKeyLightIntensity() == val)
  {
    return;
  }
  this->LightKit->SetKeyLightIntensity(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkStandardRenderView::GetKeyLightIntensity()
{
  return this->LightKit->GetKeyLightIntensity();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetKeyToFillRatio(double val)
{
  if (this->GetKeyToFillRatio() == val)
  {
    return;
  }
  this->LightKit->SetKeyToFillRatio(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkStandardRenderView::GetKeyToFillRatio()
{
  return this->LightKit->GetKeyToFillRatio();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetKeyToHeadRatio(double val)
{
  if (this->GetKeyToHeadRatio() == val)
  {
    return;
  }
  this->LightKit->SetKeyToHeadRatio(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkStandardRenderView::GetKeyToHeadRatio()
{
  return this->LightKit->GetKeyToHeadRatio();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetKeyToBackRatio(double val)
{
  if (this->GetKeyToBackRatio() == val)
  {
    return;
  }
  this->LightKit->SetKeyToBackRatio(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkStandardRenderView::GetKeyToBackRatio()
{
  return this->LightKit->GetKeyToBackRatio();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetKeyLightWarmth(double val)
{
  if (this->GetKeyLightWarmth() == val)
  {
    return;
  }
  this->LightKit->SetKeyLightWarmth(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkStandardRenderView::GetKeyLightWarmth()
{
  return this->LightKit->GetKeyLightWarmth();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetFillLightWarmth(double val)
{
  if (this->GetFillLightWarmth() == val)
  {
    return;
  }
  this->LightKit->SetFillLightWarmth(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkStandardRenderView::GetFillLightWarmth()
{
  return this->LightKit->GetFillLightWarmth();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetHeadLightWarmth(double val)
{
  if (this->GetHeadLightWarmth() == val)
  {
    return;
  }
  this->LightKit->SetHeadLightWarmth(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkStandardRenderView::GetHeadLightWarmth()
{
  return this->LightKit->GetHeadLightWarmth();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetBackLightWarmth(double val)
{
  if (this->GetBackLightWarmth() == val)
  {
    return;
  }
  this->LightKit->SetBackLightWarmth(val);
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkStandardRenderView::GetBackLightWarmth()
{
  return this->LightKit->GetBackLightWarmth();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetKeyLightAngle(double elevation, double azimuth)
{
  double* current = this->GetKeyLightAngle();
  if (current[0] == elevation && current[1] == azimuth)
  {
    return;
  }
  this->LightKit->SetKeyLightAngle(elevation, azimuth);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkStandardRenderView::GetKeyLightAngle()
{
  return this->LightKit->GetKeyLightAngle();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetFillLightAngle(double elevation, double azimuth)
{
  double* current = this->GetFillLightAngle();
  if (current[0] == elevation && current[1] == azimuth)
  {
    return;
  }
  this->LightKit->SetFillLightAngle(elevation, azimuth);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkStandardRenderView::GetFillLightAngle()
{
  return this->LightKit->GetFillLightAngle();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetBackLightAngle(double elevation, double azimuth)
{
  double* current = this->GetBackLightAngle();
  if (current[0] == elevation && current[1] == azimuth)
  {
    return;
  }
  this->LightKit->SetBackLightAngle(elevation, azimuth);
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkStandardRenderView::GetBackLightAngle()
{
  return this->LightKit->GetBackLightAngle();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetMaintainLuminance(bool val)
{
  if (this->GetMaintainLuminance() == val)
  {
    return;
  }
  this->LightKit->SetMaintainLuminance(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkStandardRenderView::GetMaintainLuminance()
{
  return this->LightKit->GetMaintainLuminance() != 0;
}

//------------------------------------------------------------------------------
vtkLightKit* vtkStandardRenderView::GetLightKit()
{
  return this->LightKit;
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::AddLight(vtkLight* light)
{
  this->Renderer->AddLight(light);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::RemoveLight(vtkLight* light)
{
  this->Renderer->RemoveLight(light);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::RemoveAllLights()
{
  this->Renderer->RemoveAllLights();
  if (this->UseLightKitFlag)
  {
    this->UseLightKitFlag = false;
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetInteractionMode(int mode)
{
  if (this->InteractionMode == mode)
  {
    return;
  }
  this->InteractionMode = mode;
  vtkInteractorObserver* oldStyle = this->GetInteractor()->GetInteractorStyle();
  if (mode == INTERACTION_MODE_3D)
  {
    if (oldStyle)
    {
      oldStyle->RemoveObserver(this->GetObserver());
    }
    vtkInteractorStyleTrackballCamera* style = vtkInteractorStyleTrackballCamera::New();
    this->GetInteractor()->SetInteractorStyle(style);
    style->Delete();
  }
  else if (mode == INTERACTION_MODE_SELECTION)
  {
    if (oldStyle)
    {
      oldStyle->RemoveObserver(this->GetObserver());
    }
    vtkInteractorStyleRubberBand3D* style = vtkInteractorStyleRubberBand3D::New();
    this->GetInteractor()->SetInteractorStyle(style);
    style->AddObserver(vtkCommand::SelectionChangedEvent, this->GetObserver());
    style->Delete();
  }
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkStandardRenderView::GetInteractionMode()
{
  return this->InteractionMode;
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetSelectionMode(int mode)
{
  if (this->SelectionMode != mode)
  {
    this->SelectionMode = mode;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkStandardRenderView::GetSelectionMode()
{
  return this->SelectionMode;
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SetSelectionFieldAssociation(int assoc)
{
  if (this->FieldAssociation != assoc)
  {
    this->FieldAssociation = assoc;
    this->HardwareSelector->SetFieldAssociation(assoc);
    this->Modified();
  }
}

//------------------------------------------------------------------------------
int vtkStandardRenderView::GetSelectionFieldAssociation()
{
  return this->FieldAssociation;
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SelectCells()
{
  this->SetSelectionFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_CELLS);
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::SelectPoints()
{
  this->SetSelectionFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_POINTS);
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::ClearSelection()
{
  vtkNew<vtkSelection> empty;
  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
  {
    this->GetRepresentation(i)->Select(this, empty, false);
  }
  this->CurrentSelection = empty;
  this->InvokeEvent(vtkCommand::SelectionChangedEvent, empty.GetPointer());
  this->Render();
}

//------------------------------------------------------------------------------
vtkSelection* vtkStandardRenderView::GetCurrentSelection()
{
  return this->CurrentSelection;
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::ProcessEvents(vtkObject* caller, unsigned long eventId, void* callData)
{
  if (caller == this->GetInteractor()->GetInteractorStyle() &&
    eventId == vtkCommand::SelectionChangedEvent)
  {
    vtkNew<vtkSelection> selection;
    this->GenerateSelection(callData, selection);

    unsigned int* data = reinterpret_cast<unsigned int*>(callData);
    bool extend = (data[4] == vtkInteractorStyleRubberBand3D::SELECT_UNION);

    for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
    {
      this->GetRepresentation(i)->Select(this, selection, extend);
    }

    // Store the selection and notify observers on the view.
    this->CurrentSelection = selection;
    this->InvokeEvent(vtkCommand::SelectionChangedEvent, selection.GetPointer());
    this->Render();
  }
  else if (vtkDataRepresentation::SafeDownCast(caller) &&
    eventId == vtkCommand::SelectionChangedEvent)
  {
    this->Render();
  }
  this->Superclass::ProcessEvents(caller, eventId, callData);
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::GenerateSelection(void* callData, vtkSelection* sel)
{
  unsigned int* rect = reinterpret_cast<unsigned int*>(callData);
  unsigned int pos1X = rect[0];
  unsigned int pos1Y = rect[1];
  unsigned int pos2X = rect[2];
  unsigned int pos2Y = rect[3];

  // For single-click (start == end), expand the area a bit.  Clamp against
  // the stretch amount using an unsigned comparison so a click near the
  // lower-left corner does not underflow to a huge value.
  unsigned int stretch = 2;
  if (pos1X == pos2X && pos1Y == pos2Y)
  {
    pos1X = pos1X > stretch ? pos1X - stretch : 0;
    pos1Y = pos1Y > stretch ? pos1Y - stretch : 0;
    pos2X = pos2X + stretch;
    pos2Y = pos2Y + stretch;
  }
  unsigned int screenMinX = pos1X < pos2X ? pos1X : pos2X;
  unsigned int screenMaxX = pos1X < pos2X ? pos2X : pos1X;
  unsigned int screenMinY = pos1Y < pos2Y ? pos1Y : pos2Y;
  unsigned int screenMaxY = pos1Y < pos2Y ? pos2Y : pos1Y;

  if (this->SelectionMode == SELECTION_MODE_FRUSTUM)
  {
    double displayRect[4] = { static_cast<double>(screenMinX), static_cast<double>(screenMinY),
      static_cast<double>(screenMaxX), static_cast<double>(screenMaxY) };
    vtkDoubleArray* frustcorners = vtkDoubleArray::New();
    frustcorners->SetNumberOfComponents(4);
    frustcorners->SetNumberOfTuples(8);

    vtkRenderer* renderer = this->GetRenderer();
    double worldP[4];
    int index = 0;

    // 4 screen corners x 2 depth values (near=0, far=1) = 8 frustum corners
    double corners[4][2] = { { displayRect[0], displayRect[1] }, { displayRect[0], displayRect[3] },
      { displayRect[2], displayRect[1] }, { displayRect[2], displayRect[3] } };
    for (int c = 0; c < 4; ++c)
    {
      for (double z = 0.0; z <= 1.0; z += 1.0)
      {
        renderer->SetDisplayPoint(corners[c][0], corners[c][1], z);
        renderer->DisplayToWorld();
        renderer->GetWorldPoint(worldP);
        frustcorners->SetTuple4(index, worldP[0], worldP[1], worldP[2], worldP[3]);
        index++;
      }
    }

    vtkSelectionNode* node = vtkSelectionNode::New();
    node->SetContentType(vtkSelectionNode::FRUSTUM);
    node->SetFieldType(this->FieldAssociation == vtkDataObject::FIELD_ASSOCIATION_POINTS
        ? vtkSelectionNode::POINT
        : vtkSelectionNode::CELL);
    node->SetSelectionList(frustcorners);
    sel->AddNode(node);
    node->Delete();
    frustcorners->Delete();
  }
  else
  {
    // Surface selection via hardware picking
    unsigned int area[4] = { 0, 0, 0, 0 };
    area[2] = static_cast<unsigned int>(this->Renderer->GetSize()[0] - 1);
    area[3] = static_cast<unsigned int>(this->Renderer->GetSize()[1] - 1);
    this->HardwareSelector->SetArea(area);
    this->HardwareSelector->CaptureBuffers();

    vtkSelection* vsel =
      this->HardwareSelector->GenerateSelection(screenMinX, screenMinY, screenMaxX, screenMaxY);
    sel->ShallowCopy(vsel);
    vsel->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::Start()
{
  this->Render();
  this->GetInteractor()->Start();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::Render()
{
  if (this->FirstRender)
  {
    this->PrepareForRendering();
    this->Renderer->ResetCamera();
    this->FirstRender = false;
  }
  this->Superclass::Render();
}

//------------------------------------------------------------------------------
void vtkStandardRenderView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FirstRender: " << this->FirstRender << "\n";
  os << indent << "OrientationAxesVisibility: " << this->OrientationWidget->GetEnabled() << "\n";
  os << indent << "OrientationAxesInteractive: " << this->OrientationWidget->GetInteractive()
     << "\n";
  os << indent << "UseLightKit: " << this->UseLightKitFlag << "\n";
  os << indent << "InteractionMode: " << this->InteractionMode << "\n";
  os << indent << "SelectionMode: " << this->SelectionMode << "\n";
  os << indent << "FieldAssociation: " << this->FieldAssociation << "\n";
  os << indent << "KeyLightIntensity: " << this->LightKit->GetKeyLightIntensity() << "\n";
  os << indent << "KeyToFillRatio: " << this->LightKit->GetKeyToFillRatio() << "\n";
  os << indent << "KeyToHeadRatio: " << this->LightKit->GetKeyToHeadRatio() << "\n";
  os << indent << "KeyToBackRatio: " << this->LightKit->GetKeyToBackRatio() << "\n";
}

VTK_ABI_NAMESPACE_END
