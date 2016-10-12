/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConstrainedPointHandleRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkConstrainedPointHandleRepresentation.h"
#include "vtkSmartPointer.h"
#include "vtkCellPicker.h"
#include "vtkCleanPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkAssemblyPath.h"
#include "vtkMath.h"
#include "vtkInteractorObserver.h"
#include "vtkLine.h"
#include "vtkCoordinate.h"
#include "vtkGlyph3D.h"
#include "vtkCursor2D.h"
#include "vtkCylinderSource.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkPlane.h"
#include "vtkPlanes.h"
#include "vtkPlaneCollection.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTransform.h"
#include "vtkCamera.h"

vtkStandardNewMacro(vtkConstrainedPointHandleRepresentation);

vtkCxxSetObjectMacro(vtkConstrainedPointHandleRepresentation, ObliquePlane, vtkPlane);
vtkCxxSetObjectMacro(vtkConstrainedPointHandleRepresentation, BoundingPlanes,vtkPlaneCollection);

//----------------------------------------------------------------------
vtkConstrainedPointHandleRepresentation::vtkConstrainedPointHandleRepresentation()
{
  // Initialize state
  this->InteractionState = vtkHandleRepresentation::Outside;

  this->ProjectionPosition = 0;
  this->ObliquePlane = NULL;
  this->ProjectionNormal = vtkConstrainedPointHandleRepresentation::ZAxis;

  this->CursorShape = NULL;
  this->ActiveCursorShape = NULL;

  // Represent the position of the cursor
  this->FocalPoint = vtkPoints::New();
  this->FocalPoint->SetNumberOfPoints(1);
  this->FocalPoint->SetPoint(0, 0.0,0.0,0.0);

  vtkSmartPointer<vtkDoubleArray> normals =
    vtkSmartPointer<vtkDoubleArray>::New();
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(1);

  double normal[3];
  this->GetProjectionNormal( normal );
  normals->SetTuple(0,normal);

  this->FocalData = vtkPolyData::New();
  this->FocalData->SetPoints(this->FocalPoint);
  this->FocalData->GetPointData()->SetNormals(normals);

  this->Glypher = vtkGlyph3D::New();
  this->Glypher->SetInputData(this->FocalData);
  this->Glypher->SetVectorModeToUseNormal();
  this->Glypher->OrientOn();
  this->Glypher->ScalingOn();
  this->Glypher->SetScaleModeToDataScalingOff();
  this->Glypher->SetScaleFactor(1.0);

  // The transformation of the cursor will be done via vtkGlyph3D
  // By default a vtkCursor2D will be used to define the cursor shape
  vtkSmartPointer<vtkCursor2D> cursor2D =
    vtkSmartPointer<vtkCursor2D>::New();
  cursor2D->AllOff();
  cursor2D->PointOn();
  cursor2D->Update();
  this->SetCursorShape( cursor2D->GetOutput() );

  vtkSmartPointer<vtkCylinderSource> cylinder =
    vtkSmartPointer<vtkCylinderSource>::New();
  cylinder->SetResolution(64);
  cylinder->SetRadius(1.0);
  cylinder->SetHeight(0.0);
  cylinder->CappingOff();
  cylinder->SetCenter(0,0,0);

  vtkSmartPointer<vtkCleanPolyData> clean =
    vtkSmartPointer<vtkCleanPolyData>::New();
  clean->PointMergingOn();
  clean->CreateDefaultLocator();
  clean->SetInputConnection(0,cylinder->GetOutputPort(0));

  vtkSmartPointer<vtkTransform> t =
    vtkSmartPointer<vtkTransform>::New();
  t->RotateZ(90.0);

  vtkSmartPointer<vtkTransformPolyDataFilter> tpd =
    vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  tpd->SetInputConnection( 0, clean->GetOutputPort(0) );
  tpd->SetTransform( t );
  tpd->Update();

  this->SetActiveCursorShape(tpd->GetOutput());

  this->Mapper = vtkPolyDataMapper::New();
  this->Mapper->SetInputConnection(
    this->Glypher->GetOutputPort());
  this->Mapper->SetResolveCoincidentTopologyToPolygonOffset();
  this->Mapper->ScalarVisibilityOff();

  // Set up the initial properties
  this->CreateDefaultProperties();

  this->Actor = vtkActor::New();
  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetProperty(this->Property);

  this->InteractionOffset[0] = 0.0;
  this->InteractionOffset[1] = 0.0;

  this->BoundingPlanes = NULL;
}

