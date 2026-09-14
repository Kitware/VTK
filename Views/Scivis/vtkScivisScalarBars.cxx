// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkScivisScalarBars.h"

#include "vtkCommand.h"
#include "vtkCoordinate.h"
#include "vtkDataObject.h"
#include "vtkLookupTableManager.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarBarRepresentation.h"
#include "vtkScalarBarWidget.h"
#include "vtkScalarsToColors.h"
#include "vtkScivisDataRepresentation.h"
#include "vtkScivisView.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <map>
#include <string>
#include <utility>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkScivisScalarBars);

//------------------------------------------------------------------------------
class vtkScivisScalarBars::Internals
{
public:
  // A scalar bar is identified by what it is labelled with, so that several
  // representations drawing one array share a bar rather than each having one.
  using Key = std::pair<std::string, int>;

  struct Entry
  {
    vtkSmartPointer<vtkScalarBarActor> Bar;
    // The range the bar spans, which only grows for as long as the bar is
    // alive, no matter what else writes to a table that may well be shared.
    double Range[2] = { 0.0, 0.0 };
    bool HasRange = false;
    // Whether the view built this array's table, and so gets to say what range
    // it spans.  A table the application registered with the manager keeps the
    // range the application gave it.
    bool OwnsRange = false;
    // Set while the bar is draggable; the widget is what the user picks up.
    vtkSmartPointer<vtkScalarBarWidget> Widget;
    // Whether the user has moved this bar, in which case stacking leaves it be.
    bool Moved = false;
    // Whoever fed the bar when its color map was last worked out, and whether
    // anything took the map the view offered.  Re-offering is cheap, but asking
    // the manager for a map creates one, so that is only done when the set of
    // contributors has actually changed.
    std::vector<vtkScivisDataRepresentation*> Contributors;
    bool SharedAccepted = false;
  };

  std::map<Key, Entry> Bars;

  // The entry at `index` in array-name order, or null outside the range.
  std::pair<const Key, Entry>* At(int index)
  {
    if (index < 0 || index >= static_cast<int>(this->Bars.size()))
    {
      return nullptr;
    }
    auto it = this->Bars.begin();
    std::advance(it, index);
    return &(*it);
  }
};

//------------------------------------------------------------------------------
vtkScivisScalarBars::vtkScivisScalarBars()
{
  this->Implementation = new Internals();
  this->AutoVisibility = true;
  this->Draggable = false;
}

//------------------------------------------------------------------------------
vtkScivisScalarBars::~vtkScivisScalarBars()
{
  delete this->Implementation;
}

//------------------------------------------------------------------------------
void vtkScivisScalarBars::SetView(vtkScivisView* view)
{
  this->View = view;
}

//------------------------------------------------------------------------------
vtkMTimeType vtkScivisScalarBars::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  // A bar's title, format and size are set on the actor rather than here.
  for (auto& pair : this->Implementation->Bars)
  {
    mTime = std::max(mTime, pair.second.Bar->GetMTime());
  }
  return mTime;
}

