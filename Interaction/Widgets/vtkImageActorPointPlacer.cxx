/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageActorPointPlacer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageActorPointPlacer.h"
#include "vtkObjectFactory.h"
#include "vtkBoundedPlanePointPlacer.h"
#include "vtkPlane.h"
#include "vtkRenderer.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"

vtkStandardNewMacro(vtkImageActorPointPlacer);

vtkCxxSetObjectMacro(vtkImageActorPointPlacer, ImageActor, vtkImageActor);

//----------------------------------------------------------------------
vtkImageActorPointPlacer::vtkImageActorPointPlacer()
{
  this->Placer = vtkBoundedPlanePointPlacer::New();
  this->ImageActor = NULL;
  this->SavedBounds[0] = 0.0;
  this->SavedBounds[1] = 0.0;
  this->SavedBounds[2] = 0.0;
  this->SavedBounds[3] = 0.0;
  this->SavedBounds[4] = 0.0;
  this->SavedBounds[5] = 0.0;
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_DOUBLE_MAX;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = VTK_DOUBLE_MIN;
}

//----------------------------------------------------------------------
vtkImageActorPointPlacer::~vtkImageActorPointPlacer()
{
  this->Placer->Delete();
  this->SetImageActor(NULL);
}


//----------------------------------------------------------------------
int vtkImageActorPointPlacer::ComputeWorldPosition( vtkRenderer *ren,
                                                    double  displayPos[2],
                                                    double *refWorldPos,
                                                    double  worldPos[3],
                                                    double  worldOrient[9] )
{
  if ( !this->UpdateInternalState() )
  {
    return 0;
  }

  return this->Placer->ComputeWorldPosition( ren, displayPos,
                                             refWorldPos, worldPos,
                                             worldOrient );
}

//----------------------------------------------------------------------
int vtkImageActorPointPlacer::ComputeWorldPosition( vtkRenderer *ren,
                                                    double displayPos[2],
                                                    double worldPos[3],
                                                    double worldOrient[9] )
{
  if ( !this->UpdateInternalState() )
  {
    return 0;
  }

  return this->Placer->ComputeWorldPosition( ren, displayPos, worldPos, worldOrient );
}

//----------------------------------------------------------------------
int vtkImageActorPointPlacer::ValidateWorldPosition( double worldPos[3],
                                                     double *worldOrient )
{
  if ( !this->UpdateInternalState() )
  {
    return 0;
  }

  return this->Placer->ValidateWorldPosition( worldPos, worldOrient );
}

//----------------------------------------------------------------------
int vtkImageActorPointPlacer::ValidateWorldPosition( double worldPos[3] )
{
  if ( !this->UpdateInternalState() )
  {
    return 0;
  }

  return this->Placer->ValidateWorldPosition( worldPos );
}

//----------------------------------------------------------------------
int vtkImageActorPointPlacer::UpdateWorldPosition( vtkRenderer *ren,
                                                   double worldPos[3],
                                                   double worldOrient[9] )
{
  if ( !this->UpdateInternalState() )
  {
    return 0;
  }

  return this->Placer->UpdateWorldPosition( ren,
                                            worldPos,
                                            worldOrient );
}

