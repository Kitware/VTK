/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlanes.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPlanes.h"
#include "vtkPlane.h"
#include "vtkObjectFactory.h"
#include "vtkFloatArray.h"

#include <math.h>

vtkCxxRevisionMacro(vtkPlanes, "1.7");
vtkStandardNewMacro(vtkPlanes);

vtkPlanes::vtkPlanes()
{
  int i;

  this->Points = NULL;
  this->Normals = NULL;
  this->Plane = vtkPlane::New();

  for (i=0; i<24; i++)
    {
    this->Planes[i] = 0.0;
    }
  for (i=0; i<6; i++)
    {
    this->Bounds[i] = 0.0;
    }
}

vtkPlanes::~vtkPlanes()
{
  if ( this->Points )
    {
    this->Points->UnRegister(this);
    }
  if ( this->Normals )
    {
    this->Normals->UnRegister(this);
    }
  this->Plane->Delete();
}

void vtkPlanes::SetNormals(vtkDataArray* normals)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting Normals to " << normals ); 

  if (normals->GetNumberOfComponents() != 3)
    {
    vtkWarningMacro("This array does not have 3 components. Ignoring normals.");
    return;
    }

  if (this->Normals != normals) 
    { 
    if (this->Normals != NULL) { this->Normals->UnRegister(this); }
    this->Normals = normals; 
    if (this->Normals != NULL) { this->Normals->Register(this); } 
    this->Modified(); 
    } 
}

// Evaluate plane equations. Return smallest absolute value.
float vtkPlanes::EvaluateFunction(float x[3])
{
  int numPlanes, i;
  float val, maxVal;

  if ( !this->Points || ! this->Normals )
    {
    vtkErrorMacro(<<"Please define points and/or normals!");
    return VTK_LARGE_FLOAT;
    }

  if ( (numPlanes=this->Points->GetNumberOfPoints()) != this->Normals->GetNumberOfTuples() )
    {
    vtkErrorMacro(<<"Number of normals/points inconsistent!");
    return VTK_LARGE_FLOAT;
    }

  for (maxVal=-VTK_LARGE_FLOAT, i=0; i < numPlanes; i++)
    {
    val = this->Plane->Evaluate(this->Normals->GetTuple(i),
                         this->Points->GetPoint(i), x);
    if (val > maxVal )
      {
      maxVal = val;
      }
    }

  return maxVal;
}

// Evaluate planes gradient.
void vtkPlanes::EvaluateGradient(float x[3], float n[3])
{
  int numPlanes, i;
  float val, maxVal, *nTemp;

  if ( !this->Points || ! this->Normals )
    {
    vtkErrorMacro(<<"Please define points and/or normals!");
    return;
    }

  if ( (numPlanes=this->Points->GetNumberOfPoints()) != this->Normals->GetNumberOfTuples() )
    {
    vtkErrorMacro(<<"Number of normals/points inconsistent!");
    return;
    }

  for (maxVal=-VTK_LARGE_FLOAT, i=0; i < numPlanes; i++)
    {
    nTemp = this->Normals->GetTuple(i);
    val = this->Plane->Evaluate(nTemp,this->Points->GetPoint(i), x);
    if ( val > maxVal )
      {
      maxVal = val;
      n[0] = nTemp[0];
      n[1] = nTemp[1];
      n[2] = nTemp[2];
      }
    }
}

void vtkPlanes::SetFrustumPlanes(float planes[24])
{
  int i;
  float *plane, n[3], x[3];

  for (i=0; i<24; i++)
    {
    if ( this->Planes[i] != planes[i] )
      {
      break;
      }
    }
  if ( i >= 24 )
    {
    return; //same as before don't modify
    }

  // okay, need to allocate stuff
  this->Modified();
  vtkPoints *pts = vtkPoints::New();
  vtkFloatArray *normals = vtkFloatArray::New();

  pts->SetNumberOfPoints(6);
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(6);
  this->SetPoints(pts);
  this->SetNormals(normals);

  for (i=0; i<6; i++)
    {
    plane = planes + 4*i;
    n[0] = -plane[0];
    n[1] = -plane[1];
    n[2] = -plane[2];
    x[0] = x[1] = x[2] = 0.0;
    if ( n[0] != 0.0 )
      {
      x[0] = plane[3] / n[0];
      }
    else if ( n[1] != 0.0 )
      {
      x[1] = plane[3] / n[1];
      }
    else
      {
      x[2] = plane[3] / n[2];
      }
    pts->SetPoint(i,x);
    normals->SetTuple(i,n);
    }
  
  pts->Delete(); //ok reference counting
  normals->Delete();
}

