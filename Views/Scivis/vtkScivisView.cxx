// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkScivisView.h"

#include "vtkAxesActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkInteractorObserver.h"
#include "vtkInteractorStyleRubberBand2D.h"
#include "vtkInteractorStyleRubberBand3D.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkLight.h"
#include "vtkLightKit.h"
#include "vtkObjectFactory.h"
#include "vtkOrientationMarkerWidget.h"
#include "vtkProp.h"
#include "vtkPropCollection.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkScivisRepresentation.h"
#include "vtkScivisSelector.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"

#include <algorithm>
#include <cstring>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkScivisView);

namespace
{
//------------------------------------------------------------------------------
// Read the rubber-band rectangle out of the interactor style that reported a
// selection.  Returns false for a style that has no such rectangle to offer.
bool GetStyleSelectionRegion(vtkObject* style, int region[4])
{
  const int* start = nullptr;
  const int* end = nullptr;
  if (auto* band3D = vtkInteractorStyleRubberBand3D::SafeDownCast(style))
  {
    start = band3D->GetStartPosition();
    end = band3D->GetEndPosition();
  }
  else if (auto* band2D = vtkInteractorStyleRubberBand2D::SafeDownCast(style))
  {
    start = band2D->GetStartPosition();
    end = band2D->GetEndPosition();
  }
  else
  {
    return false;
  }
  region[0] = start[0];
  region[1] = start[1];
  region[2] = end[0];
  region[3] = end[1];
  return true;
}
}

