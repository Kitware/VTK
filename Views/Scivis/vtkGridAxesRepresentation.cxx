// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkGridAxesRepresentation.h"

#include "vtkBoundingBox.h"
#include "vtkGridAxesActor3D.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkScivisView.h"
#include "vtkTextProperty.h"

#include <string>

#include <algorithm>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkGridAxesRepresentation);

//------------------------------------------------------------------------------
vtkGridAxesRepresentation::vtkGridAxesRepresentation()
{
  // There is no data behind the axes; they take their size from the view.
  this->SetNumberOfInputPorts(0);
  this->Padding = 0.0;
  this->ObserverTag = 0;

  this->HasSceneBounds = false;
  std::fill(this->SceneBounds, this->SceneBounds + 6, 0.0);
}

//------------------------------------------------------------------------------
vtkGridAxesRepresentation::~vtkGridAxesRepresentation() = default;

//------------------------------------------------------------------------------
bool vtkGridAxesRepresentation::AddToView(vtkScivisView* view)
{
  view->GetRenderer()->AddViewProp(this->Actor);
  this->ObserverTag = view->AddObserver(
    vtkScivisView::BoundsChangedEvent, this, &vtkGridAxesRepresentation::OnBoundsChanged);

  // The scene is already whatever size it is; the next event only says when
  // that changes, so fit the axes to it now.
  double bounds[6];
  if (view->GetSceneBounds(bounds))
  {
    this->OnBoundsChanged(view, vtkScivisView::BoundsChangedEvent, bounds);
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkGridAxesRepresentation::RemoveFromView(vtkScivisView* view)
{
  view->GetRenderer()->RemoveViewProp(this->Actor);
  if (this->ObserverTag)
  {
    view->RemoveObserver(this->ObserverTag);
    this->ObserverTag = 0;
  }
  return true;
}

//------------------------------------------------------------------------------
void vtkGridAxesRepresentation::OnBoundsChanged(
  vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(event), void* callData)
{
  if (!callData)
  {
    // Nothing with data is being drawn, so there is nothing to measure.  The
    // axes are left as they were rather than collapsed to a point.
    return;
  }
  std::copy(static_cast<double*>(callData), static_cast<double*>(callData) + 6, this->SceneBounds);
  this->HasSceneBounds = true;
  this->Fit();
}

//------------------------------------------------------------------------------
void vtkGridAxesRepresentation::Fit()
{
  if (!this->HasSceneBounds)
  {
    return;
  }
  vtkBoundingBox box(this->SceneBounds);
  if (this->Padding > 0.0)
  {
    box.ScaleAboutCenter(1.0 + this->Padding);
  }
  double bounds[6];
  box.GetBounds(bounds);
  this->Actor->SetGridBounds(bounds);
}

//------------------------------------------------------------------------------
void vtkGridAxesRepresentation::SetVisibility(bool val)
{
  if (this->GetVisibility() == val)
  {
    return;
  }
  this->Actor->SetVisibility(val);
  this->Modified();
}

//------------------------------------------------------------------------------
bool vtkGridAxesRepresentation::GetVisibility()
{
  return this->Actor->GetVisibility() != 0;
}

//------------------------------------------------------------------------------
void vtkGridAxesRepresentation::SetPadding(double padding)
{
  if (this->Padding == padding)
  {
    return;
  }
  this->Padding = padding;
  // Room around the data can change when the data has not.
  this->Fit();
  this->Modified();
}

//------------------------------------------------------------------------------
double vtkGridAxesRepresentation::GetPadding()
{
  return this->Padding;
}

//------------------------------------------------------------------------------
void vtkGridAxesRepresentation::SetXTitle(const char* title)
{
  const std::string current = this->Actor->GetTitle(0);
  if (current == (title ? title : ""))
  {
    return;
  }
  this->Actor->SetTitle(0, title ? title : "");
  this->Modified();
}

//------------------------------------------------------------------------------
const char* vtkGridAxesRepresentation::GetXTitle()
{
  return this->Actor->GetTitle(0).c_str();
}

//------------------------------------------------------------------------------
void vtkGridAxesRepresentation::SetYTitle(const char* title)
{
  const std::string current = this->Actor->GetTitle(1);
  if (current == (title ? title : ""))
  {
    return;
  }
  this->Actor->SetTitle(1, title ? title : "");
  this->Modified();
}

//------------------------------------------------------------------------------
const char* vtkGridAxesRepresentation::GetYTitle()
{
  return this->Actor->GetTitle(1).c_str();
}

//------------------------------------------------------------------------------
void vtkGridAxesRepresentation::SetZTitle(const char* title)
{
  const std::string current = this->Actor->GetTitle(2);
  if (current == (title ? title : ""))
  {
    return;
  }
  this->Actor->SetTitle(2, title ? title : "");
  this->Modified();
}

//------------------------------------------------------------------------------
const char* vtkGridAxesRepresentation::GetZTitle()
{
  return this->Actor->GetTitle(2).c_str();
}

//------------------------------------------------------------------------------
void vtkGridAxesRepresentation::SetLabelFontSize(int size)
{
  if (this->GetLabelFontSize() == size)
  {
    return;
  }
  for (int axis = 0; axis < 3; ++axis)
  {
    this->Actor->GetLabelTextProperty(axis)->SetFontSize(size);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkGridAxesRepresentation::GetLabelFontSize()
{
  return this->Actor->GetLabelTextProperty(0)->GetFontSize();
}

//------------------------------------------------------------------------------
void vtkGridAxesRepresentation::SetTitleFontSize(int size)
{
  if (this->GetTitleFontSize() == size)
  {
    return;
  }
  for (int axis = 0; axis < 3; ++axis)
  {
    this->Actor->GetTitleTextProperty(axis)->SetFontSize(size);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkGridAxesRepresentation::GetTitleFontSize()
{
  return this->Actor->GetTitleTextProperty(0)->GetFontSize();
}

//------------------------------------------------------------------------------
void vtkGridAxesRepresentation::SetLabelColor(double r, double g, double b)
{
  double* current = this->GetLabelColor();
  if (current[0] == r && current[1] == g && current[2] == b)
  {
    return;
  }
  for (int axis = 0; axis < 3; ++axis)
  {
    this->Actor->GetLabelTextProperty(axis)->SetColor(r, g, b);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkGridAxesRepresentation::GetLabelColor()
{
  return this->Actor->GetLabelTextProperty(0)->GetColor();
}

//------------------------------------------------------------------------------
void vtkGridAxesRepresentation::SetTitleColor(double r, double g, double b)
{
  double* current = this->GetTitleColor();
  if (current[0] == r && current[1] == g && current[2] == b)
  {
    return;
  }
  for (int axis = 0; axis < 3; ++axis)
  {
    this->Actor->GetTitleTextProperty(axis)->SetColor(r, g, b);
  }
  this->Modified();
}

//------------------------------------------------------------------------------
double* vtkGridAxesRepresentation::GetTitleColor()
{
  return this->Actor->GetTitleTextProperty(0)->GetColor();
}

//------------------------------------------------------------------------------
vtkGridAxesActor3D* vtkGridAxesRepresentation::GetGridAxesActor()
{
  return this->Actor;
}

//------------------------------------------------------------------------------
void vtkGridAxesRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Visibility: " << this->GetVisibility() << "\n";
  os << indent << "Padding: " << this->Padding << "\n";
  os << indent << "Actor:\n";
  this->Actor->PrintSelf(os, indent.GetNextIndent());
}
VTK_ABI_NAMESPACE_END
