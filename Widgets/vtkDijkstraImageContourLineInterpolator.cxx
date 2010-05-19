/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDijkstraImageContourLineInterpolator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDijkstraImageContourLineInterpolator.h"

#include "vtkCellArray.h"
#include "vtkCellLocator.h"
#include "vtkContourRepresentation.h"
#include "vtkDijkstraImageGeodesicPath.h"
#include "vtkImageActor.h"
#include "vtkImageActorPointPlacer.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkDijkstraImageContourLineInterpolator);

//----------------------------------------------------------------------
vtkDijkstraImageContourLineInterpolator
::vtkDijkstraImageContourLineInterpolator()
{
  this->DijkstraImageGeodesicPath = vtkDijkstraImageGeodesicPath::New();
  this->CostImage = NULL;
}

//----------------------------------------------------------------------
vtkDijkstraImageContourLineInterpolator
::~vtkDijkstraImageContourLineInterpolator()
{
  this->DijkstraImageGeodesicPath->Delete();
  this->CostImage = NULL;
}

//----------------------------------------------------------------------------
void vtkDijkstraImageContourLineInterpolator::SetCostImage( vtkImageData *arg )
{
  if ( this->CostImage == arg )
    {
    return;
    }

  this->CostImage = arg;
  if ( this->CostImage )
    {
    this->DijkstraImageGeodesicPath->SetInput( this->CostImage );
    }
}

//----------------------------------------------------------------------
int vtkDijkstraImageContourLineInterpolator::InterpolateLine( 
  vtkRenderer* vtkNotUsed(ren), vtkContourRepresentation *rep,
  int idx1, int idx2 )
{
  // if the user didn't set the image, try to get it from the actor
  if ( !this->CostImage )
    {

    vtkImageActorPointPlacer *placer =
      vtkImageActorPointPlacer::SafeDownCast(rep->GetPointPlacer());

    if ( !placer ) 
      {
      return 1;
      }
      
    vtkImageActor* actor = placer->GetImageActor();
    if ( !actor || !(this->CostImage = actor->GetInput()) )
      {
      return 1;
      }
    this->DijkstraImageGeodesicPath->SetInput( this->CostImage );
    }

  double p1[3], p2[3];
  rep->GetNthNodeWorldPosition( idx1, p1 );
  rep->GetNthNodeWorldPosition( idx2, p2 );

  vtkIdType beginVertId = this->CostImage->FindPoint( p1 );
  vtkIdType endVertId = this->CostImage->FindPoint( p2 );

  // Could not find the starting and ending cells. We can't interpolate.
  if ( beginVertId == -1 || endVertId == -1 ) 
    {
    return 0;
    }

  int nnodes = rep->GetNumberOfNodes();

  if ( this->DijkstraImageGeodesicPath->GetRepelPathFromVertices() && nnodes > 2 )
    {
    vtkPoints* verts = vtkPoints::New();
    double pt[3];
    for( int i = 0; i < nnodes; ++i )
      {
      if( i == idx1 ) continue;

      for( int j = 0; j < rep->GetNumberOfIntermediatePoints( i ); ++j )
        {
          rep->GetIntermediatePointWorldPosition( i, j, pt );
          verts->InsertNextPoint( pt );
        }
      }
    this->DijkstraImageGeodesicPath->SetRepelVertices( verts );
    verts->Delete();
    }
  else
    {
    this->DijkstraImageGeodesicPath->SetRepelVertices( NULL );
    }

  this->DijkstraImageGeodesicPath->SetStartVertex( endVertId );
  this->DijkstraImageGeodesicPath->SetEndVertex( beginVertId );
  this->DijkstraImageGeodesicPath->Update();

  vtkPolyData *pd = this->DijkstraImageGeodesicPath->GetOutput();
  
  vtkIdType npts = 0, *pts = NULL;
  pd->GetLines()->InitTraversal();
  pd->GetLines()->GetNextCell( npts, pts );

  for ( int i = 0; i < npts; ++i )
    {
    rep->AddIntermediatePointWorldPosition( idx1, pd->GetPoint( pts[i] )  );
    }

  return 1;
}

//----------------------------------------------------------------------
void vtkDijkstraImageContourLineInterpolator::PrintSelf(
                              ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "DijkstraImageGeodesicPath: " << this->DijkstraImageGeodesicPath << endl;
  os << indent << "CostImage: " << this->GetCostImage() << endl;
}