//----------------------------------------------------------------------
vtkConstrainedPointHandleRepresentation::~vtkConstrainedPointHandleRepresentation()
{
  this->FocalPoint->Delete();
  this->FocalData->Delete();

  this->SetCursorShape( NULL );
  this->SetActiveCursorShape( NULL );

  this->RemoveAllBoundingPlanes();

  this->Glypher->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();

  this->Property->Delete();
  this->SelectedProperty->Delete();
  this->ActiveProperty->Delete();

  if ( this->ObliquePlane )
  {
    this->ObliquePlane->UnRegister(this);
    this->ObliquePlane = NULL;
  }

  if (this->BoundingPlanes)
  {
    this->BoundingPlanes->UnRegister(this);
  }
}

//----------------------------------------------------------------------
int vtkConstrainedPointHandleRepresentation::CheckConstraint(vtkRenderer *renderer,
                                                             double eventPos[2])
{
  double worldPos[3];
  double tolerance = 0.0;
  return  this->GetIntersectionPosition(eventPos, worldPos, tolerance, renderer);
}

//----------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::SetProjectionPosition(double position)
{
  if ( this->ProjectionPosition != position )
  {
    this->ProjectionPosition = position;
    this->Modified();
  }
}

//----------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::SetCursorShape(vtkPolyData *shape)
{
  if ( shape != this->CursorShape )
  {
    if ( this->CursorShape )
    {
      this->CursorShape->Delete();
    }
    this->CursorShape = shape;
    if ( this->CursorShape )
    {
      this->CursorShape->Register(this);
      this->Glypher->SetSourceData(this->CursorShape);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------
vtkPolyData *vtkConstrainedPointHandleRepresentation::GetCursorShape()
{
  return this->CursorShape;
}

//----------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::SetActiveCursorShape(vtkPolyData *shape)
{
  if ( shape != this->ActiveCursorShape )
  {
    if ( this->ActiveCursorShape )
    {
      this->ActiveCursorShape->Delete();
    }
    this->ActiveCursorShape = shape;
    if ( this->CursorShape )
    {
      this->ActiveCursorShape->Register(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------
vtkPolyData *vtkConstrainedPointHandleRepresentation::GetActiveCursorShape()
{
  return this->ActiveCursorShape;
}

//----------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::AddBoundingPlane(vtkPlane *plane)
{
  if (this->BoundingPlanes == NULL)
  {
    this->BoundingPlanes = vtkPlaneCollection::New();
    this->BoundingPlanes->Register(this);
    this->BoundingPlanes->Delete();
  }

  this->BoundingPlanes->AddItem(plane);
}

//----------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::RemoveBoundingPlane(vtkPlane *plane)
{
  if (this->BoundingPlanes )
  {
    this->BoundingPlanes->RemoveItem(plane);
  }
}

//----------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::RemoveAllBoundingPlanes()
{
  if ( this->BoundingPlanes )
  {
    this->BoundingPlanes->RemoveAllItems();
    this->BoundingPlanes->Delete();
    this->BoundingPlanes = NULL;
  }
}
//----------------------------------------------------------------------

void vtkConstrainedPointHandleRepresentation::SetBoundingPlanes(vtkPlanes *planes)
{
  if (!planes)
  {
    return;
  }

  vtkPlane *plane;
  int numPlanes = planes->GetNumberOfPlanes();

  this->RemoveAllBoundingPlanes();
  for (int i=0; i<numPlanes ; i++)
  {
    plane = vtkPlane::New();
    planes->GetPlane(i, plane);
    this->AddBoundingPlane(plane);
    plane->Delete();
  }
}

//----------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::SetRenderer(vtkRenderer *ren)
{
  this->WorldPosition->SetViewport(ren);
  this->Superclass::SetRenderer(ren);
}

//-------------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::SetPosition(double x, double y, double z)
{
  this->WorldPosition->SetValue(x,y,z);
  this->FocalPoint->SetPoint(0, x,y,z);
  this->FocalPoint->Modified();
}

//-------------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::SetDisplayPosition(double eventPos[3])
{
  double worldPos[3];
  this->DisplayPosition->SetValue(eventPos);
  if(this->Renderer)
  {
    if ( this->GetIntersectionPosition(eventPos, worldPos) )
    {
      this->SetPosition(worldPos);
    }
  }
  this->DisplayPositionTime.Modified();
}

//-------------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::SetPosition(double xyz[3])
{
  this->SetPosition(xyz[0],xyz[1],xyz[2]);
}

//-------------------------------------------------------------------------
double *vtkConstrainedPointHandleRepresentation::GetPosition()
{
  return this->FocalPoint->GetPoint(0);
}

//-------------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::GetPosition(double xyz[3])
{
  this->FocalPoint->GetPoint(0, xyz);
}

//-------------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::ShallowCopy(vtkProp* prop)
{
  vtkConstrainedPointHandleRepresentation *rep =
    vtkConstrainedPointHandleRepresentation::SafeDownCast(prop);
  if(rep)
  {
    this->Property->DeepCopy( rep->GetProperty() );
    this->SelectedProperty->DeepCopy(rep->GetSelectedProperty());
    this->ActiveProperty->DeepCopy(rep->GetActiveProperty());
    this->ProjectionNormal = rep->GetProjectionNormal();
    this->ProjectionPosition = rep->GetProjectionPosition();

    this->SetObliquePlane(rep->GetObliquePlane());
    this->SetBoundingPlanes(rep->GetBoundingPlanes());
  }
  this->Superclass::ShallowCopy(prop);
}
//-------------------------------------------------------------------------
int vtkConstrainedPointHandleRepresentation::ComputeInteractionState(
  int X, int Y, int vtkNotUsed(modify))
{

  double pos[4], xyz[3];
  this->FocalPoint->GetPoint(0,pos);
  pos[3] = 1.0;
  this->Renderer->SetWorldPoint(pos);
  this->Renderer->WorldToDisplay();
  this->Renderer->GetDisplayPoint(pos);

  xyz[0] = static_cast<double>(X);
  xyz[1] = static_cast<double>(Y);
  xyz[2] = pos[2];

  this->VisibilityOn();
  double tol2 = this->Tolerance * this->Tolerance;
  if ( vtkMath::Distance2BetweenPoints(xyz,pos) <= tol2 )
  {
    this->InteractionState = vtkHandleRepresentation::Nearby;
    this->Glypher->SetSourceData(this->ActiveCursorShape);
    this->Actor->SetProperty( this->ActiveProperty );
    if ( !this->ActiveCursorShape )
    {
      this->VisibilityOff();
    }
  }
  else
  {
    this->InteractionState = vtkHandleRepresentation::Outside;
    this->Glypher->SetSourceData(this->CursorShape);
    this->Actor->SetProperty( this->Property );
    if ( !this->CursorShape )
    {
      this->VisibilityOff();
    }
  }

  return this->InteractionState;
}

//----------------------------------------------------------------------
// Record the current event position, and the rectilinear wipe position.
void vtkConstrainedPointHandleRepresentation::StartWidgetInteraction(double startEventPos[2])
{
  this->StartEventPosition[0] = startEventPos[0];
  this->StartEventPosition[1] = startEventPos[1];
  this->StartEventPosition[2] = 0.0;

  this->LastEventPosition[0] = startEventPos[0];
  this->LastEventPosition[1] = startEventPos[1];

  // How far is this in pixels from the position of this widget?
  // Maintain this during interaction such as translating (don't
  // force center of widget to snap to mouse position)

  // convert position to display coordinates
  double pos[3];
  this->GetDisplayPosition(pos);

  this->InteractionOffset[0] = pos[0] - startEventPos[0];
  this->InteractionOffset[1] = pos[1] - startEventPos[1];

}


//----------------------------------------------------------------------
// Based on the displacement vector (computed in display coordinates) and
// the cursor state (which corresponds to which part of the widget has been
// selected), the widget points are modified.
// First construct a local coordinate system based on the display coordinates
// of the widget.
void vtkConstrainedPointHandleRepresentation::WidgetInteraction(double eventPos[2])
{
  // Process the motion
  if ( this->InteractionState == vtkHandleRepresentation::Selecting ||
       this->InteractionState == vtkHandleRepresentation::Translating )
  {
    this->Translate(eventPos);
  }

  else if ( this->InteractionState == vtkHandleRepresentation::Scaling )
  {
    this->Scale(eventPos);
  }

  // Book keeping
  this->LastEventPosition[0] = eventPos[0];
  this->LastEventPosition[1] = eventPos[1];
}

//----------------------------------------------------------------------
// Translate everything
void vtkConstrainedPointHandleRepresentation::Translate(double eventPos[2])
{
  double worldPos[3];

  if ( this->GetIntersectionPosition(eventPos, worldPos) )
  {
    this->SetPosition(worldPos);
  }
  else
  {
    // I really want to track the closest point here,
    // but I am postponing this at the moment....
  }
}

//----------------------------------------------------------------------
int vtkConstrainedPointHandleRepresentation::
GetIntersectionPosition(double eventPos[2],double worldPos[3],double tolerance,
                        vtkRenderer * renderer)
{
  double nearWorldPoint[4];
  double farWorldPoint[4];
  double tmp[3];

  tmp[0] = eventPos[0] + this->InteractionOffset[0];
  tmp[1] = eventPos[1] + this->InteractionOffset[1];
  tmp[2] = 0.0;  // near plane
  if(renderer == 0)
  {
    renderer = this->Renderer;
  }

  renderer->SetDisplayPoint(tmp);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(nearWorldPoint);

  tmp[2] = 1.0;  // far plane
  renderer->SetDisplayPoint(tmp);
  renderer->DisplayToWorld();
  renderer->GetWorldPoint(farWorldPoint);

  double normal[3];
  double origin[3];

  this->GetProjectionNormal( normal );
  this->GetProjectionOrigin( origin );

  vtkSmartPointer<vtkCellPicker> picker =
    vtkSmartPointer<vtkCellPicker>::New();

  picker->Pick(eventPos[0], eventPos[1], 0, renderer);

  vtkAssemblyPath *path = picker->GetPath();

  if(path == 0)
  {
   return 0;
  }
  double pickPos[3];
  picker->GetPickPosition(pickPos);
  if ( this->BoundingPlanes )
  {
    vtkPlane *p;
    this->BoundingPlanes->InitTraversal();
    while ( (p = this->BoundingPlanes->GetNextItem()) )
    {
      double v = p->EvaluateFunction( pickPos );
      if ( v < tolerance )
      {
        return 0;
      }
    }
  }

  worldPos[0] = pickPos[0];
  worldPos[1] = pickPos[1];
  worldPos[2] = pickPos[2];

  return 1;
}

//----------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::GetProjectionNormal( double normal[3] )
{
  switch ( this->ProjectionNormal )
  {
    case vtkConstrainedPointHandleRepresentation::XAxis:
      normal[0] = 1.0;
      normal[1] = 0.0;
      normal[2] = 0.0;
      break;
    case vtkConstrainedPointHandleRepresentation::YAxis:
      normal[0] = 0.0;
      normal[1] = 1.0;
      normal[2] = 0.0;
      break;
    case vtkConstrainedPointHandleRepresentation::ZAxis:
      normal[0] = 0.0;
      normal[1] = 0.0;
      normal[2] = 1.0;
      break;
    case vtkConstrainedPointHandleRepresentation::Oblique:
      this->ObliquePlane->GetNormal(normal);
      break;
  }
}

//----------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::GetProjectionOrigin( double origin[3] )
{
  switch ( this->ProjectionNormal )
  {
    case vtkConstrainedPointHandleRepresentation::XAxis:
      origin[0] = this->ProjectionPosition;
      origin[1] = 0.0;
      origin[2] = 0.0;
      break;
    case vtkConstrainedPointHandleRepresentation::YAxis:
      origin[0] = 0.0;
      origin[1] = this->ProjectionPosition;
      origin[2] = 0.0;
      break;
    case vtkConstrainedPointHandleRepresentation::ZAxis:
      origin[0] = 0.0;
      origin[1] = 0.0;
      origin[2] = this->ProjectionPosition;
      break;
    case vtkConstrainedPointHandleRepresentation::Oblique:
      this->ObliquePlane->GetOrigin(origin);
      break;
  }
}

//----------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::Scale(double eventPos[2])
{
  // Get the current scale factor
  double sf = this->Glypher->GetScaleFactor();

  // Compute the scale factor
  int *size = this->Renderer->GetSize();
  double dPos = static_cast<double>(eventPos[1]-this->LastEventPosition[1]);
  sf *= (1.0 + 2.0*(dPos / size[1])); //scale factor of 2.0 is arbitrary

  // Scale the handle
  this->Glypher->SetScaleFactor(sf);
}

//----------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::Highlight(int highlight)
{
  if ( highlight )
  {
    this->Actor->SetProperty(this->SelectedProperty);
  }
  else
  {
    this->Actor->SetProperty(this->ActiveProperty);
  }
}

//----------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::CreateDefaultProperties()
{
  this->Property = vtkProperty::New();
  this->Property->SetColor(1.0,1.0,1.0);
  this->Property->SetLineWidth(0.5);
  this->Property->SetPointSize(3);

  this->SelectedProperty = vtkProperty::New();
  this->SelectedProperty->SetColor(0.0,1.0,1.0);
  this->SelectedProperty->SetRepresentationToWireframe();
  this->SelectedProperty->SetAmbient(1.0);
  this->SelectedProperty->SetDiffuse(0.0);
  this->SelectedProperty->SetSpecular(0.0);
  this->SelectedProperty->SetLineWidth(2.0);

  this->ActiveProperty = vtkProperty::New();
  this->ActiveProperty->SetColor(0.0,1.0,0.0);
  this->ActiveProperty->SetRepresentationToWireframe();
  this->ActiveProperty->SetAmbient(1.0);
  this->ActiveProperty->SetDiffuse(0.0);
  this->ActiveProperty->SetSpecular(0.0);
  this->ActiveProperty->SetLineWidth(1.0);
}

//----------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::BuildRepresentation()
{
  double normal[3];
  this->GetProjectionNormal( normal );
  this->FocalData->GetPointData()->GetNormals()->SetTuple(0,normal);

  double *pos=this->WorldPosition->GetValue();
  this->FocalPoint->SetPoint(0, pos[0],pos[1],pos[2]);
  this->FocalPoint->Modified();
}

//----------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::GetActors(vtkPropCollection *pc)
{
  this->Actor->GetActors(pc);
}

//----------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::ReleaseGraphicsResources(vtkWindow *win)
{
  this->Actor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkConstrainedPointHandleRepresentation::RenderOverlay(vtkViewport *viewport)
{
  return this->Actor->RenderOverlay(viewport);
}

//----------------------------------------------------------------------
int vtkConstrainedPointHandleRepresentation::RenderOpaqueGeometry(vtkViewport *viewport)
{
  return this->Actor->RenderOpaqueGeometry(viewport);
}

//-----------------------------------------------------------------------------
int vtkConstrainedPointHandleRepresentation::RenderTranslucentPolygonalGeometry(vtkViewport *viewport)
{
  return this->Actor->RenderTranslucentPolygonalGeometry(viewport);
}

//-----------------------------------------------------------------------------
int vtkConstrainedPointHandleRepresentation::HasTranslucentPolygonalGeometry()
{
  return this->Actor->HasTranslucentPolygonalGeometry();
}

//----------------------------------------------------------------------
void vtkConstrainedPointHandleRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Projection Normal: ";
  if ( this->ProjectionNormal == vtkConstrainedPointHandleRepresentation::XAxis )
  {
    os << "XAxis\n";
  }
  else if ( this->ProjectionNormal == vtkConstrainedPointHandleRepresentation::YAxis )
  {
    os << "YAxis\n";
  }
  else if ( this->ProjectionNormal == vtkConstrainedPointHandleRepresentation::ZAxis )
  {
    os << "ZAxis\n";
  }
  else //if ( this->ProjectionNormal == vtkConstrainedPointHandleRepresentation::Oblique )
  {
    os << "Oblique\n";
  }

  os << indent << "Active Property: ";
  this->ActiveProperty->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Projection Position: " << this->ProjectionPosition << "\n";

  os << indent << "Property: ";
  this->Property->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Selected Property: ";
  this->SelectedProperty->PrintSelf(os,indent.GetNextIndent());

  os << indent << "Oblique Plane: ";
  if ( this->ObliquePlane )
  {
    this->ObliquePlane->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }

  os << indent << "Bounding Planes: ";
  if ( this->BoundingPlanes )
  {
    this->BoundingPlanes->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << "(none)\n";
  }
}
