/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImage2DRobotSpace.cxx
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
#include <stdio.h>
#include <math.h>
#include "vtkImageXViewer.h"
#include "vtkImage2DRobotSpace.h"
#include "vtkImageDistance.h"


// State is in pixel units.

//----------------------------------------------------------------------------
vtkImage2DRobotSpace::vtkImage2DRobotSpace()
{
  this->Canvas = NULL;
  this->DistanceMap = NULL;
  this->WorkSpace = NULL;
  this->Threshold = 1.0;
  this->MaximumNumberOfSegments = 0;
  this->NumberOfSegments = 0;
  this->Segments = NULL;
}


//----------------------------------------------------------------------------
vtkImage2DRobotSpace::~vtkImage2DRobotSpace()
{
  if (this->WorkSpace)
    {
    this->WorkSpace->Delete();
    }
  if (this->DistanceMap)
    {
    this->DistanceMap->Delete();
    }
  if (this->Canvas)
    {
    this->Canvas->Delete();
    }
  if (this->Segments)
    {
    delete [] this->Segments;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method  creates the array to hold the segments and should be called
// before segemnts are added.
void vtkImage2DRobotSpace::SetNumberOfSegments(int number)
{
  if (this->Segments)
    {
    delete [] this->Segments;
    }
  this->NumberOfSegments = 0;
  this->MaximumNumberOfSegments = number;
  this->Segments = new float[number * 4];
}



//----------------------------------------------------------------------------
// Description:
// Add a segment to the robot.
void vtkImage2DRobotSpace::AddSegment(float x0, float y0, float x1, float y1)
{
  float *ptr;
  int flag = 0;
  
  if (this->NumberOfSegments >= this->MaximumNumberOfSegments)
    {
    vtkErrorMacro(<< "AddSegment: Too many segments");
    return;
    }
  ptr = this->Segments + 4*this->NumberOfSegments;
  *ptr++ = x0;
  *ptr++ = y0;
  *ptr++ = x1;
  *ptr++ = y1;

  ++(this->NumberOfSegments);
  
  // Keep track of bounds so we know how to scale rotation.
  if (this->NumberOfSegments == 1)
    {
    this->RotationFactor = 0.0;
    this->RobotBounds[0] = x0;
    this->RobotBounds[1] = x0;
    this->RobotBounds[2] = y0;
    this->RobotBounds[3] = y0;
    }
  if (x0 < this->RobotBounds[0])
    {
    flag = 1;
    this->RobotBounds[0] = x0;
    }
  if (x0 > this->RobotBounds[1])
    {
    flag = 1;
    this->RobotBounds[1] = x0;
    }
  if (x1 < this->RobotBounds[0])
    {
    flag = 1;
    this->RobotBounds[0] = x1;
    }
  if (x1 > this->RobotBounds[1])
    {
    flag = 1;
    this->RobotBounds[1] = x1;
    }
  if (y0 < this->RobotBounds[2])
    {
    flag = 1;
    this->RobotBounds[2] = y0;
    }
  if (y0 > this->RobotBounds[3])
    {
    flag = 1;
    this->RobotBounds[3] = y0;
    }
  if (y1 < this->RobotBounds[2])
    {
    flag = 1;
    this->RobotBounds[2] = y1;
    }
  if (y1 > this->RobotBounds[3])
    {
    flag = 1;
    this->RobotBounds[3] = y1;
    }
  
  // If the this->RobotBounds were modified, recompute Rotation factor.
  if (flag)
    {
    float diameter, dx, dy;
    dx = this->RobotBounds[1] - this->RobotBounds[0];
    dy = this->RobotBounds[3] - this->RobotBounds[2];
    diameter = sqrt (dx*dx + dy*dy);
    // 1 / radius
    this->RotationFactor = 2.0 / diameter;
    }
}










//----------------------------------------------------------------------------
// Description:
// This method removes redundant locations in state space.
void vtkImage2DRobotSpace::Wrap(float *state)
{
  // Third dimension represents orientation.
  while (state[2] < 0.0)
    {
    state[2] += 6.283185307;   // 2pi
    }
  while (state[2] > 6.283185307)
    {
    state[2] -= 6.283185307;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method returns 0.0 if a state is out of the image bounds.
// Values are middle of pixels (rounded)
float vtkImage2DRobotSpace::BoundsTest(float *state)
{
  int idx;
  int round;
  int *extent;
  
  // Make sure the state is in the bounds of the strucutred point set.
  extent = this->DistanceMap->GetExtent();
  for (idx = 0; idx < 2; ++idx)
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
// This method takes a region.  All zero values are assumed to be collision.
// All 1 values are assumed to be open space..
// A distance map is created from it.
void vtkImage2DRobotSpace::SetWorkSpace(vtkImageRegion *region)
{
  vtkImageDistance *distanceFilter = new vtkImageDistance;
  
  // Copy the work space (we should have a threshold filter in here).
  this->Modified();
  if (this->WorkSpace)
    {
    this->WorkSpace->Delete();
    }
  this->WorkSpace = new vtkImageRegion;
  this->WorkSpace->SetScalarType(VTK_SHORT);
  this->WorkSpace->SetExtent(VTK_IMAGE_DIMENSIONS, region->GetExtent());
  this->WorkSpace->CopyRegionData(region);
  
  // Create a new distance map.
  if (this->DistanceMap)
    {
    this->DistanceMap->Delete();
    }
  this->DistanceMap = new vtkImageRegion;
  // Set the size of the map to be the same as the region
  this->DistanceMap->SetExtent(VTK_IMAGE_DIMENSIONS, region->GetExtent());
  this->DistanceMap->SetScalarType(VTK_SHORT);
  // Use the distance fitler to generate the map.
  distanceFilter->SetDimensionality(2);
  distanceFilter->SetInput(region);
  distanceFilter->GetOutput()->UpdateRegion(this->DistanceMap);

  // Get rid of the filter.
  distanceFilter->Delete();

  // Create a new canvas with the same dimensions as the workspace.
  if (this->Canvas)
    {
    this->Canvas->Delete();
    }
  this->Canvas = new vtkImageDraw;
  this->Canvas->SetExtent(VTK_IMAGE_DIMENSIONS, this->WorkSpace->GetExtent());
  this->Canvas->SetScalarType(this->WorkSpace->GetScalarType());
  this->ClearCanvas();
  this->Canvas->SetDrawValue(0.0);
}

  

//----------------------------------------------------------------------------
// This method computes max distance between two points.
// manhaten distance. 
float vtkImage2DRobotSpace::Distance(float *p0, float *p1)
{
  int idx;
  float sum;
  
  // handle rotation
  sum = fabs(p0[2] - p1[2]);
  // Make sure the sum is less than PI
  if (sum > 3.1415927)
    {
    sum = 6.2831835 - sum;
    }
  sum /= this->RotationFactor;
  
  // handle position
  for (idx = 0; idx < 2; ++idx)
    {
    sum += fabs(p0[idx] - p1[idx]);
    }
  
  return sum;
}






//----------------------------------------------------------------------------
// This method determines collision space from free space
int vtkImage2DRobotSpace::Collide(float *state)
{
  float *segment = this->Segments;
  float x0,y0, x1,y1, s,c;
  short *map;
  short d0, d1;
  float length;
  int *extent;
  int xInc, yInc, x, y;
  int idx;

  extent = this->DistanceMap->GetExtent();
  this->DistanceMap->GetIncrements(xInc, yInc);
  map = (short *)(this->DistanceMap->GetScalarPointer());
  s = sin(state[2]);
  c = cos(state[2]);
  for (idx = 0; idx < this->NumberOfSegments; ++idx)
    {
    x0 = state[0] + c*segment[0] + s*segment[1];
    y0 = state[1] + c*segment[1] - s*segment[0];
    x1 = state[0] + c*segment[2] + s*segment[3];
    y1 = state[1] + c*segment[3] - s*segment[2];
    segment += 4;
    
    // Do the initial check
    length = fabs(x1-x0) + fabs(y1-y0);
    x = (int)(floor(x0 + 0.5));
    y = (int)(floor(y0 + 0.5));
    // Make sure the point is in the map
    if (x < extent[0] || x > extent[1] || y < extent[2] || y > extent[3])
      {
      return 1;
      }
    d0 = *(map + x*xInc + y*yInc);
    x = (int)(floor(x1 + 0.5));
    y = (int)(floor(y1 + 0.5));
    // Make sure the point is in the map
    if (x < extent[0] || x > extent[1] || y < extent[2] || y > extent[3])
      {
      return 1;
      }
    d1 = *(map + x*xInc + y*yInc);
    // check for imediate collision
    if (d0 == 0 || d1 == 0)
      {
      return 1;
      }
    // do not call recursive collide if we have wide clearance
    if ((length >= (float)(d0)-0.5) || (length >= (float)(d1)-0.5))
      {
      // Call recursive segment collide
      if (this->CollideSegment(x0,y0,d0, x1,y1,d1, length, map, xInc, yInc))
	{
	return 1;
	}
      }
    }
  
  
  return 0;
}




//----------------------------------------------------------------------------
// This method determines collision space from free space
int vtkImage2DRobotSpace::CollideSegment(float x0, float y0, short d0,
					 float x1, float y1, short d1,
					 float length, short *map, 
					 int xInc, int yInc)
{
  float xMid, yMid;
  int x, y;
  short dMid;

  // Find the middle of the segment
  xMid = (x0 + x1) / 2.0;
  yMid = (y0 + y1) / 2.0;
  x = (int)(floor(xMid + 0.5));
  y = (int)(floor(yMid + 0.5));
  dMid = *(map + x*xInc + y*yInc);
  length *= 0.5;

  // check for imediate collision
  if (dMid == 0)
    {
    return 1;
    }
  // check for wide clearance.
  if (length < d0-1 && length < d1-1 && length < dMid-1)
    {
    return 0;
    }

  // Call recursive segment collide
  if ((length >= (float)(d0)-0.5) || (length >= (float)(dMid)-0.5))
    {
    if (this->CollideSegment(x0,y0,d0, xMid,yMid,dMid, length, map, xInc,yInc))
      {
      return 1;
      }
    }

  if ((length >= (float)(d1)-0.5) || (length >= (float)(dMid)-0.5))
    {
    if (this->CollideSegment(xMid,yMid,dMid, x1,y1,d1, length, map, xInc,yInc))
      {
      return 1;
      }
    }

  return 0;
}

  

  
  
  
//----------------------------------------------------------------------------
// This method returns the state mid-way between s0 and s1.
void vtkImage2DRobotSpace::GetMiddleState(float *s0, float *s1, float *middle)
{
  int idx;
  float temp;
  
  for (idx = 0; idx < 2; ++idx)
    {
    middle[idx] = (s0[idx] + s1[idx]) / 2.0;
    }
  
  // Find the shortest path
  temp = s0[2] - s1[2];
  if (temp > 3.1415927)
    {
    temp -= 6.2831853;
    }
  if (temp < -3.1415927)
    {
    temp += 6.2831853;
    }
  middle[2] = s1[2] + temp * 0.5;

  // Convert back to range 0->2PI.
  this->Wrap(middle);
}




//----------------------------------------------------------------------------
// This method finds a child of a state.  This is a new state a specified
// distance along an axis from the first state.
void vtkImage2DRobotSpace::GetChildState(float *state, int axis, 
					 float distance, float *child)
{
  int idx;

  // First copy the state.
  for (idx = 0; idx < VTK_CLAW_DIMENSIONS; ++idx)
    {
    child[idx] = state[idx];
    }
  
  // add distance along one direction.
  if (axis < 2)
    {
    child[axis] += distance;
    }
  else
    {
    child[2] += distance * this->RotationFactor;
    }
}




//============================================================================
// Methods for drawing the robot.



//----------------------------------------------------------------------------
// This method reinitializes the canvas with the Workspace.
void vtkImage2DRobotSpace::ClearCanvas()
{
  if (this->Canvas && this->WorkSpace)
    {
    this->Canvas->CopyRegionData(this->WorkSpace);
    }
}


//----------------------------------------------------------------------------
// This method draws the robot on the canvas.
void vtkImage2DRobotSpace::DrawRobot(float *state)
{
  float *segment = this->Segments;
  float x0,y0, x1,y1, s,c;
  int idx;

  // Make sure we have a canvas.
  if ( ! this->Canvas)
    {
    return;
    }

  s = sin(state[2]);
  c = cos(state[2]);
  // loop through all the segments.
  for (idx = 0; idx < this->NumberOfSegments; ++idx)
    {
    // transform the segment using the state.
    x0 = state[0] + c*segment[0] + s*segment[1];
    y0 = state[1] + c*segment[1] - s*segment[0];
    x1 = state[0] + c*segment[2] + s*segment[3];
    y1 = state[1] + c*segment[3] - s*segment[2];
    segment += 4;
    
    // draw the segment
    this->Canvas->DrawSegment((int)(floor(x0 + 0.5)), (int)(floor(y0 + 0.5)),
			      (int)(floor(x1 + 0.5)), (int)(floor(y1 + 0.5)));
    }
}


//----------------------------------------------------------------------------
// This method animates a path
void vtkImage2DRobotSpace::AnimatePath(vtkClaw *planner)
{
  int idx;
  int numberOfStates;
  float state[3];
  vtkImageXViewer *viewer;
  
  if ( !planner || !this->Canvas)
    {
    return;
    }

  viewer = new vtkImageXViewer;  
  viewer->SetInput(this->Canvas->GetOutput());
  
  numberOfStates = planner->GetPathLength();
  
  for(idx = 0; idx < numberOfStates; ++idx)
    {
    planner->GetPathState(idx, state);
    this->ClearCanvas();
    this->DrawRobot(state);
    viewer->Render();
    printf("%d: pause:", idx);
    getchar();
    }
  
  viewer->Delete();
}

    