//------------------------------------------------------------------------------
void vtkScivisScalarBars::SetDraggable(bool val)
{
  if (this->Draggable == val)
  {
    return;
  }
  this->Draggable = val;
  // The widgets are put up and taken down in UpdateWidgets(), which runs before
  // the next render and has the renderer they have to be added to and removed
  // from.
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkScivisScalarBars::GetDraggable()
{
  return this->Draggable;
}

//------------------------------------------------------------------------------
vtkScalarBarWidget* vtkScivisScalarBars::GetWidget(int index)
{
  auto* entry = this->Implementation->At(index);
  return entry ? entry->second.Widget : nullptr;
}

void vtkScivisScalarBars::SetAutoVisibility(bool val)
{
  if (this->AutoVisibility == val)
  {
    return;
  }
  this->AutoVisibility = val;
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkScivisScalarBars::GetAutoVisibility()
{
  return this->AutoVisibility;
}

//------------------------------------------------------------------------------
int vtkScivisScalarBars::GetNumberOfBars()
{
  return static_cast<int>(this->Implementation->Bars.size());
}

//------------------------------------------------------------------------------
const char* vtkScivisScalarBars::GetArrayName(int index)
{
  auto* entry = this->Implementation->At(index);
  return entry ? entry->first.first.c_str() : nullptr;
}

//------------------------------------------------------------------------------
int vtkScivisScalarBars::GetFieldAssociation(int index)
{
  auto* entry = this->Implementation->At(index);
  return entry ? entry->first.second : vtkDataObject::FIELD_ASSOCIATION_POINTS;
}

//------------------------------------------------------------------------------
vtkScalarBarActor* vtkScivisScalarBars::GetActor(int index)
{
  auto* entry = this->Implementation->At(index);
  return entry ? entry->second.Bar : nullptr;
}

//------------------------------------------------------------------------------
vtkScalarBarActor* vtkScivisScalarBars::GetActor(const char* arrayName, int fieldAssoc)
{
  if (!arrayName)
  {
    return nullptr;
  }
  auto& bars = this->Implementation->Bars;
  auto it = bars.find(Internals::Key(arrayName, fieldAssoc));
  return it == bars.end() ? nullptr : it->second.Bar;
}

//------------------------------------------------------------------------------
void vtkScivisScalarBars::Update()
{
  if (!this->View)
  {
    return;
  }
  auto& bars = this->Implementation->Bars;
  vtkRenderer* renderer = this->View->GetRenderer();
  vtkLookupTableManager* manager = this->View->GetLookupTableManager();

  if (!this->AutoVisibility)
  {
    // Stop maintaining bars, leaving the representations themselves alone.  The
    // color maps they draw through are the manager's and outlive the bars.
    for (auto& pair : bars)
    {
      renderer->RemoveViewProp(pair.second.Bar);
    }
    bars.clear();
    return;
  }

  // What is being drawn, and by whom.  A representation with no data behind it
  // colors nothing and is passed over, and so is a hidden one -- hiding a
  // representation takes it out of the range its array's bar spans, and retires
  // the bar entirely when it was the last one drawing that array.
  std::map<Internals::Key, std::vector<vtkScivisDataRepresentation*>> rendered;
  for (int i = 0; i < this->View->GetNumberOfRepresentations(); ++i)
  {
    auto* rep = vtkScivisDataRepresentation::SafeDownCast(this->View->GetRepresentation(i));
    if (!rep || !rep->GetVisibility())
    {
      continue;
    }
    const char* name = rep->GetRenderedArrayName();
    if (!name)
    {
      continue;
    }
    const int assoc = rep->GetRenderedFieldAssociation();
    if (assoc == vtkDataObject::FIELD_ASSOCIATION_NONE)
    {
      // Field data carries one tuple per block rather than one per element, so
      // a bar over it would be labelling a range with no extent in the scene.
      continue;
    }
    rendered[Internals::Key(name, assoc)].push_back(rep);
  }

  // Retire the bars nothing feeds any more.
  for (auto it = bars.begin(); it != bars.end();)
  {
    if (rendered.find(it->first) == rendered.end())
    {
      renderer->RemoveViewProp(it->second.Bar);
      it = bars.erase(it);
    }
    else
    {
      ++it;
    }
  }

  for (const auto& active : rendered)
  {
    const std::string& arrayName = active.first.first;
    const int fieldAssoc = active.first.second;
    auto& entry = bars[active.first];

    if (!entry.Bar)
    {
      entry.Bar = vtkSmartPointer<vtkScalarBarActor>::New();
      entry.Bar->SetNumberOfLabels(5);
      entry.Bar->SetTitle(arrayName.c_str());
      renderer->AddViewProp(entry.Bar);
      // A table the application registered with the manager before this array
      // was first drawn brings its own range; one the view is about to build
      // does not, so the view spans it over the data.
      entry.OwnsRange = !manager->HasLookupTable(arrayName.c_str());
    }

    // The union of what the contributors draw, grown into whatever the bar was
    // already showing.
    double merged[2] = { VTK_DOUBLE_MAX, VTK_DOUBLE_MIN };
    bool haveRange = false;
    for (vtkScivisDataRepresentation* rep : active.second)
    {
      double range[2];
      if (rep->GetDataRange(arrayName.c_str(), fieldAssoc, range))
      {
        merged[0] = std::min(merged[0], range[0]);
        merged[1] = std::max(merged[1], range[1]);
        haveRange = true;
      }
    }
    if (haveRange)
    {
      if (entry.HasRange)
      {
        merged[0] = std::min(merged[0], entry.Range[0]);
        merged[1] = std::max(merged[1], entry.Range[1]);
      }
      entry.Range[0] = merged[0];
      entry.Range[1] = merged[1];
      entry.HasRange = true;
    }

    // Offer every contributor the one map for this array, which is what makes
    // them agree on colors and, since the mappers take their range from it, on
    // range as well.  A representation that owns transfer functions of its own
    // declines and goes on reporting its own.
    // Offer every contributor the one map for this array, which is what makes
    // them agree on colors and, since the mappers take their range from it, on
    // range as well.  A representation that owns transfer functions of its own
    // declines and goes on reporting its own.
    vtkScalarsToColors* shared = nullptr;
    const bool contributorsChanged = entry.Contributors != active.second;
    entry.Contributors = active.second;

    if (contributorsChanged || entry.SharedAccepted)
    {
      const bool managerHadMap = manager->HasLookupTable(arrayName.c_str());
      shared = manager->GetLookupTable(arrayName.c_str());
      bool accepted = false;
      for (vtkScivisDataRepresentation* rep : active.second)
      {
        rep->SetColorMap(shared);
        accepted = accepted || rep->GetColorMap() == shared;
      }
      // Asking the manager creates a map, so an array drawn only by
      // representations that decline it -- a volume, which colors through
      // transfer functions of its own -- would leave one behind that nothing
      // uses.  Keep the manager to the arrays actually colored through it.
      if (!accepted && !managerHadMap)
      {
        manager->RemoveLookupTable(arrayName.c_str());
        shared = nullptr;
      }
      entry.SharedAccepted = accepted;
    }

    if (entry.SharedAccepted && entry.HasRange && entry.OwnsRange)
    {
      shared->SetRange(entry.Range);
    }

    // One bar cannot show two maps.  It shows the shared one when anything took
    // it, and otherwise the map the first contributor reports, so that an array
    // drawn only as a volume is still labelled with the functions in use.
    entry.Bar->SetLookupTable(entry.SharedAccepted ? shared : active.second.front()->GetColorMap());
  }

  this->UpdateWidgets();
  this->Position();
}

//------------------------------------------------------------------------------
void vtkScivisScalarBars::UpdateWidgets()
{
  vtkRenderer* renderer = this->View->GetRenderer();
  vtkRenderWindowInteractor* interactor = this->View->GetInteractor();

  for (auto& pair : this->Implementation->Bars)
  {
    Internals::Entry& entry = pair.second;

    if (this->Draggable && !entry.Widget)
    {
      entry.Widget = vtkSmartPointer<vtkScalarBarWidget>::New();
      entry.Widget->SetScalarBarActor(entry.Bar);

      // The representation drives the bar once the widget is enabled, and has
      // geometry of its own.  Start it where the bar is.  For a bar in the
      // stack this only agrees with what Position() is about to say; for one
      // the user has moved, which Position() leaves alone, it is what keeps it
      // where it was put.
      if (vtkScalarBarRepresentation* representation = entry.Widget->GetScalarBarRepresentation())
      {
        double* position = entry.Bar->GetPositionCoordinate()->GetValue();
        representation->SetPosition(position[0], position[1]);
        representation->SetPosition2(entry.Bar->GetWidth(), entry.Bar->GetHeight());
      }

      // The widget draws the bar from here on, so the renderer must not draw it
      // a second time.
      renderer->RemoveViewProp(entry.Bar);
      entry.Widget->SetInteractor(interactor);
      entry.Widget->SetCurrentRenderer(renderer);
      entry.Widget->SetEnabled(1);
      entry.Widget->AddObserver(
        vtkCommand::InteractionEvent, this, &vtkScivisScalarBars::OnDragged);
    }
    else if (!this->Draggable && entry.Widget)
    {
      // Handing the bar back: the widget stops drawing it, so the renderer has
      // to again.  Whatever the user did to it stays done -- the geometry comes
      // off the representation before it goes away, and a bar that was moved is
      // still a moved bar, so stacking goes on leaving it alone.
      if (vtkScalarBarRepresentation* representation = entry.Widget->GetScalarBarRepresentation())
      {
        double* position = representation->GetPosition();
        double* size = representation->GetPosition2();
        entry.Bar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
        entry.Bar->GetPositionCoordinate()->SetValue(position[0], position[1]);
        entry.Bar->SetWidth(size[0]);
        entry.Bar->SetHeight(size[1]);
      }
      entry.Widget->SetEnabled(0);
      entry.Widget = nullptr;
      renderer->AddViewProp(entry.Bar);
    }
    else if (entry.Widget)
    {
      entry.Widget->SetInteractor(interactor);
    }
  }
}

//------------------------------------------------------------------------------
void vtkScivisScalarBars::OnDragged(vtkObject* caller, unsigned long, void*)
{
  // Whichever bar was dragged keeps where it was put from now on.
  for (auto& pair : this->Implementation->Bars)
  {
    if (pair.second.Widget == caller)
    {
      pair.second.Moved = true;
    }
  }
}

//------------------------------------------------------------------------------
void vtkScivisScalarBars::Position()
{
  const int count = this->GetNumberOfBars();
  if (count == 0)
  {
    return;
  }

  // Bars run down the right edge, sharing the vertical space between them and
  // shrinking as more of them appear.
  constexpr double top = 0.9;
  constexpr double bottom = 0.1;
  constexpr double gap = 0.02;
  constexpr double width = 0.08;
  constexpr double left = 0.88;

  const double height = std::min(0.5, ((top - bottom) - gap * (count - 1)) / count);
  double y = top - height;
  for (auto& pair : this->Implementation->Bars)
  {
    if (pair.second.Moved)
    {
      // The user put this one somewhere; stacking would take it back.
      continue;
    }
    vtkScalarBarActor* bar = pair.second.Bar;
    bar->SetOrientationToVertical();
    bar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
    bar->GetPositionCoordinate()->SetValue(left, y);
    bar->SetWidth(width);
    bar->SetHeight(height);
    // A widget's representation overwrites the bar's own geometry when it draws,
    // so the stack has to be spelled there as well while one is in charge.
    if (pair.second.Widget)
    {
      if (vtkScalarBarRepresentation* representation =
            pair.second.Widget->GetScalarBarRepresentation())
      {
        representation->SetPosition(left, y);
        representation->SetPosition2(width, height);
      }
    }
    y -= height + gap;
  }
}

//------------------------------------------------------------------------------
void vtkScivisScalarBars::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "AutoVisibility: " << this->AutoVisibility << "\n";
  os << indent << "Draggable: " << this->Draggable << "\n";
  os << indent << "NumberOfBars: " << this->GetNumberOfBars() << "\n";
  for (int i = 0; i < this->GetNumberOfBars(); ++i)
  {
    os << indent << "  " << this->GetArrayName(i) << " (association "
       << this->GetFieldAssociation(i) << ")\n";
  }
  os << indent << "View: " << (this->View ? "set" : "(none)") << "\n";
}

VTK_ABI_NAMESPACE_END