//------------------------------------------------------------------------------
class vtkScivisView::Command : public vtkCommand
{
public:
  static Command* New() { return new Command(); }
  void Execute(vtkObject* caller, unsigned long eventId, void* callData) override
  {
    if (this->Target)
    {
      this->Target->ProcessEvents(caller, eventId, callData);
    }
  }
  void SetTarget(vtkScivisView* t) { this->Target = t; }

private:
  Command() { this->Target = nullptr; }
  vtkScivisView* Target;
};
//------------------------------------------------------------------------------
class vtkScivisView::Internals
{
public:
  std::vector<vtkSmartPointer<vtkScivisRepresentation>> Representations;
};
//------------------------------------------------------------------------------
vtkScivisView::vtkScivisView()
{
  this->Implementation = new Internals();
  this->Observer = vtkScivisView::Command::New();
  this->Observer->SetTarget(this);

  this->Renderer = vtkSmartPointer<vtkRenderer>::New();
  this->RenderWindow = vtkSmartPointer<vtkRenderWindow>::New();
  this->RenderWindow->AddRenderer(this->Renderer);

  // An interactor is created with the window rather than on demand, so that a
  // view can be configured before anything is drawn.
  vtkSmartPointer<vtkRenderWindowInteractor> interactor =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  this->SetInteractor(interactor);

  this->FirstRender = true;
  this->InteractionMode = INTERACTION_MODE_3D;
  this->UseLightKitFlag = false;

  // Both built-in styles are created up front and kept for the life of the
  // view, so that switching interaction modes does not discard whatever the
  // application configured on them.  The selection observer is attached once
  // here rather than on every mode change.
  this->TrackballStyle = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
  this->RubberBandStyle = vtkSmartPointer<vtkInteractorStyleRubberBand3D>::New();
  this->RubberBandStyle->AddObserver(vtkCommand::SelectionChangedEvent, this->GetObserver());
  this->GetInteractor()->SetInteractorStyle(this->TrackballStyle);

  // Set sensible defaults
  this->GetRenderWindow()->SetSize(1024, 768);

  // Gradient background
  this->Renderer->SetBackground(0.32, 0.34, 0.43);
  this->Renderer->SetBackground2(0.0, 0.0, 0.17);
  this->Renderer->SetGradientBackground(true);

  // vtkRenderer makes a headlight of its own whenever it is asked to render
  // with no lights, and offers no way to ask which light that was.  Owning one
  // here means the renderer is never lightless, so it never makes one, and
  // enabling the light kit can put back exactly what it displaced instead of
  // clearing everything the application added.
  this->DefaultLight->SetLightTypeToHeadlight();
  this->Renderer->AddLight(this->DefaultLight);

  // Selection lives in its own object; it needs to know which view it selects in.
  this->Selector->SetView(this);

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
vtkScivisView::~vtkScivisView()
{
  this->RemoveAllRepresentations();
  this->Observer->SetTarget(nullptr);
  this->Observer->Delete();

  if (this->UseLightKitFlag)
  {
    this->LightKit->RemoveLightsFromRenderer(this->Renderer);
  }
  this->OrientationWidget->SetEnabled(0);
  delete this->Implementation;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkScivisView::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  // The background colors live on the renderer and the size and title on the
  // render window; neither is touched by rendering itself.
  mTime = std::max(mTime, this->Renderer->GetMTime());
  mTime = std::max(mTime, this->GetRenderWindow()->GetMTime());
  mTime = std::max(mTime, this->LightKit->GetMTime());
  mTime = std::max(mTime, this->OrientationWidget->GetMTime());
  mTime = std::max(mTime, this->Selector->GetMTime());
  return mTime;
}

//------------------------------------------------------------------------------
void vtkScivisView::SetBackground(double r, double g, double b)
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
double* vtkScivisView::GetBackground()
{
  return this->Renderer->GetBackground();
}

//------------------------------------------------------------------------------
void vtkScivisView::SetBackground2(double r, double g, double b)
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
double* vtkScivisView::GetBackground2()
{
  return this->Renderer->GetBackground2();
}

//------------------------------------------------------------------------------
void vtkScivisView::SetGradientBackground(bool val)
{
  if (this->GetGradientBackground() == val)
  {
    return;
  }
  this->Renderer->SetGradientBackground(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkScivisView::GetGradientBackground()
{
  return this->Renderer->GetGradientBackground();
}

//------------------------------------------------------------------------------
void vtkScivisView::SetWindowSize(int w, int h)
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
int* vtkScivisView::GetWindowSize()
{
  return this->GetRenderWindow()->GetSize();
}

//------------------------------------------------------------------------------
void vtkScivisView::SetWindowTitle(const char* title)
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
const char* vtkScivisView::GetWindowTitle()
{
  return this->GetRenderWindow()->GetWindowName();
}

//------------------------------------------------------------------------------
void vtkScivisView::SetOrientationAxesVisibility(bool val)
{
  if (this->GetOrientationAxesVisibility() == val)
  {
    return;
  }
  this->OrientationWidget->SetEnabled(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkScivisView::GetOrientationAxesVisibility()
{
  return this->OrientationWidget->GetEnabled() != 0;
}

//------------------------------------------------------------------------------
vtkOrientationMarkerWidget* vtkScivisView::GetOrientationMarkerWidget()
{
  return this->OrientationWidget;
}

//------------------------------------------------------------------------------
void vtkScivisView::SetUseLightKit(bool val)
{
  if (this->UseLightKitFlag == val)
  {
    return;
  }
  this->UseLightKitFlag = val;
  if (val)
  {
    // The kit replaces this view's headlight and nothing else; lights the
    // application added are its own business and stay where they are.
    this->Renderer->RemoveLight(this->DefaultLight);
    this->LightKit->AddLightsToRenderer(this->Renderer);
  }
  else
  {
    this->LightKit->RemoveLightsFromRenderer(this->Renderer);
    this->Renderer->AddLight(this->DefaultLight);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkScivisView::GetUseLightKit()
{
  return this->UseLightKitFlag;
}

//------------------------------------------------------------------------------
vtkLightKit* vtkScivisView::GetLightKit()
{
  return this->LightKit;
}

//------------------------------------------------------------------------------
void vtkScivisView::SetInteractionMode(int mode)
{
  vtkInteractorObserver* style = nullptr;
  switch (mode)
  {
    case INTERACTION_MODE_3D:
      style = this->TrackballStyle;
      break;
    case INTERACTION_MODE_SELECTION:
      style = this->RubberBandStyle;
      break;
    case INTERACTION_MODE_CUSTOM:
      if (!this->CustomStyle)
      {
        vtkWarningMacro("INTERACTION_MODE_CUSTOM requires a style set with SetInteractorStyle().");
        return;
      }
      style = this->CustomStyle;
      break;
    default:
      vtkWarningMacro("Unknown interaction mode: " << mode);
      return;
  }
  if (this->InteractionMode == mode)
  {
    return;
  }
  this->InteractionMode = mode;
  this->GetInteractor()->SetInteractorStyle(style);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkScivisView::SetInteractorStyle(vtkInteractorObserver* style)
{
  if (!style)
  {
    this->SetInteractionModeTo3D();
    return;
  }
  if (this->CustomStyle == style && this->InteractionMode == INTERACTION_MODE_CUSTOM)
  {
    return;
  }
  this->CustomStyle = style;
  this->InteractionMode = INTERACTION_MODE_CUSTOM;
  this->GetInteractor()->SetInteractorStyle(style);
  this->Modified();
}

//------------------------------------------------------------------------------
vtkInteractorObserver* vtkScivisView::GetInteractorStyle()
{
  return this->GetInteractor() ? this->GetInteractor()->GetInteractorStyle() : nullptr;
}

//------------------------------------------------------------------------------
int vtkScivisView::GetInteractionMode()
{
  return this->InteractionMode;
}

//------------------------------------------------------------------------------
void vtkScivisView::ProcessEvents(
  vtkObject* caller, unsigned long eventId, void* vtkNotUsed(callData))
{
  if (caller == this->GetInteractor()->GetInteractorStyle() &&
    eventId == vtkCommand::SelectionChangedEvent)
  {
    // The region is read from the style rather than from the event's call
    // data: the call data is an untyped blob whose layout only the rubber-band
    // styles define, while the style itself exposes the same numbers through
    // typed accessors.
    int region[4];
    if (GetStyleSelectionRegion(caller, region))
    {
      this->Selector->SelectRegion(
        region[0], region[1], region[2], region[3], this->GetInteractor()->GetShiftKey() != 0);
    }
    else
    {
      vtkWarningMacro("Ignoring SelectionChangedEvent from an interactor style that does not "
                      "expose a selection region.  Call SelectRegion() to drive selection from a "
                      "custom style.");
    }
  }
  else if (vtkScivisRepresentation::SafeDownCast(caller) &&
    eventId == vtkCommand::SelectionChangedEvent)
  {
    this->Render();
  }
  else if (auto* rep = vtkScivisRepresentation::SafeDownCast(caller))
  {
    // A push pipeline execution updated one of the representations.
    if (this->IsRepresentationPresent(rep) && eventId == vtkCommand::UpdateEvent)
    {
      this->Update();
    }
  }
}

//------------------------------------------------------------------------------
vtkScivisSelector* vtkScivisView::GetSelector()
{
  return this->Selector;
}

//------------------------------------------------------------------------------
void vtkScivisView::Start()
{
  this->Render();
  this->GetInteractor()->Start();
}

//------------------------------------------------------------------------------
void vtkScivisView::Render()
{
  if (this->FirstRender)
  {
    this->PrepareForRendering();
    this->Renderer->ResetCamera();
    this->FirstRender = false;
  }
  this->PrepareForRendering();
  this->RenderWindow->Render();
}

//------------------------------------------------------------------------------
void vtkScivisView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FirstRender: " << this->FirstRender << "\n";
  os << indent << "OrientationAxesVisibility: " << this->OrientationWidget->GetEnabled() << "\n";
  os << indent << "UseLightKit: " << this->UseLightKitFlag << "\n";
  os << indent << "InteractionMode: " << this->InteractionMode << "\n";
}

VTK_ABI_NAMESPACE_END

//------------------------------------------------------------------------------
vtkCommand* vtkScivisView::GetObserver()
{
  return this->Observer;
}

//------------------------------------------------------------------------------
vtkRenderer* vtkScivisView::GetRenderer()
{
  return this->Renderer;
}

//------------------------------------------------------------------------------
void vtkScivisView::SetRenderer(vtkRenderer* renderer)
{
  if (!renderer || renderer == this->Renderer)
  {
    return;
  }

  // Carry the props across, so that representations already in this view go on
  // being drawn by the renderer that replaces the old one.
  vtkPropCollection* props = this->Renderer->GetViewProps();
  props->InitTraversal();
  while (vtkProp* prop = props->GetNextProp())
  {
    renderer->AddViewProp(prop);
  }
  this->RenderWindow->RemoveRenderer(this->Renderer);
  this->RenderWindow->AddRenderer(renderer);
  this->Renderer = renderer;
  this->Modified();
}

//------------------------------------------------------------------------------
vtkRenderWindow* vtkScivisView::GetRenderWindow()
{
  return this->RenderWindow;
}

//------------------------------------------------------------------------------
void vtkScivisView::SetRenderWindow(vtkRenderWindow* window)
{
  if (!window)
  {
    vtkErrorMacro("SetRenderWindow() needs a window; ignoring a null one.");
    return;
  }
  if (window == this->RenderWindow)
  {
    return;
  }

  // The renderers belong to the view rather than to the window that happened to
  // be showing them, so they move across.
  vtkRendererCollection* renderers = this->RenderWindow->GetRenderers();
  while (renderers->GetNumberOfItems())
  {
    vtkRenderer* renderer = renderers->GetFirstRenderer();
    renderer->SetRenderWindow(nullptr);
    window->AddRenderer(renderer);
    this->RenderWindow->RemoveRenderer(renderer);
  }

  // So does the interactor style, whichever interactor is carrying it.
  vtkSmartPointer<vtkInteractorObserver> style =
    this->GetInteractor() ? this->GetInteractor()->GetInteractorStyle() : nullptr;
  this->RenderWindow = window;
  if (this->GetInteractor() && style)
  {
    this->GetInteractor()->SetInteractorStyle(style);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
vtkRenderWindowInteractor* vtkScivisView::GetInteractor()
{
  return this->RenderWindow->GetInteractor();
}

//------------------------------------------------------------------------------
void vtkScivisView::SetInteractor(vtkRenderWindowInteractor* interactor)
{
  if (interactor == this->GetInteractor())
  {
    return;
  }

  // Whatever style was installed belongs to the view rather than to the
  // interactor that happened to be carrying it.
  vtkSmartPointer<vtkInteractorObserver> style =
    this->GetInteractor() ? this->GetInteractor()->GetInteractorStyle() : nullptr;
  this->RenderWindow->SetInteractor(interactor);
  if (this->GetInteractor() && style)
  {
    this->GetInteractor()->SetInteractorStyle(style);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkScivisView::AddRepresentation(vtkScivisRepresentation* rep)
{
  if (!rep || this->IsRepresentationPresent(rep))
  {
    return;
  }

  // Added before AddToView() is called, so that a representation adding one of
  // its own from inside AddToView() lands after this one rather than before.
  auto& reps = this->Implementation->Representations;
  const size_t index = reps.size();
  reps.emplace_back(rep);

  if (rep->AddToView(this))
  {
    rep->AddObserver(vtkCommand::SelectionChangedEvent, this->GetObserver());
    rep->AddObserver(vtkCommand::UpdateEvent, this->GetObserver());
    this->AddRepresentationInternal(rep);
  }
  else
  {
    // The representation refused this view.
    reps.erase(reps.begin() + index);
  }
}

//------------------------------------------------------------------------------
void vtkScivisView::SetRepresentation(vtkScivisRepresentation* rep)
{
  this->RemoveAllRepresentations();
  this->AddRepresentation(rep);
}

//------------------------------------------------------------------------------
void vtkScivisView::RemoveRepresentation(vtkScivisRepresentation* rep)
{
  if (!this->IsRepresentationPresent(rep))
  {
    return;
  }
  this->RemoveRepresentationInternal(rep);
  rep->RemoveObserver(this->GetObserver());
  rep->RemoveFromView(this);
  auto& reps = this->Implementation->Representations;
  reps.erase(std::find(reps.begin(), reps.end(), rep));
}

//------------------------------------------------------------------------------
void vtkScivisView::RemoveAllRepresentations()
{
  while (!this->Implementation->Representations.empty())
  {
    this->RemoveRepresentation(this->Implementation->Representations.back());
  }
}

//------------------------------------------------------------------------------
int vtkScivisView::GetNumberOfRepresentations()
{
  return static_cast<int>(this->Implementation->Representations.size());
}

//------------------------------------------------------------------------------
vtkScivisRepresentation* vtkScivisView::GetRepresentation(int index)
{
  if (index < 0 || index >= this->GetNumberOfRepresentations())
  {
    return nullptr;
  }
  return this->Implementation->Representations[index];
}

//------------------------------------------------------------------------------
bool vtkScivisView::IsRepresentationPresent(vtkScivisRepresentation* rep)
{
  const auto& reps = this->Implementation->Representations;
  return rep && std::find(reps.begin(), reps.end(), rep) != reps.end();
}

//------------------------------------------------------------------------------
void vtkScivisView::Update()
{
  for (auto& rep : this->Implementation->Representations)
  {
    rep->Update();
  }
}

//------------------------------------------------------------------------------
void vtkScivisView::PrepareForRendering()
{
  this->Update();
}

//------------------------------------------------------------------------------
void vtkScivisView::ResetCamera()
{
  this->PrepareForRendering();
  this->Renderer->ResetCamera();
}
