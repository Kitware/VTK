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
#include <stdlib.h>
#include "vtkImageStateSpace.h"
#include "vtkImageDraw.h"
#include "vtkImageWrapPad.h"


// State is in pixel units.

//----------------------------------------------------------------------------
vtkImageStateSpace::vtkImageStateSpace()
{
  this->Planner = NULL;
  this->Region = NULL;
  // Canvas and viewer is for panning in images feedback
  this->Canvas = NULL;
  this->Viewer = NULL;
  // Default is volumes.
  this->StateDimensionality = 3;
  this->CollisionValue = 0;
}


//----------------------------------------------------------------------------
vtkImageStateSpace::~vtkImageStateSpace()
{
  if (this->Region)
    {
    this->Region->Delete();
    }
  if (this->Canvas)
    {
    this->Canvas->Delete();
    }
  if (this->Viewer)
    {
    this->Viewer->Delete();
    }
}

//----------------------------------------------------------------------------
int vtkImageStateSpace::GetDegreesOfFreedom()
{
  return this->StateDimensionality;
};


//----------------------------------------------------------------------------
// Description:
// This method removes redundant locations in state space.
void vtkImageStateSpace::Wrap(float *state)
{
  // Default is that the state space ends at the borders.
  state = state;
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
  for (idx = 0; idx < this->StateDimensionality; ++idx)
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
  
  for (idx = 0; idx < this->StateDimensionality; ++idx)
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
  for (idx = 0; idx < this->StateDimensionality; ++idx)
    {
    round[idx] = (int)(floor(state[idx] + 0.5));
    if ((round[idx] < extent[idx*2]) || (round[idx] > extent[idx*2+1]))
      {
      // Out of the region means collision.
      return 1;
      }
    }
  
  ptr = this->Region->GetScalarPointer(this->StateDimensionality, round);

  switch (this->Region->GetScalarType())
    {
    case VTK_FLOAT:
      return *((int *)ptr) == this->CollisionValue;
    case VTK_INT:
      return (int)(*((int *)ptr)) == this->CollisionValue;
    case VTK_SHORT:
      return (int)(*((short *)ptr)) == this->CollisionValue;
    case VTK_UNSIGNED_SHORT:
      return (int)(*((unsigned short *)ptr)) == this->CollisionValue;
    case VTK_UNSIGNED_CHAR:
      return (int)(*((unsigned char *)ptr)) == this->CollisionValue;
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
  
  for (idx = 0; idx < this->StateDimensionality; ++idx)
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
  for (idx = 0; idx < this->StateDimensionality; ++idx)
    {
    child[idx] = state[idx];
    }
  
  // add distance along one direction.
  child[axis] += distance;
}

  


//============================================================================
// Stuff specialized for 2d images.
//============================================================================


//----------------------------------------------------------------------------
// Description:
// This Method makes sure the canvas has been created. 
// The region must be set before this method is called.
void vtkImageStateSpace::CheckCanvas()
{
  int axes[3];
  
  if ( ! this->Region)
    {
    vtkErrorMacro(<< "CheckCanvas: Region must be set.");
    return;
    }
  this->Region->GetAxes(2, axes);
  axes[2] = VTK_IMAGE_COMPONENT_AXIS;
  
  if ( ! this->Canvas)
    {
    this->Canvas = new vtkImagePaint;
    this->Canvas->SetAxes(3, axes);
    this->Canvas->SetExtent(2, this->Region->GetExtent());
    this->Canvas->SetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, 0, 2);
    this->ClearCanvas();
    }

  if ( ! this->Viewer)
    {
    this->Viewer = vtkImageViewer::New();
    // Viewer is non standard.
    this->Viewer->SetAxes(axes[0], axes[1], axes[2]);
    this->Viewer->SetInput(this->Canvas->GetOutput());
    this->Viewer->SetColorWindow(1.0);
    this->Viewer->SetColorLevel(0.5);
    this->Viewer->ColorFlagOn();
    }
}

