/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointHandleRepresentation2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointHandleRepresentation2D.h"
#include "vtkActor2D.h"
#include "vtkAssemblyPath.h"
#include "vtkCamera.h"
#include "vtkCoordinate.h"
#include "vtkCursor2D.h"
#include "vtkGlyph2D.h"
#include "vtkInteractorObserver.h"
#include "vtkLine.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointPlacer.h"
#include "vtkPoints.h"
#include "vtkPolyDataAlgorithm.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkWindow.h"

vtkStandardNewMacro(vtkPointHandleRepresentation2D);

vtkCxxSetObjectMacro(vtkPointHandleRepresentation2D, Property, vtkProperty2D);
vtkCxxSetObjectMacro(vtkPointHandleRepresentation2D, SelectedProperty, vtkProperty2D);
vtkCxxSetObjectMacro(vtkPointHandleRepresentation2D, PointPlacer, vtkPointPlacer);

//----------------------------------------------------------------------
vtkPointHandleRepresentation2D::vtkPointHandleRepresentation2D()
{
  // Initialize state
  this->InteractionState = vtkHandleRepresentation::Outside;

  // Represent the position of the cursor
  this->FocalPoint = vtkPoints::New();
  this->FocalPoint->SetNumberOfPoints(1);
  this->FocalPoint->SetPoint(0, 0.0, 0.0, 0.0);

  this->FocalData = vtkPolyData::New();
  this->FocalData->SetPoints(this->FocalPoint);

  // The transformation of the cursor will be done via vtkGlyph2D
  // By default a vtkGlyphSOurce2D will be used to define the cursor shape
  vtkCursor2D* cursor2D = vtkCursor2D::New();
  cursor2D->AllOff();
  cursor2D->AxesOn();
  cursor2D->PointOn();
  cursor2D->Update();
  this->CursorShape = cursor2D->GetOutput();
  this->CursorShape->Register(this);
  cursor2D->Delete();

  this->Glypher = vtkGlyph2D::New();
  this->Glypher->SetInputData(this->FocalData);
  this->Glypher->SetSourceData(this->CursorShape);
  this->Glypher->SetVectorModeToVectorRotationOff();
  this->Glypher->ScalingOn();
  this->Glypher->SetScaleModeToDataScalingOff();
  this->Glypher->SetScaleFactor(1.0);

  this->MapperCoordinate = vtkCoordinate::New();
  this->MapperCoordinate->SetCoordinateSystemToDisplay();

  this->Mapper = vtkPolyDataMapper2D::New();
  this->Mapper->SetInputConnection(this->Glypher->GetOutputPort());
  this->Mapper->SetTransformCoordinate(this->MapperCoordinate);

  // Set up the initial properties
  this->CreateDefaultProperties();

  this->Actor = vtkActor2D::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);

  // The size of the hot spot
  this->WaitingForMotion = 0;
}