//----------------------------------------------------------------------
int vtkImageActorPointPlacer::UpdateInternalState()
{
  if ( !this->ImageActor )
  {
    return 0;
  }

  vtkImageData *input;
  input = this->ImageActor->GetInput();
  if ( !input )
  {
    return 0;
  }

  double spacing[3];
  input->GetSpacing(spacing);

  double origin[3];
  input->GetOrigin(origin);

  double bounds[6];
  this->ImageActor->GetBounds(bounds);
  if (this->Bounds[0] != VTK_DOUBLE_MAX)
  {
    bounds[0] = (bounds[0] < this->Bounds[0]) ? this->Bounds[0] : bounds[0];
    bounds[1] = (bounds[1] > this->Bounds[1]) ? this->Bounds[1] : bounds[1];
    bounds[2] = (bounds[2] < this->Bounds[2]) ? this->Bounds[2] : bounds[2];
    bounds[3] = (bounds[3] > this->Bounds[3]) ? this->Bounds[3] : bounds[3];
    bounds[4] = (bounds[4] < this->Bounds[4]) ? this->Bounds[4] : bounds[4];
    bounds[5] = (bounds[5] > this->Bounds[5]) ? this->Bounds[5] : bounds[5];
  }

  int displayExtent[6];
  this->ImageActor->GetDisplayExtent(displayExtent);

  int axis;
  double position;
  if ( displayExtent[0] == displayExtent[1] )
  {
    axis = vtkBoundedPlanePointPlacer::XAxis;
    position = origin[0] + displayExtent[0]*spacing[0];
  }
  else if ( displayExtent[2] == displayExtent[3] )
  {
    axis = vtkBoundedPlanePointPlacer::YAxis;
    position = origin[1] + displayExtent[2]*spacing[1];
  }
  else if ( displayExtent[4] == displayExtent[5] )
  {
    axis = vtkBoundedPlanePointPlacer::ZAxis;
    position = origin[2] + displayExtent[4]*spacing[2];
  }
  else
  {
    vtkErrorMacro("Incorrect display extent in Image Actor");
    return 0;
  }

  if ( axis != this->Placer->GetProjectionNormal() ||
       position != this->Placer->GetProjectionPosition() ||
       bounds[0] != this->SavedBounds[0] ||
       bounds[1] != this->SavedBounds[1] ||
       bounds[2] != this->SavedBounds[2] ||
       bounds[3] != this->SavedBounds[3] ||
       bounds[4] != this->SavedBounds[4] ||
       bounds[5] != this->SavedBounds[5] )
  {
    this->SavedBounds[0] = bounds[0];
    this->SavedBounds[1] = bounds[1];
    this->SavedBounds[2] = bounds[2];
    this->SavedBounds[3] = bounds[3];
    this->SavedBounds[4] = bounds[4];
    this->SavedBounds[5] = bounds[5];

    this->Placer->SetProjectionNormal(axis);
    this->Placer->SetProjectionPosition(position);

    this->Placer->RemoveAllBoundingPlanes();

    vtkPlane *plane;

    if ( axis != vtkBoundedPlanePointPlacer::XAxis )
    {
      plane = vtkPlane::New();
      plane->SetOrigin( bounds[0], bounds[2], bounds[4] );
      plane->SetNormal( 1.0, 0.0, 0.0 );
      this->Placer->AddBoundingPlane( plane );
      plane->Delete();

      plane = vtkPlane::New();
      plane->SetOrigin( bounds[1], bounds[3], bounds[5] );
      plane->SetNormal( -1.0, 0.0, 0.0 );
      this->Placer->AddBoundingPlane( plane );
      plane->Delete();
    }

    if ( axis != vtkBoundedPlanePointPlacer::YAxis )
    {
      plane = vtkPlane::New();
      plane->SetOrigin( bounds[0], bounds[2], bounds[4] );
      plane->SetNormal( 0.0, 1.0, 0.0 );
      this->Placer->AddBoundingPlane( plane );
      plane->Delete();

      plane = vtkPlane::New();
      plane->SetOrigin( bounds[1], bounds[3], bounds[5] );
      plane->SetNormal( 0.0, -1.0, 0.0 );
      this->Placer->AddBoundingPlane( plane );
      plane->Delete();
    }

    if ( axis != vtkBoundedPlanePointPlacer::ZAxis )
    {
      plane = vtkPlane::New();
      plane->SetOrigin( bounds[0], bounds[2], bounds[4] );
      plane->SetNormal( 0.0, 0.0, 1.0 );
      this->Placer->AddBoundingPlane( plane );
      plane->Delete();

      plane = vtkPlane::New();
      plane->SetOrigin( bounds[1], bounds[3], bounds[5] );
      plane->SetNormal( 0.0, 0.0, -1.0 );
      this->Placer->AddBoundingPlane( plane );
      plane->Delete();
    }

    this->Modified();
  }

  return 1;
}

//----------------------------------------------------------------------
void vtkImageActorPointPlacer::SetWorldTolerance( double tol )
{
  if (this->WorldTolerance !=
      (tol<0.0?0.0:(tol>VTK_DOUBLE_MAX?VTK_DOUBLE_MAX:tol)))
  {
    this->WorldTolerance =
      (tol<0.0?0.0:(tol>VTK_DOUBLE_MAX?VTK_DOUBLE_MAX:tol));
    this->Placer->SetWorldTolerance(tol);
    this->Modified();
  }
}

//----------------------------------------------------------------------
void vtkImageActorPointPlacer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  double *bounds = this->GetBounds();
  if ( bounds != NULL )
  {
    os << indent << "Bounds: \n";
    os << indent << "  Xmin,Xmax: ("
       << this->Bounds[0] << ", " << this->Bounds[1] << ")\n";
    os << indent << "  Ymin,Ymax: ("
       << this->Bounds[2] << ", " << this->Bounds[3] << ")\n";
    os << indent << "  Zmin,Zmax: ("
       << this->Bounds[4] << ", " << this->Bounds[5] << ")\n";
  }
  else
  {
    os << indent << "Bounds: (not defined)\n";
  }

  os << indent << "Image Actor: " << this->ImageActor << "\n";
}

