/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointCloudRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointCloudRepresentation.h"
#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkDataSetMapper.h"
#include "vtkFloatArray.h"
#include "vtkGlyphSource2D.h"
#include "vtkInteractorObserver.h"
#include "vtkMapper2D.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineFilter.h"
#include "vtkPicker.h"
#include "vtkPickingManager.h"
#include "vtkPointData.h"
#include "vtkPointGaussianMapper.h"
#include "vtkPointPicker.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkPropCollection.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkWindow.h"
#

vtkStandardNewMacro(vtkPointCloudRepresentation);

//-------------------------------------------------------------------------
vtkPointCloudRepresentation::vtkPointCloudRepresentation()
{
  // Internal state
  this->PointCloud = nullptr;
  this->PointCloudMapper = nullptr;
  this->PointCloudActor = nullptr;
  this->Highlighting = true;
  this->PointId = (-1);
  this->PointCoordinates[0] = this->PointCoordinates[1] = this->PointCoordinates[2] = 0.0;
  this->InteractionState = vtkPointCloudRepresentation::Outside;
  this->Tolerance = 0.0001; // fraction of bounding box

  // Manage the picking stuff. Note that for huge point clouds the picking may not
  // be fast enough: TODO: use a rendering-based point picker, or further accelerate
  // vtkPointPicker with the use of a point locator.
  this->OutlinePicker = vtkPicker::New();
  this->OutlinePicker->PickFromListOn();
  this->PointPicker = vtkPointPicker::New();
  this->PointPicker->PickFromListOn();

  // The outline around the points
  this->OutlineFilter = vtkOutlineFilter::New();

  this->OutlineMapper = vtkPolyDataMapper::New();
  this->OutlineMapper->SetInputConnection(this->OutlineFilter->GetOutputPort());

  this->OutlineActor = vtkActor::New();
  this->OutlineActor->SetMapper(this->OutlineMapper);

  // Create the selection prop
  this->SelectionShape = vtkGlyphSource2D::New();
  this->SelectionShape->SetGlyphTypeToCircle();
  this->SelectionShape->SetResolution(32);
  this->SelectionShape->SetScale(10);

  this->SelectionMapper = vtkPolyDataMapper2D::New();
  this->SelectionMapper->SetInputConnection(this->SelectionShape->GetOutputPort());

  this->SelectionActor = vtkActor2D::New();
  this->SelectionActor->SetMapper(this->SelectionMapper);

  // Set up the initial selection properties
  this->CreateDefaultProperties();
  this->SelectionActor->SetProperty(this->SelectionProperty);
}

//-------------------------------------------------------------------------
vtkPointCloudRepresentation::~vtkPointCloudRepresentation()
{
  if (this->PointCloud)
  {
    this->PointCloud->Delete();
    this->PointCloudMapper->Delete();
    this->PointCloudActor->Delete();
  }
  this->OutlinePicker->Delete();
  this->PointPicker->Delete();
  this->OutlineFilter->Delete();
  this->OutlineMapper->Delete();
  this->OutlineActor->Delete();
  this->SelectionShape->Delete();
  this->SelectionMapper->Delete();
  this->SelectionActor->Delete();
  this->SelectionProperty->Delete();
}

//----------------------------------------------------------------------
void vtkPointCloudRepresentation::CreateDefaultProperties()
{
  this->SelectionProperty = vtkProperty2D::New();
  this->SelectionProperty->SetColor(1.0, 1.0, 1.0);
  this->SelectionProperty->SetLineWidth(1.0);
}