//----------------------------------------------------------------------
vtkPointHandleRepresentation2D::~vtkPointHandleRepresentation2D()
{
  this->FocalPoint->Delete();
  this->FocalData->Delete();

  this->CursorShape->Delete();
  this->Glypher->Delete();
  this->MapperCoordinate->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();

  this->Property->Delete();
  this->SelectedProperty->Delete();
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation2D::SetCursorShape(vtkPolyData* shape)
{
  if (shape != this->CursorShape)
  {
    if (this->CursorShape)
    {
      this->CursorShape->Delete();
    }
    this->CursorShape = shape;
    if (this->CursorShape)
    {
      this->CursorShape->Register(this);
    }
    this->Glypher->SetSourceData(this->CursorShape);
    this->Modified();
  }
}

//----------------------------------------------------------------------
vtkPolyData* vtkPointHandleRepresentation2D::GetCursorShape()
{
  return this->CursorShape;
}

//-------------------------------------------------------------------------
double* vtkPointHandleRepresentation2D::GetBounds()
{
  return nullptr;
}

//-------------------------------------------------------------------------
void vtkPointHandleRepresentation2D::SetDisplayPosition(double p[3])
{
  this->Superclass::SetDisplayPosition(p);
  this->FocalPoint->SetPoint(0, p);
  this->FocalPoint->Modified();

  if (this->PointPlacer)
  {
    // The point placer will compute the world position for us.
    return;
  }

  double w[4];
  if (this->Renderer)
  {
    vtkInteractorObserver::ComputeDisplayToWorld(this->Renderer, p[0], p[1], p[2], w);
    this->SetWorldPosition(w);
  }
}

//-------------------------------------------------------------------------
int vtkPointHandleRepresentation2D::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  double pos[3], xyz[3];
  this->FocalPoint->GetPoint(0, pos);
  xyz[0] = static_cast<double>(X);
  xyz[1] = static_cast<double>(Y);
  xyz[2] = pos[2];

  this->VisibilityOn();
  double tol2 = this->Tolerance * this->Tolerance;
  if (vtkMath::Distance2BetweenPoints(xyz, pos) <= tol2)
  {
    this->InteractionState = vtkHandleRepresentation::Nearby;
  }
  else
  {
    this->InteractionState = vtkHandleRepresentation::Outside;
    if (this->ActiveRepresentation)
    {
      this->VisibilityOff();
    }
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------
// Record the current event position, and the rectilinear wipe position.
void vtkPointHandleRepresentation2D::StartWidgetInteraction(double startEventPos[2])
{
  this->StartEventPosition[0] = startEventPos[0];
  this->StartEventPosition[1] = startEventPos[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = startEventPos[0];
  this->LastEventPosition[1] = startEventPos[1];

  this->WaitCount = 0;
  if (this->IsTranslationConstrained())
  {
    this->WaitingForMotion = 1;
  }
  else
  {
    this->WaitingForMotion = 0;
  }
}

//----------------------------------------------------------------------
// Based on the displacement vector (computed in display coordinates) and
// the cursor state (which corresponds to which part of the widget has been
// selected), the widget points are modified.
// First construct a local coordinate system based on the display coordinates
// of the widget.
void vtkPointHandleRepresentation2D::WidgetInteraction(double eventPos[2])
{
  // Process the motion
  if (this->InteractionState == vtkHandleRepresentation::Selecting ||
    this->InteractionState == vtkHandleRepresentation::Translating)
  {
    if (!this->WaitingForMotion || this->WaitCount++ > 1)
    {
      this->Translate(eventPos);
    }
  }

  else if (this->InteractionState == vtkHandleRepresentation::Scaling)
  {
    this->Scale(eventPos);
  }

  // Book keeping
  this->LastEventPosition[0] = eventPos[0];
  this->LastEventPosition[1] = eventPos[1];

  this->Modified();
}

//----------------------------------------------------------------------
// Translate everything
void vtkPointHandleRepresentation2D::Translate(const double* eventPos)
{
  double pos[3];
  this->FocalPoint->GetPoint(0, pos);
  if (this->IsTranslationConstrained())
  {
    pos[this->TranslationAxis] += eventPos[this->TranslationAxis] - pos[this->TranslationAxis];
  }
  else
  {
    pos[0] += eventPos[0] - pos[0];
    pos[1] += eventPos[1] - pos[1];
  }
  this->SetDisplayPosition(pos);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation2D::Scale(const double eventPos[2])
{
  // Get the current scale factor
  double sf = this->Glypher->GetScaleFactor();

  // Compute the scale factor
  int* size = this->Renderer->GetSize();
  double dPos = static_cast<double>(eventPos[1] - this->LastEventPosition[1]);
  sf *= (1.0 + 2.0 * (dPos / size[1])); // scale factor of 2.0 is arbitrary

  // Scale the handle
  this->Glypher->SetScaleFactor(sf);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation2D::Highlight(int highlight)
{
  if (highlight)
  {
    this->Actor->SetProperty(this->SelectedProperty);
  }
  else
  {
    this->Actor->SetProperty(this->Property);
  }
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation2D::CreateDefaultProperties()
{
  this->Property = vtkProperty2D::New();
  this->Property->SetColor(1.0, 1.0, 1.0);
  this->Property->SetLineWidth(1.0);

  this->SelectedProperty = vtkProperty2D::New();
  this->SelectedProperty->SetColor(0.0, 1.0, 0.0);
  this->SelectedProperty->SetLineWidth(2.0);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation2D::BuildRepresentation()
{
  if (this->GetMTime() > this->BuildTime ||
    (this->Renderer && this->Renderer->GetActiveCamera() &&
      this->Renderer->GetActiveCamera()->GetMTime() > this->BuildTime) ||
    (this->Renderer && this->Renderer->GetVTKWindow() &&
      this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime))
  {
    double p[3];
    this->GetDisplayPosition(p);
    this->FocalPoint->SetPoint(0, p);
    this->FocalPoint->Modified();
    this->BuildTime.Modified();
  }
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation2D::ShallowCopy(vtkProp* prop)
{
  vtkPointHandleRepresentation2D* rep = vtkPointHandleRepresentation2D::SafeDownCast(prop);
  if (rep)
  {
    this->SetCursorShape(rep->GetCursorShape());
    this->SetProperty(rep->GetProperty());
    this->SetSelectedProperty(rep->GetSelectedProperty());
    this->Actor->SetProperty(this->Property);
  }
  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation2D::DeepCopy(vtkProp* prop)
{
  vtkPointHandleRepresentation2D* rep = vtkPointHandleRepresentation2D::SafeDownCast(prop);
  if (rep)
  {
    this->SetCursorShape(rep->GetCursorShape());
    this->Property->DeepCopy(rep->GetProperty());
    this->SelectedProperty->DeepCopy(rep->GetSelectedProperty());
    this->Actor->SetProperty(this->Property);
  }
  this->Superclass::DeepCopy(prop);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation2D::GetActors2D(vtkPropCollection* pc)
{
  this->Actor->GetActors2D(pc);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation2D::ReleaseGraphicsResources(vtkWindow* win)
{
  this->Actor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkPointHandleRepresentation2D::RenderOverlay(vtkViewport* viewport)
{
  this->BuildRepresentation();
  return this->Actor->RenderOverlay(viewport);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation2D::SetVisibility(vtkTypeBool visible)
{
  this->Actor->SetVisibility(visible);
  // Forward to superclass
  this->Superclass::SetVisibility(visible);
}

//----------------------------------------------------------------------
void vtkPointHandleRepresentation2D::PrintSelf(ostream& os, vtkIndent indent)
{
  // Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  if (this->Property)
  {
    os << indent << "Property: " << this->Property << "\n";
  }
  else
  {
    os << indent << "Property: (none)\n";
  }

  if (this->SelectedProperty)
  {
    os << indent << "Selected Property: " << this->SelectedProperty << "\n";
  }
  else
  {
    os << indent << "Selected Property: (none)\n";
  }

  if (this->CursorShape)
  {
    os << indent << "Cursor Shape: " << this->CursorShape << "\n";
  }
  else
  {
    os << indent << "Cursor Shape: (none)\n";
  }
}
