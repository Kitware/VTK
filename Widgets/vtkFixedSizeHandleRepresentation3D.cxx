/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFixedSizeHandleRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFixedSizeHandleRepresentation3D.h"

#include "vtkObjectFactory.h"
#include "vtkSphereSource.h"
#include "vtkRenderer.h"
#include "vtkMath.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTransform.h"
#include "vtkCamera.h"
#include "vtkInteractorObserver.h"

vtkStandardNewMacro(vtkFixedSizeHandleRepresentation3D);

//----------------------------------------------------------------------
vtkFixedSizeHandleRepresentation3D::vtkFixedSizeHandleRepresentation3D()
{
  // Instantiate a handle template shape as a sphere
  this->SphereSource = vtkSphereSource::New();
  this->SphereSource->SetThetaResolution(20);
  this->SphereSource->SetPhiResolution(20);
  this->SphereSource->SetRadius(1.2);
  this->SphereSource->Update();
  this->SetHandle(this->SphereSource->GetOutput());

  this->HandleSizeInPixels = 10.0;
  this->HandleSizeToleranceInPixels = 0.5;
}

//----------------------------------------------------------------------
vtkFixedSizeHandleRepresentation3D::~vtkFixedSizeHandleRepresentation3D()
{
  this->SphereSource->Delete();
}

//-----------------------------------------------------------------------------
void vtkFixedSizeHandleRepresentation3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "HandleSizeInPixels: " << this->HandleSizeInPixels << endl;
  os << indent << "HandleSizeToleranceInPixels: "
     << this->HandleSizeToleranceInPixels << endl;
  os << indent << "SphereSource: " << this->SphereSource << endl;
  if (this->SphereSource)
    {
    this->SphereSource->PrintSelf(os, indent.GetNextIndent());
    }
}

//---------------------------------------------------------------------------
// Convert a given point from world to display coords.
void vtkFixedSizeHandleRepresentation3D
::WorldToDisplay( double w[4], double d[4] )
{
  vtkRenderer* viewport = this->GetRenderer();
  viewport->SetWorldPoint(w);
  viewport->WorldToDisplay();
  viewport->GetDisplayPoint(d);
}

//---------------------------------------------------------------------------
// Convert a given point from display to world coords.
void vtkFixedSizeHandleRepresentation3D
::DisplayToWorld( double d[4], double w[4] )
{
  d[3] = 1.0;
  vtkRenderer* viewport = this->GetRenderer();
  viewport->SetDisplayPoint(d);
  viewport->DisplayToWorld();
  viewport->GetWorldPoint(w);
}

//---------------------------------------------------------------------------
void vtkFixedSizeHandleRepresentation3D::BuildRepresentation()
{
  // Compute display position from world position of center. Notation here
  // is such that
  //    w_ represents world coords,
  //    d_ represents display/pixel coords.

  double w_c[4], d_c[4];

  // Get the current world position and convert it to display coords
  this->GetWorldPosition(w_c);

  w_c[3] = 1.0;

  this->WorldToDisplay(w_c, d_c);

  // Compute current display size of the handle. To do this, we convert
  // a point laying at (<current_center> + {radius \times <view_up_vector>})
  // to display coords and see how far it is from the current display position
  // of the handle.

  double v_u[4], w_p[4], d_p[4];

  this->GetRenderer()->GetActiveCamera()->GetViewUp(v_u);
  const double currRadius = this->SphereSource->GetRadius();
  w_p[0] = w_c[0] + currRadius * v_u[0];
  w_p[1] = w_c[1] + currRadius * v_u[1];
  w_p[2] = w_c[2] + currRadius * v_u[2];
  w_p[3] = 1.0;

  this->WorldToDisplay( w_p, d_p );

  // Size in Pixels
  const double currentSizeInPixels =
    sqrt(vtkMath::Distance2BetweenPoints(d_p ,d_c));
  const double displayRadius = this->HandleSizeInPixels / 2.0;
  const double radiusTolerance = this->HandleSizeToleranceInPixels / 2.0;

  // The current size in pixels is not within the tolerance of the desired
  // size. Recompute the required 3D sphere size to achieve this.
  if(fabs(currentSizeInPixels - displayRadius) > radiusTolerance)
    {
    // Adjust size
    // Computing radius in physical units which will result in a
    // displayRadius in pixels
    double d_x[4], w_x[4];

    d_x[0] = d_c[0] + displayRadius;
    d_x[1] = d_c[1];
    d_x[2] = d_c[2];

    // Display to world
    this->DisplayToWorld(d_x, w_x);

    // computed radius in world coords
    const double w_r = sqrt(vtkMath::Distance2BetweenPoints(w_x ,w_c));

    // Generate a handle with a radius of w_r in physical units
    this->SphereSource->SetRadius(w_r);
    this->SphereSource->Update();

    this->SetHandle(this->SphereSource->GetOutput());
    }
}