//-------------------------------------------------------------------------
void vtkPointCloudRepresentation::PlacePointCloud(vtkActor* a)
{
  // Return if nothing has changed
  if (a == this->PointCloudActor)
  {
    return;
  }

  // Make sure the prop has associated data of the proper type
  vtkPolyDataMapper* mapper = reinterpret_cast<vtkPolyDataMapper*>(a->GetMapper());
  vtkPointSet* pc =
    (mapper == nullptr ? nullptr : reinterpret_cast<vtkPointSet*>(mapper->GetInput()));
  if (mapper == nullptr || pc == nullptr)
  {
    if (this->PointCloud != nullptr)
    {
      this->PointCloud->Delete();
      this->PointCloud = nullptr;
    }
    if (this->PointCloudMapper != nullptr)
    {
      this->PointCloudMapper->Delete();
      this->PointCloudMapper = nullptr;
    }
    if (this->PointCloudActor != nullptr)
    {
      this->PointCloudActor->Delete();
      this->PointCloudActor = nullptr;
    }
    return;
  }

  // Restructure the pipeline
  if (this->PointCloud != nullptr)
  {
    this->PointCloud->Delete();
    this->PointCloudMapper->Delete();
    this->PointCloudActor->Delete();
  }
  this->PointCloud = pc;
  this->PointCloudMapper = mapper;
  this->PointCloudActor = a;
  this->PointCloud->Register(this);
  this->PointCloudMapper->Register(this);
  this->PointCloudActor->Register(this);

  this->OutlinePicker->InitializePickList();
  this->OutlinePicker->AddPickList(a);

  this->PointPicker->InitializePickList();
  this->PointPicker->AddPickList(a);

  this->PlaceWidget(pc->GetBounds());

  this->OutlineFilter->SetInputData(pc);

  this->Modified();
}

//-------------------------------------------------------------------------
// If specifying point set, create our own actor and mapper
void vtkPointCloudRepresentation::PlacePointCloud(vtkPointSet* pc)
{
  // Return if nothing has changed
  if (pc == this->PointCloud)
  {
    return;
  }

  // Reconstruct the pipeline
  vtkNew<vtkActor> actor;

  if (vtkPolyData::SafeDownCast(pc))
  {
    vtkNew<vtkPointGaussianMapper> mapper;
    mapper->EmissiveOff();
    mapper->SetScaleFactor(0.0);
    mapper->SetInputData(vtkPolyData::SafeDownCast(pc));
    actor->SetMapper(mapper);
  }
  else
  {
    vtkNew<vtkDataSetMapper> mapper;
    mapper->SetInputData(pc);
    actor->SetMapper(mapper);
  }

  this->PlacePointCloud(actor);
}

//-------------------------------------------------------------------------
double* vtkPointCloudRepresentation::GetBounds()
{
  if (this->PointCloudActor)
  {
    return this->PointCloudActor->GetBounds();
  }
  else
  {
    return nullptr;
  }
}

//-------------------------------------------------------------------------
int vtkPointCloudRepresentation::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  // Need a tolerance for picking
  double tol = this->Tolerance * this->InitialLength;

  // First pick the bounding box
  this->OutlinePicker->SetTolerance(tol);
  vtkAssemblyPath* path = this->GetAssemblyPath(X, Y, 0., this->OutlinePicker);
  if (path != nullptr)
  {
    this->InteractionState = vtkPointCloudRepresentation::OverOutline;
    this->OutlineActor->VisibilityOn();
    // Now see if we can pick a point (with the appropriate tolerance)
    this->PointPicker->SetTolerance(tol);
    //    this->PointPicker->SetTolerance(0.004);
    path = this->GetAssemblyPath(X, Y, 0., this->PointPicker);
    if (path != nullptr)
    {
      this->InteractionState = vtkPointCloudRepresentation::Over;
      // Create a tolerance and update the pick position
      double center[4], p[4];
      this->PointId = this->PointPicker->GetPointId();
      this->PointPicker->GetPickPosition(p); // in world coordinates
      vtkInteractorObserver::ComputeWorldToDisplay(this->Renderer, p[0], p[1], p[2], center);
      this->PointCoordinates[0] = p[0];
      this->PointCoordinates[1] = p[1];
      this->PointCoordinates[2] = p[2];
      this->SelectionShape->SetCenter(center);
      this->SelectionActor->VisibilityOn();
    }
    else
    {
      this->PointId = (-1);
      this->SelectionActor->VisibilityOff();
    }
  }
  else
  {
    this->InteractionState = vtkPointCloudRepresentation::Outside;
    this->OutlineActor->VisibilityOff();
  }

  return this->InteractionState;
}