void vtkPlanes::SetBounds(float bounds[6])
{
  int i;
  float n[3], x[3];

  for (i=0; i<6; i++)
    {
    if ( this->Bounds[i] != bounds[i] )
      {
      break;
      }
    }
  if ( i >= 6 )
    {
    return; //same as before don't modify
    }

  // okay, need to allocate stuff
  this->Modified();
  vtkPoints *pts = vtkPoints::New();
  vtkFloatArray *normals = vtkFloatArray::New();

  pts->SetNumberOfPoints(6);
  normals->SetNumberOfComponents(3);
  normals->SetNumberOfTuples(6);
  this->SetPoints(pts);
  this->SetNormals(normals);

  // Define the six planes
  // The x planes
  n[0] = -1.0;
  n[1] = 0.0;
  n[2] = 0.0;
  x[0] = this->Bounds[0] = bounds[0];
  x[1] = 0.0;
  x[2] = 0.0;
  pts->SetPoint(0,x);
  normals->SetTuple(0,n);

  n[0] = 1.0;
  x[0] = this->Bounds[1] = bounds[1];
  pts->SetPoint(1,x);
  normals->SetTuple(1,n);
  
  // The y planes
  n[0] = 0.0;
  n[1] = -1.0;
  n[2] = 0.0;
  x[0] = 0.0;
  x[1] = this->Bounds[2] = bounds[2];
  x[2] = 0.0;
  pts->SetPoint(2,x);
  normals->SetTuple(2,n);

  n[1] = 1.0;
  x[1] = this->Bounds[3] = bounds[3];
  pts->SetPoint(3,x);
  normals->SetTuple(3,n);
  
  // The z planes
  n[0] = 0.0;
  n[1] = 0.0;
  n[2] = -1.0;
  x[0] = 0.0;
  x[1] = 0.0;
  x[2] = this->Bounds[4] = bounds[4];
  pts->SetPoint(4,x);
  normals->SetTuple(4,n);

  n[2] = 1.0;
  x[2] = this->Bounds[5] = bounds[5];
  pts->SetPoint(5,x);
  normals->SetTuple(5,n);
  
  pts->Delete(); //ok reference counting
  normals->Delete();
}

void vtkPlanes::SetBounds(float xmin, float xmax, float ymin, float ymax,
                          float zmin, float zmax)
{
  float bounds[6];
  bounds[0] = xmin;
  bounds[1] = xmax;
  bounds[2] = ymin;
  bounds[3] = ymax;
  bounds[4] = zmin;
  bounds[5] = zmax;

  this->SetBounds(bounds);
}

int vtkPlanes::GetNumberOfPlanes()
{
  if ( this->Points && this->Normals )
    {
    int npts = this->Points->GetNumberOfPoints();
    int nnormals = this->Normals->GetNumberOfTuples();
    return ( npts <= nnormals ? npts : nnormals );
    }
  else
    {
    return 0;
    }
}
  
vtkPlane *vtkPlanes::GetPlane(int i)
{
  if ( i >= 0 && i < this->GetNumberOfPlanes() )
    {
    vtkPlane *plane = vtkPlane::New();
    plane->SetNormal(this->Normals->GetTuple(i));
    plane->SetOrigin(this->Points->GetPoint(i));
    return plane;
    }
  else
    {
    return NULL;
    }
}

void vtkPlanes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  int numPlanes;

  if ( this->Points && (numPlanes=this->Points->GetNumberOfPoints()) > 0 )
    {
    os << indent << "Number of Planes: " << numPlanes << "\n";
    }
  else
    {
    os << indent << "No Planes Defined.\n";
    }

  if ( this->Normals )
    {
    os << indent << "Normals: " << this->Normals << "\n";
    }
  else
    {
    os << indent << "Normals: (none)\n";
    }
}