//----------------------------------------------------------------------------
// Description:
// This Method initializes the canvas from the region.
// It redraws the region in the canvas in gray scale.
void vtkImageStateSpace::ClearCanvas()
{
  vtkImageWrapPad *duplicate;
  int axes[3];
  
  if ( ! this->Region)
    {
    vtkErrorMacro(<< "CheckCanvas: Region must be set.");
    return;
    }
  
  if ( ! this->Canvas)
    {
    vtkErrorMacro(<< "CheckCanvas: no canvas.");
    return;
    }

  this->Region->GetAxes(2, axes);
  axes[2] = VTK_IMAGE_COMPONENT_AXIS;
  duplicate = new vtkImageWrapPad;
  duplicate->SetInput(this->Region->GetOutput());
  // Setup the ouput extent to match Canvas extent
  duplicate->SetAxes(this->Canvas->GetAxes());  
  duplicate->SetOutputImageExtent(this->Canvas->GetExtent());
  // Since Component must be repeated, it cannot be the last axis.
  duplicate->SetAxes(3, axes);
  duplicate->GetOutput()->UpdateRegion(this->Canvas);

  duplicate->Delete();
}


//----------------------------------------------------------------------------
vtkImageViewer *vtkImageStateSpace::GetViewer()
{
  this->CheckCanvas();
  return this->Viewer;
}

//----------------------------------------------------------------------------
vtkImagePaint *vtkImageStateSpace::GetCanvas()
{
  this->CheckCanvas();
  return this->Canvas;
}

//----------------------------------------------------------------------------
void vtkImageStateSpace::SampleCallBack(vtkClaw *planner)
{
  SphereList *list;
  Sphere *sphere;
  SphereList *neighbors;
  Sphere *neighbor;
  int x, y;
  int x2, y2;
  int count;
  
  printf("SampleCallBack\n");
  fflush(stdout);
  this->Planner = planner;
  
  if (this->StateDimensionality != 2)
    {
    vtkErrorMacro(<< "Call backs only work with images");
    return;
    }

  this->CheckCanvas();
  this->ClearCanvas();
  // Draw all of the spheres.
  list = planner->GetFreeSpheres();
  count = 0;
  while (list)
    {
    sphere = list->Item;
    x = (int)(floor(sphere->Center[0] + 0.5));
    y = (int)(floor(sphere->Center[1] + 0.5));
    // Draw the surface
    this->Canvas->SetDrawColor(1, 1, 0);
    this->Canvas->DrawCircle(x, y, sphere->Radius);
    // Draw the links to neighbors
    this->Canvas->SetDrawColor(1, 0, 0);
    neighbors = sphere->Neighbors;
    while(neighbors)
      {
      neighbor = neighbors->Item;
      x2 = (int)(floor(neighbor->Center[0] + 0.5));
      y2 = (int)(floor(neighbor->Center[1] + 0.5));
      this->Canvas->DrawSegment(x, y, x2, y2);
      neighbors = neighbors->Next;
      }
    ++count;
    list = list->Next;
    }
  
  printf("%d Spheres\n", count);
  fflush(stdout);

  // Draw all of the collision.
  list = planner->GetCollisionSpheres();
  count = 0;
  while (list)
    {
    sphere = list->Item;
    x = (int)(floor(sphere->Center[0] + 0.5));
    y = (int)(floor(sphere->Center[1] + 0.5));
    // Draw the collision location
    this->Canvas->SetDrawColor(0, 1, 0);
    this->Canvas->DrawPoint(x, y);
    ++count;
    list = list->Next;
    }
  
  printf("%d Collisions\n", count);
  fflush(stdout);

  this->Viewer->Render();
}

