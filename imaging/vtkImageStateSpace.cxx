/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStateSpace.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <math.h>
#include "vtkImageStateSpace.h"
#include "vtkImageDraw.h"


// State is in pixel units.

//----------------------------------------------------------------------------
vtkImageStateSpace::vtkImageStateSpace()
{
  this->Region = NULL;
  this->NumberOfDimensions = 3;
}


//----------------------------------------------------------------------------
vtkImageStateSpace::~vtkImageStateSpace()
{
  if (this->Region)
    {
    this->Region->Delete();
    }
}


//----------------------------------------------------------------------------
// Description:
// This method removes redundant locations in state space.
void vtkImageStateSpace::Wrap(float *state)
{
  int idx;

  // Collapse extra dimensions
  for (idx = this->NumberOfDimensions; idx < 3; ++idx)
    {
    state[idx] = 0.0;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method returns 0.0 if a state is out of the image bounds.
// Values are middle of pixels (rounded)
float vtkImageStateSpace::BoundsTest(float *state)
{
  int idx;
  int round;
  int *extent;
  
  // Make sure the state is in the bounds of the strucutred point set.
  extent = this->Region->GetExtent();
  for (idx = 0; idx < this->NumberOfDimensions; ++idx)
    {
    round = (int)(floor(state[idx] + 0.5));
    if (round < extent[idx*2] || round > extent[idx*2+1])
      {
      // Out of bounds
      return 0.0;
      }
    }
  return 1.0;
}

//----------------------------------------------------------------------------
// This method computes max distance between two points.
// Euclidean distance.
float vtkImageStateSpace::Distance(float *p0, float *p1)
{
  int idx;
  float sum = 0.0;
  float temp;
  float *aspectRatio = this->Region->GetAspectRatio();
  
  for (idx = 0; idx < this->NumberOfDimensions; ++idx)
    {
    temp = (p0[idx] - p1[idx]) * aspectRatio[idx];
    sum += temp * temp;
    }
  
  return sqrt(sum);
}

//----------------------------------------------------------------------------
// This method determines collision space from free space
int vtkImageStateSpace::Collide(float *state)
{
  void *ptr;
  int *extent;
  int idx, round[3];

  // Round values
  extent = this->Region->GetExtent();
  for (idx = 0; idx < this->NumberOfDimensions; ++idx)
    {
    round[idx] = (int)(floor(state[idx] + 0.5));
    if ((round[idx] < extent[idx*2]) || (round[idx] > extent[idx*2+1]))
      {
      return 1;
      }
    }
  
  ptr = this->Region->GetScalarPointer(this->NumberOfDimensions, round);

  switch (this->Region->GetScalarType())
    {
    case VTK_FLOAT:
      return *((float *)ptr) > this->Threshold;
    case VTK_INT:
      return (float)(*((int *)ptr)) > this->Threshold;
    case VTK_SHORT:
      return (float)(*((short *)ptr)) > this->Threshold;
    case VTK_UNSIGNED_SHORT:
      return (float)(*((unsigned short *)ptr)) > this->Threshold;
    case VTK_UNSIGNED_CHAR:
      return (float)(*((unsigned char *)ptr)) > this->Threshold;
    default:
      vtkErrorMacro(<< "Collide: Unknown data type");
      return 0;
    }
}






//----------------------------------------------------------------------------
// This method returns the state mid-way between s0 and s1.
void vtkImageStateSpace::GetMiddleState(float *s0, float *s1, float *middle)
{
  int idx;
  
  for (idx = 0; idx < 3; ++idx)
    {
    middle[idx] = (s0[idx] + s1[idx]) / 2.0;
    }
}




//----------------------------------------------------------------------------
// This method finds a child of a state.  This is a new state a specified
// distance along an axis from the first state.
void vtkImageStateSpace::GetChildState(float *state, int axis, float distance, 
				       float *child)
{
  int idx;

  // First copy the state.
  for (idx = 0; idx < 3; ++idx)
    {
    child[idx] = state[idx];
    }
  
  // add distance along one direction.
  child[axis] += distance;
}

  

//----------------------------------------------------------------------------
// This method draws a path
void vtkImageStateSpace::DrawPath(vtkClaw *planner, float value)
{
  int idx;
  int numberOfStates;
  float state1[3], state2[3];
  vtkImageDraw *canvas;
  
  if ( !planner || !this->Region)
    {
    return;
    }

  canvas = new vtkImageDraw;
  canvas->SetExtent(VTK_IMAGE_DIMENSIONS, this->Region->GetExtent());
  this->Region->UpdateRegion(canvas);
  canvas->SetDrawValue(value);
  
  numberOfStates = planner->GetPathLength();
  
  for(idx = 1; idx < numberOfStates; ++idx)
    {
    planner->GetPathState(idx - 1, state1);
    planner->GetPathState(idx, state2);
    canvas->DrawSegment3D(state1, state2);
    }
  
  canvas->Delete();
}





//----------------------------------------------------------------------------
// This returns all the spheres and the path in the form of a PolyData object.
vtkPolyData *vtkImageStateSpace::GetPathPolyData(vtkClaw *planner)
{
  SphereList *sphereList;
  
  vtkPolyData *polyData;
  vtkCellArray *lines;
  vtkFloatPoints *points;
  vtkFloatScalars *scalars;
  float *aspectRatio;
  float x[3];
  int idx;
  int ptIds[2];

  
  if ( ! planner || ! this->Region)
    {
    vtkErrorMacro(<< "Need a planner and a region.");
    return NULL;
    }

  aspectRatio = this->Region->GetAspectRatio();
  polyData = new vtkPolyData;
  points = new vtkFloatPoints;
  scalars = new vtkFloatScalars;
  lines = new vtkCellArray();
  
  sphereList = planner->GetPath();
  if ( ! sphereList)
    {
    // delete stuff ....
    vtkErrorMacro(<< "No Path");
    return NULL;
    }
  
  // First point
  for (idx = 0; idx < 3; ++idx)
    {
    x[idx] = sphereList->Item->Center[idx] * aspectRatio[idx];
    }
  ptIds[1] = points->InsertNextPoint(x);
  scalars->InsertNextScalar(sphereList->Item->Radius);
  lines->InsertNextCell(2,ptIds);    
  sphereList = sphereList->Next;
  // The rest of the points (and lines
  while (sphereList)
    {
    ptIds[0] = ptIds[1];
    for (idx = 0; idx < 3; ++idx)
      {
      x[idx] = sphereList->Item->Center[idx] * aspectRatio[idx];
      }
    ptIds[1] = points->InsertNextPoint(x);
    scalars->InsertNextScalar(sphereList->Item->Radius);
    lines->InsertNextCell(2,ptIds);    
    sphereList = sphereList->Next;
    }
  
  // Construct the polyData
  polyData->SetPoints(points);
  points->Delete();

  polyData->SetLines(lines);
  lines->Delete();

  polyData->GetPointData()->SetScalars(scalars);
  scalars->Delete();

  polyData->Squeeze();
  return polyData;
}

  
//----------------------------------------------------------------------------
// This returns all the spheres in the form of polydata
vtkPolyData *vtkImageStateSpace::GetSpherePolyData(vtkClaw *planner)
{
  SphereList *sphereList = planner->GetSpheres();
  vtkPolyData *polyData;
  vtkFloatPoints *points;
  vtkFloatScalars *scalars;
  float *aspectRatio;
  float x[3];
  int idx;

  
  if ( ! planner || ! this->Region)
    {
    vtkErrorMacro(<< "Need a planner and a region.");
    return NULL;
    }

  // Set up the points for all the spheres.
  aspectRatio = this->Region->GetAspectRatio();
  polyData = new vtkPolyData;
  points = new vtkFloatPoints;
  scalars = new vtkFloatScalars;
  while (sphereList)
    {
    for (idx = 0; idx < 3; ++idx)
      {
      x[idx] = sphereList->Item->Center[idx] * aspectRatio[idx];
      }
    points->InsertNextPoint(x);
    scalars->InsertNextScalar(sphereList->Item->Radius);
    sphereList = sphereList->Next;
    }
  
  // Construct the polyData
  polyData->SetPoints(points);
  points->Delete();

  polyData->GetPointData()->SetScalars(scalars);
  scalars->Delete();

  polyData->Squeeze();
  return polyData;
}

  
  
//----------------------------------------------------------------------------
// This returns all the collisions as polydata
vtkPolyData *vtkImageStateSpace::GetCollisionPolyData(vtkClaw *planner)
{
  SphereList *sphereList = planner->GetCollisions();
  vtkPolyData *polyData;
  vtkFloatPoints *points;
  float *aspectRatio;
  float x[3];
  int idx;
  
  if ( ! planner || ! this->Region)
    {
    vtkErrorMacro(<< "Need a planner and a region.");
    return NULL;
    }

  // Set up the points for all the spheres.
  aspectRatio = this->Region->GetAspectRatio();
  polyData = new vtkPolyData;
  points = new vtkFloatPoints;
  while (sphereList)
    {
    for (idx = 0; idx < 3; ++idx)
      {
      x[idx] = sphereList->Item->Center[idx] * aspectRatio[idx];
      }
    points->InsertNextPoint(x);
    sphereList = sphereList->Next;
    }
  
  // Construct the polyData
  polyData->SetPoints(points);
  points->Delete();

  polyData->Squeeze();
  
  return polyData;
}

  
  


    





