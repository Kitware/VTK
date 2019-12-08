/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRMenuRepresentation.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenVRMenuRepresentation.h"

#include "vtkEventData.h"
#include "vtkMatrix4x4.h"
#include "vtkOpenVRRenderWindow.h"
#include "vtkTransform.h"

#include "vtkActor.h"
#include "vtkAssemblyPath.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkOpenGLState.h"
#include "vtkPickingManager.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor3D.h"
#include "vtkRenderer.h"
#include "vtkTextActor3D.h"
#include "vtkTextProperty.h"
#include "vtkWindow.h"

#include "vtkSmartPointer.h"

#include "vtkWidgetEvent.h"

//#include "vtk_glew.h"

vtkStandardNewMacro(vtkOpenVRMenuRepresentation);

class vtkOpenVRMenuRepresentation::InternalElement
{
public:
  vtkNew<vtkTextActor3D> TextActor;
  vtkCommand* Command;
  std::string Name;

  InternalElement()
  {
    vtkTextProperty* prop = this->TextActor->GetTextProperty();
    this->TextActor->ForceOpaqueOn();

    prop->SetFontFamilyToTimes();
    prop->SetFrame(1);
    prop->SetFrameWidth(12);
    prop->SetFrameColor(1.0, 1.0, 1.0);
    prop->SetFrameColor(0.0, 0.0, 0.0);
    prop->SetBackgroundOpacity(1.0);
    prop->SetBackgroundColor(0.0, 0.0, 0.0);
    prop->SetFontSize(32);
  }
};

//----------------------------------------------------------------------------
vtkOpenVRMenuRepresentation::vtkOpenVRMenuRepresentation()
{
  this->VisibilityOff();
}

//----------------------------------------------------------------------------
vtkOpenVRMenuRepresentation::~vtkOpenVRMenuRepresentation()
{
  this->RemoveAllMenuItems();
}

void vtkOpenVRMenuRepresentation::PushFrontMenuItem(
  const char* name, const char* text, vtkCommand* cmd)
{
  vtkOpenVRMenuRepresentation::InternalElement* el =
    new vtkOpenVRMenuRepresentation::InternalElement();
  el->TextActor->SetInput(text);
  el->Command = cmd;
  el->Name = name;
  this->Menus.push_front(el);
  this->Modified();
}

void vtkOpenVRMenuRepresentation::RenameMenuItem(const char* name, const char* text)
{
  for (auto itr : this->Menus)
  {
    if (itr->Name == name)
    {
      itr->TextActor->SetInput(text);
      this->Modified();
    }
  }
}

void vtkOpenVRMenuRepresentation::RemoveMenuItem(const char* name)
{
  for (auto itr = this->Menus.begin(); itr != this->Menus.end(); ++itr)
  {
    if ((*itr)->Name == name)
    {
      delete *itr;
      this->Menus.erase(itr);
      this->Modified();
      return;
    }
  }
}

void vtkOpenVRMenuRepresentation::RemoveAllMenuItems()
{
  while (this->Menus.size() > 0)
  {
    auto itr = this->Menus.begin();
    delete *itr;
    this->Menus.erase(itr);
  }
}

void vtkOpenVRMenuRepresentation::StartComplexInteraction(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void* calldata)
{
  vtkEventData* edata = static_cast<vtkEventData*>(calldata);
  vtkEventDataDevice3D* ed = edata->GetAsEventDataDevice3D();
  if (ed)
  {
    this->CurrentOption = 0;
    this->Modified();
    this->BuildRepresentation();
    this->VisibilityOn();
  }
}

void vtkOpenVRMenuRepresentation::EndComplexInteraction(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long, void*)
{
  this->VisibilityOff();
}

void vtkOpenVRMenuRepresentation::ComplexInteraction(
  vtkRenderWindowInteractor*, vtkAbstractWidget*, unsigned long event, void* calldata)
{
  switch (event)
  {
    case vtkWidgetEvent::Select3D:
    {
      this->VisibilityOff();
      long count = 0;
      for (auto& menu : this->Menus)
      {
        if (count == std::lround(this->CurrentOption))
        {
          menu->Command->Execute(this, vtkWidgetEvent::Select3D,
            static_cast<void*>(const_cast<char*>(menu->Name.c_str())));
        }
        count++;
      }
    }
    break;

    case vtkWidgetEvent::Move3D:
    {
      vtkEventData* edata = static_cast<vtkEventData*>(calldata);
      vtkEventDataDevice3D* ed = edata->GetAsEventDataDevice3D();
      if (!ed)
      {
        return;
      }
      const double* dir = ed->GetWorldDirection();
      // adjust CurrentOption based on controller orientation
      vtkOpenVRRenderWindow* renWin =
        static_cast<vtkOpenVRRenderWindow*>(this->Renderer->GetRenderWindow());
      double* vup = renWin->GetPhysicalViewUp();
      double dot = vtkMath::Dot(dir, vup);
      this->CurrentOption -= dot * 0.12;
      if (this->CurrentOption < 0)
      {
        this->CurrentOption = 0.0;
      }
      else if (this->CurrentOption > this->Menus.size() - 1)
      {
        this->CurrentOption = this->Menus.size() - 1;
      }
      this->BuildRepresentation();
    }
  }
}