//----------------------------------------------------------------------------
void vtkImageStateSpace::CollisionCallBack(float *collision)
{
  SphereList *list;
  Sphere *sphere;
  SphereList *neighbors;
  Sphere *neighbor;
  int x, y;
  int x2, y2;
  int count;

  // I am through debugging.
  if(1)
    {
    return;
    }
  
  
  if (! this->Planner)
    {
    return;
    }
  
  printf("CollisionCallBack\n");
  fflush(stdout);
  
  if (this->StateDimensionality != 2)
    {
    vtkErrorMacro(<< "Call backs only work with images");
    return;
    }

  this->CheckCanvas();
  this->ClearCanvas();
  // Draw all of the spheres.
  list = this->Planner->GetFreeSpheres();
  count = 0;
  while (list)
    {
    sphere = list->Item;
    x = (int)(floor(sphere->Center[0] + 0.5));
    y = (int)(floor(sphere->Center[1] + 0.5));
    // Draw the surface
    this->Canvas->SetDrawColor(1, 1, 0);
    this->Canvas->DrawCircle(x, y, sphere->Radius);
    // Draw the links to neighbors
    this->Canvas->SetDrawColor(1, 0, 0);
    neighbors = sphere->Neighbors;
    while(neighbors)
      {
      neighbor = neighbors->Item;
      x2 = (int)(floor(neighbor->Center[0] + 0.5));
      y2 = (int)(floor(neighbor->Center[1] + 0.5));
      this->Canvas->DrawSegment(x, y, x2, y2);
      neighbors = neighbors->Next;
      }
    ++count;
    list = list->Next;
    }
  
  printf("%d Spheres\n", count);
  fflush(stdout);

  // Draw all of the collision.
  list = this->Planner->GetCollisionSpheres();
  count = 0;
  while (list)
    {
    sphere = list->Item;
    x = (int)(floor(sphere->Center[0] + 0.5));
    y = (int)(floor(sphere->Center[1] + 0.5));
    // Draw the collision location
    this->Canvas->SetDrawColor(0, 1, 0);
    this->Canvas->DrawPoint(x, y);
    ++count;
    list = list->Next;
    }
  
  printf("%d Collisions\n", count);
  fflush(stdout);

  // Draw the newest collision as a cross
  x = (int)(floor(collision[0] + 0.5));
  y = (int)(floor(collision[1] + 0.5));
  // Draw the collision location
  this->Canvas->SetDrawColor(0, 1, 0);
  this->Canvas->DrawSegment(x+5, y+5, x-5, y-5);
  this->Canvas->DrawSegment(x+5, y-5, x-5, y+5);
  
  this->Viewer->Render();
  printf("Collision: %d, %d \n", x, y);
  printf("Pause:");
  getchar();
  
}




//----------------------------------------------------------------------------
// Description:
// This method draws a path.  It only works for a 2d space for now.
// Canvas DrawColor can be set before this method is called.
void vtkImageStateSpace::DrawPath(vtkClaw *planner)
{
  int idx;
  int numberOfStates;
  float state1[2], state2[2];
  int x1, y1, x2, y2;

  // This only works for 2d data sets.
  if (this->StateDimensionality != 2)
    {
    vtkErrorMacro(<< "GetPathPolyData: Only hanldes volumes");
    return;
    }

  // Make sure the canvas has been created.
  this->CheckCanvas();
  
  if ( !planner || !this->Region)
    {
    return;
    }

  numberOfStates = planner->GetPathLength();
  
  for(idx = 1; idx < numberOfStates; ++idx)
    {
    planner->GetPathState(idx - 1, state1);
    planner->GetPathState(idx, state2);
    x1 = (int)(floor(state1[0] + 0.5));
    y1 = (int)(floor(state1[1] + 0.5));
    x2 = (int)(floor(state2[0] + 0.5));
    y2 = (int)(floor(state2[1] + 0.5));
    this->Canvas->DrawSegment(x1, y1, x2, y2);
    }
}





//============================================================================
// Stuff specialized for volumes.
//============================================================================


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

  // This only works for 3d data sets.
  if (this->StateDimensionality != 3)
    {
    vtkErrorMacro(<< "GetPathPolyData: Only hanldes volumes");
    return NULL;
    }
  
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

  
  // This only works for 3d data sets.
  if (this->StateDimensionality != 3)
    {
    vtkErrorMacro(<< "GetPathPolyData: Only hanldes volumes");
    return NULL;
    }
  
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
  
  // This only works for 3d data sets.
  if (this->StateDimensionality != 3)
    {
    vtkErrorMacro(<< "GetPathPolyData: Only hanldes volumes");
    return NULL;
    }
  
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

  
  


    





