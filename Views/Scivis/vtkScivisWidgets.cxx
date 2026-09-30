// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkScivisWidgets.h"

#include "vtkAbstractWidget.h"
#include "vtkAngleRepresentation2D.h"
#include "vtkAngleWidget.h"
#include "vtkBoxRepresentation.h"
#include "vtkBoxWidget2.h"
#include "vtkCaptionActor2D.h"
#include "vtkCaptionRepresentation.h"
#include "vtkCaptionWidget.h"
#include "vtkDistanceRepresentation2D.h"
#include "vtkDistanceWidget.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkLineRepresentation.h"
#include "vtkLineWidget2.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointHandleRepresentation3D.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScivisView.h"
#include "vtkSmartPointer.h"
#include "vtkSplineRepresentation.h"
#include "vtkSplineWidget2.h"
#include "vtkTextProperty.h"
#include "vtkWidgetRepresentation.h"

#include <algorithm>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkScivisWidgets);

//------------------------------------------------------------------------------
class vtkScivisWidgets::Internals
{
public:
  std::vector<vtkSmartPointer<vtkAbstractWidget>> Widgets;
};

//------------------------------------------------------------------------------
vtkScivisWidgets::vtkScivisWidgets()
{
  this->Implementation = new Internals();
}

//------------------------------------------------------------------------------
vtkScivisWidgets::~vtkScivisWidgets()
{
  this->RemoveAll();
  delete this->Implementation;
}

//------------------------------------------------------------------------------
void vtkScivisWidgets::SetView(vtkScivisView* view)
{
  this->View = view;
}

//------------------------------------------------------------------------------
void vtkScivisWidgets::PlaceOverScene(vtkAbstractWidget* widget)
{
  if (!this->View)
  {
    return;
  }
  widget->CreateDefaultRepresentation();
  vtkWidgetRepresentation* representation = widget->GetRepresentation();
  double bounds[6];
  if (representation && this->View->GetSceneBounds(bounds))
  {
    // Representations shrink what they are placed over by a place factor of
    // 0.5 unless told otherwise, which would leave the widget around the middle
    // half of the data rather than around the data.
    representation->SetPlaceFactor(1.0);
    representation->PlaceWidget(bounds);
  }
}

//------------------------------------------------------------------------------
void vtkScivisWidgets::Add(vtkAbstractWidget* widget)
{
  if (!widget)
  {
    return;
  }
  if (!this->View)
  {
    vtkErrorMacro("There is no view to add a widget to.");
    return;
  }

  auto& widgets = this->Implementation->Widgets;
  if (std::find(widgets.begin(), widgets.end(), widget) != widgets.end())
  {
    return;
  }

  widget->SetInteractor(this->View->GetInteractor());
  widget->SetDefaultRenderer(this->View->GetRenderer());
  widget->SetEnabled(1);

  widgets.emplace_back(widget);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkScivisWidgets::Remove(vtkAbstractWidget* widget)
{
  auto& widgets = this->Implementation->Widgets;
  auto found = std::find(widgets.begin(), widgets.end(), widget);
  if (found == widgets.end())
  {
    return;
  }
  widget->SetEnabled(0);
  widgets.erase(found);
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkScivisWidgets::RemoveAll()
{
  auto& widgets = this->Implementation->Widgets;
  if (widgets.empty())
  {
    return;
  }
  for (auto& widget : widgets)
  {
    widget->SetEnabled(0);
  }
  widgets.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkScivisWidgets::GetNumberOfWidgets()
{
  return static_cast<int>(this->Implementation->Widgets.size());
}

//------------------------------------------------------------------------------
vtkAbstractWidget* vtkScivisWidgets::GetWidget(int index)
{
  auto& widgets = this->Implementation->Widgets;
  if (index < 0 || index >= static_cast<int>(widgets.size()))
  {
    return nullptr;
  }
  return widgets[index];
}

//------------------------------------------------------------------------------
vtkDistanceWidget* vtkScivisWidgets::AddDistance()
{
  vtkNew<vtkDistanceRepresentation2D> representation;
  // 3D handles, so the ends can be picked and dragged in a 3D scene rather than
  // only in the plane of the screen.
  vtkNew<vtkPointHandleRepresentation3D> handles;
  representation->SetHandleRepresentation(handles);

  auto widget = vtkSmartPointer<vtkDistanceWidget>::New();
  widget->SetRepresentation(representation);
  this->Add(widget);
  return widget;
}

//------------------------------------------------------------------------------
vtkAngleWidget* vtkScivisWidgets::AddAngle()
{
  vtkNew<vtkAngleRepresentation2D> representation;
  vtkNew<vtkPointHandleRepresentation3D> handles;
  representation->SetHandleRepresentation(handles);

  auto widget = vtkSmartPointer<vtkAngleWidget>::New();
  widget->SetRepresentation(representation);
  this->Add(widget);
  return widget;
}

//------------------------------------------------------------------------------
vtkCaptionWidget* vtkScivisWidgets::AddCaption(const char* text, double x, double y, double z)
{
  vtkNew<vtkCaptionRepresentation> representation;
  double anchor[3] = { x, y, z };
  representation->SetAnchorPosition(anchor);
  representation->GetCaptionActor2D()->SetCaption(text ? text : "");

  auto widget = vtkSmartPointer<vtkCaptionWidget>::New();
  widget->SetRepresentation(representation);
  this->Add(widget);
  return widget;
}

//------------------------------------------------------------------------------
vtkLineWidget2* vtkScivisWidgets::AddLine()
{
  auto widget = vtkSmartPointer<vtkLineWidget2>::New();
  this->PlaceOverScene(widget);
  this->Add(widget);
  return widget;
}

//------------------------------------------------------------------------------
vtkImplicitPlaneWidget2* vtkScivisWidgets::AddPlane()
{
  auto widget = vtkSmartPointer<vtkImplicitPlaneWidget2>::New();
  this->PlaceOverScene(widget);
  this->Add(widget);
  return widget;
}

//------------------------------------------------------------------------------
vtkBoxWidget2* vtkScivisWidgets::AddBox()
{
  auto widget = vtkSmartPointer<vtkBoxWidget2>::New();
  this->PlaceOverScene(widget);
  this->Add(widget);
  return widget;
}

//------------------------------------------------------------------------------
vtkSplineWidget2* vtkScivisWidgets::AddSpline(int handles)
{
  vtkNew<vtkSplineRepresentation> representation;
  representation->SetNumberOfHandles(handles);

  auto widget = vtkSmartPointer<vtkSplineWidget2>::New();
  widget->SetRepresentation(representation);
  this->PlaceOverScene(widget);
  this->Add(widget);
  return widget;
}

//------------------------------------------------------------------------------
void vtkScivisWidgets::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfWidgets: " << this->GetNumberOfWidgets() << "\n";
  os << indent << "View: " << (this->View ? "set" : "(none)") << "\n";
}
VTK_ABI_NAMESPACE_END