//----------------------------------------------------------------------------
void vtkOpenVRMenuRepresentation::ReleaseGraphicsResources(vtkWindow* w)
{
  for (auto& menu : this->Menus)
  {
    menu->TextActor->ReleaseGraphicsResources(w);
  }
}

//----------------------------------------------------------------------------
int vtkOpenVRMenuRepresentation::RenderOverlay(vtkViewport* v)
{
  if (!this->GetVisibility())
  {
    return 0;
  }

  vtkOpenVRRenderWindow* renWin =
    static_cast<vtkOpenVRRenderWindow*>(this->Renderer->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();

  // always draw over the rest
  ostate->vtkglDepthFunc(GL_ALWAYS);
  for (auto& menu : this->Menus)
  {
    menu->TextActor->RenderOpaqueGeometry(v);
  }
  ostate->vtkglDepthFunc(GL_LEQUAL);

  return static_cast<int>(this->Menus.size());
}

//-----------------------------------------------------------------------------
vtkTypeBool vtkOpenVRMenuRepresentation::HasTranslucentPolygonalGeometry()
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkOpenVRMenuRepresentation::BuildRepresentation()
{
  vtkOpenVRRenderWindow* renWin =
    static_cast<vtkOpenVRRenderWindow*>(this->Renderer->GetRenderWindow());
  double physicalScale = renWin->GetPhysicalScale();

  if (this->GetMTime() > this->BuildTime)
  {
    // Compute camera position and orientation
    vtkCamera* cam = this->Renderer->GetActiveCamera();
    cam->GetPosition(this->PlacedPos);
    double* dop = cam->GetDirectionOfProjection();
    vtkMath::Normalize(dop);

    renWin->GetPhysicalViewUp(this->PlacedVUP);
    double vupdot = vtkMath::Dot(dop, this->PlacedVUP);
    if (fabs(vupdot) < 0.999)
    {
      this->PlacedDOP[0] = dop[0] - this->PlacedVUP[0] * vupdot;
      this->PlacedDOP[1] = dop[1] - this->PlacedVUP[1] * vupdot;
      this->PlacedDOP[2] = dop[2] - this->PlacedVUP[2] * vupdot;
      vtkMath::Normalize(this->PlacedDOP);
    }
    else
    {
      renWin->GetPhysicalViewDirection(this->PlacedDOP);
    }
    vtkMath::Cross(this->PlacedDOP, this->PlacedVUP, this->PlacedVRight);

    vtkNew<vtkMatrix4x4> rot;
    for (int i = 0; i < 3; ++i)
    {
      rot->SetElement(0, i, this->PlacedVRight[i]);
      rot->SetElement(1, i, this->PlacedVUP[i]);
      rot->SetElement(2, i, -this->PlacedDOP[i]);
    }
    rot->Transpose();
    vtkTransform::GetOrientation(this->PlacedOrientation, rot);

    this->BuildTime.Modified();
  }

  // Set frame distance to camera
  double frameDistance = physicalScale * 1.5; // 1.5 meters

  double fov = this->Renderer->GetActiveCamera()->GetViewAngle();
  double psize = frameDistance * 0.03 * 2.0 * atan(fov * 0.5); // 3% of fov
  double tscale = psize / 55.0;                                // about 55 pixel high texture map

  // Frame position
  double frameCenter[3];

  long count = 0;
  for (auto& menu : this->Menus)
  {
    double shift = count - this->CurrentOption;
    long icurr = std::lround(this->CurrentOption);

    if (count == icurr)
    {
      menu->TextActor->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
    }
    else
    {
      menu->TextActor->GetTextProperty()->SetColor(0.6, 0.6, 0.6);
    }
    double angle = -shift * 2.0 * 3.1415926 / 180.0; // about 2 degree vert sep
    double fdist = frameDistance * (1.0 + 3.0 * (1.0 - cos(angle)));
    double udist = 3.0 * frameDistance * sin(angle);

    frameCenter[0] = this->PlacedPos[0] + fdist * this->PlacedDOP[0] -
      psize * this->PlacedVRight[0] + udist * this->PlacedVUP[0];
    frameCenter[1] = this->PlacedPos[1] + fdist * this->PlacedDOP[1] -
      psize * this->PlacedVRight[1] + udist * this->PlacedVUP[1];
    frameCenter[2] = this->PlacedPos[2] + fdist * this->PlacedDOP[2] -
      psize * this->PlacedVRight[2] + udist * this->PlacedVUP[2];

    menu->TextActor->SetScale(tscale, tscale, tscale);
    menu->TextActor->SetPosition(frameCenter);
    menu->TextActor->SetOrientation(this->PlacedOrientation);
    menu->TextActor->RotateX(-angle * 180.0 / 3.1415926);
    count++;
  }
}

//----------------------------------------------------------------------------
void vtkOpenVRMenuRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