//-------------------------------------------------------------------------
void vtkPointCloudRepresentation::GetActors2D(vtkPropCollection* pc)
{
  pc->AddItem(this->SelectionActor);
  this->Superclass::GetActors2D(pc);
}

//-------------------------------------------------------------------------
void vtkPointCloudRepresentation::ReleaseGraphicsResources(vtkWindow* w)
{
  if (this->PointCloudActor)
  {
    this->PointCloudActor->ReleaseGraphicsResources(w);
  }
  this->OutlineActor->ReleaseGraphicsResources(w);
}

//----------------------------------------------------------------------
int vtkPointCloudRepresentation::RenderOpaqueGeometry(vtkViewport* viewport)
{
  int count = 0;
  if (this->PointCloudActor && !this->Renderer->HasViewProp(this->PointCloudActor))
  {
    count += this->PointCloudActor->RenderOpaqueGeometry(viewport);
  }

  if (this->OutlineActor->GetVisibility())
  {
    count += this->OutlineActor->RenderOpaqueGeometry(viewport);
  }
  return count;
}

//-----------------------------------------------------------------------------
int vtkPointCloudRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport* viewport)
{
  int count = 0;
  if (this->PointCloudActor && !this->Renderer->HasViewProp(this->PointCloudActor))
  {
    count += this->PointCloudActor->RenderTranslucentPolygonalGeometry(viewport);
  }

  if (this->OutlineActor->GetVisibility())
  {
    count += this->OutlineActor->RenderTranslucentPolygonalGeometry(viewport);
  }
  return count;
}

//-----------------------------------------------------------------------------
vtkTypeBool vtkPointCloudRepresentation::HasTranslucentPolygonalGeometry()
{
  int result = 0;
  if (this->PointCloudActor && !this->Renderer->HasViewProp(this->PointCloudActor))
  {
    result |= this->PointCloudActor->HasTranslucentPolygonalGeometry();
  }

  if (this->OutlineActor->GetVisibility())
  {
    result |= this->OutlineActor->HasTranslucentPolygonalGeometry();
  }

  return result;
}

//-------------------------------------------------------------------------
int vtkPointCloudRepresentation::RenderOverlay(vtkViewport* v)
{
  int count = 0;
  if (this->PointId != (-1) && this->Highlighting)
  {
    vtkRenderer* ren = vtkRenderer::SafeDownCast(v);
    if (ren)
    {
      count += this->SelectionActor->RenderOverlay(v);
    }
  }
  return count;
}

//----------------------------------------------------------------------
void vtkPointCloudRepresentation::RegisterPickers()
{
  vtkPickingManager* pm = this->GetPickingManager();
  if (!pm)
  {
    return;
  }
  pm->AddPicker(this->PointPicker, this);
}

//-------------------------------------------------------------------------
void vtkPointCloudRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->PointCloudActor)
  {
    os << indent << "Point Cloud Actor: " << this->PointCloudActor << "\n";
  }
  else
  {
    os << indent << "Point Cloud Actor: (none)\n";
  }

  os << indent << "Point Id: " << this->PointId << "\n";
  os << indent << "Point Coordinates: (" << this->PointCoordinates[0] << ","
     << this->PointCoordinates[1] << "," << this->PointCoordinates[2] << ")\n";

  os << indent << "Highlighting: " << (this->Highlighting ? "On" : "Off") << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";

  if (this->SelectionProperty)
  {
    os << indent << "Selection Property: " << this->SelectionProperty << "\n";
  }
  else
  {
    os << indent << "Selection Property: (none)\n";
  }
}
