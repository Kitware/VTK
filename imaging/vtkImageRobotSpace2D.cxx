/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRobotSpace2D.cxx
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
//#include "vtkImageXViewer.h"
#include "vtkImageRobotSpace2D.h"
#include "vtkImageDistance.h"


#define GUIDE_POINT_HACK 50.0

// State is in pixel units.

//----------------------------------------------------------------------------
vtkImageRobotSpace2D::vtkImageRobotSpace2D()
{
  this->Robot = NULL;
  this->Canvas = NULL;
  this->DistanceMap = NULL;
  this->WorkSpace = NULL;
  this->Threshold = 1.0;
  this->MaximumNumberOfJoints = 0;
  this->NumberOfJoints = 0;
  this->Joints = NULL;
}


//----------------------------------------------------------------------------
vtkImageRobotSpace2D::~vtkImageRobotSpace2D()
{
  if (this->Robot)
    {
    this->Robot->Delete();
    }
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
  if (this->Joints)
    {
    delete this->Joints;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method  creates the array to hold the joints and should be called
// before jointss are added.
void vtkImageRobotSpace2D::SetNumberOfJoints(int number)
{
  if (this->Joints)
    {
    delete this->Joints;
    }
  this->NumberOfJoints = 0;
  this->MaximumNumberOfJoints = number;
  this->Joints = (vtkRobotJoint2D **)malloc(sizeof (void *) * number);
}



//----------------------------------------------------------------------------
// Description:
// Sets the robot and computes the rotation factor at the same time.
void vtkImageRobotSpace2D::SetRobot(vtkRobotTransform2D *robot)
{
  float bounds[4];
  float diameter;
  
  this->Modified();
  this->Robot = robot;
  robot->GetBounds(bounds);
  diameter = fabs(bounds[1] - bounds[0]) + fabs(bounds[3] - bounds[2]);
  this->RotationFactor = 2.0 / diameter;
}






//----------------------------------------------------------------------------
// Description:
// Add a segment to the robot.
void vtkImageRobotSpace2D::AddJoint(vtkRobotJoint2D *joint)
{
  float bounds[4];
  float pivot[2];
  float max, temp;
  
  if (this->NumberOfJoints >= this->MaximumNumberOfJoints)
    {
    vtkErrorMacro(<< "AddJoint: Too many joints");
    return;
    }

  this->Modified();
  this->Joints[this->NumberOfJoints] = joint;
  ++(this->NumberOfJoints);
  
  // Compute the joint rotation factor here.
  joint->GetRobotB()->GetBounds(bounds);
  joint->GetPivot(pivot);
  // Compute the maximum radius.
  max = 0;
  temp = fabs(pivot[0] - bounds[0]) + fabs(pivot[1] - bounds[2]);
  max = (temp > max) ? temp : max;
  temp = fabs(pivot[0] - bounds[0]) + fabs(pivot[1] - bounds[3]);
  max = (temp > max) ? temp : max;
  temp = fabs(pivot[0] - bounds[1]) + fabs(pivot[1] - bounds[2]);
  max = (temp > max) ? temp : max;
  temp = fabs(pivot[0] - bounds[1]) + fabs(pivot[1] - bounds[3]);
  max = (temp > max) ? temp : max;
  
  joint->SetFactor(1.0 / max);
}










//----------------------------------------------------------------------------
// Description:
// This method removes redundant locations in state space.
void vtkImageRobotSpace2D::Wrap(float *state)
{
  int idx;
  
  // Loop: Robot rotation + joint rotations.
  state += 2;
  for (idx = 0; idx <= this->NumberOfJoints; ++idx)
    {
    while (*state < -3.1415927)
      {
      *state += 6.283185307;   // 2pi
      }
    while (*state > 3.1415927)
      {
      *state -= 6.283185307;
      }
    
    ++state;
    }
}


//----------------------------------------------------------------------------
// Description:
// This method returns 0.0 if a state is out of the image bounds.
// Values are middle of pixels (rounded)
float vtkImageRobotSpace2D::BoundsTest(float *state)
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
void vtkImageRobotSpace2D::SetWorkSpace(vtkImageRegion *region)
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
float vtkImageRobotSpace2D::Distance(float *p0, float *p1)
{
  int idx;
  float sum, temp;
  
  // handle position
  sum = 0.0;
  for (idx = 0; idx < 2; ++idx)
    {
    temp = (*p0++ - *p1++);
    // Try a hack for guide point.
    temp *= GUIDE_POINT_HACK;
    sum += temp * temp;
    //sum += fabs(*p0++ - *p1++);
    }
  
  // handle robot rotation
  temp = fabs(*p0++ - *p1++);
  // Make sure the difference is less than PI
  if (temp > 3.1415927)
    {
    temp = 6.2831835 - temp;
    }
  temp /= this->RotationFactor;
  sum += temp * temp;
  
  // handle joint rotation
  for (idx = 0; idx < this->NumberOfJoints; ++idx)
    {
    temp = fabs(*p0++ - *p1++);
    // Make sure the difference is less than PI
    if (temp > 3.1415927)
      {
      temp = 6.2831835 - temp;
      }
    temp /= this->Joints[idx]->GetFactor();
    sum += temp * temp;
    }
  
  return sqrt(sum);
}


//----------------------------------------------------------------------------
// This method determines collision space from free space
int vtkImageRobotSpace2D::Collide(float *state)
{
  int idx;
  
  // Set the parameters from the state.
  this->Robot->SetX(*state++);
  this->Robot->SetY(*state++);
  this->Robot->SetTheta(*state++);
  for (idx = 0; idx < this->NumberOfJoints; ++idx)
    {
    this->Joints[idx]->SetTheta(*state++);
    }
  
  return this->Robot->Collide(this->DistanceMap);
}

  
//----------------------------------------------------------------------------
// This method returns the state mid-way between s0 and s1.
void vtkImageRobotSpace2D::GetMiddleState(float *s0, float *s1, float *middle)
{
  int idx;
  float temp, *tmid = middle;
  
  // Position
  for (idx = 0; idx < 2; ++idx)
    {
    *tmid++ = (*s0++ + *s1++) / 2.0;
    }
  
  // Rotation of robot.
  temp = *s0 - *s1;
  while (temp > 3.1415927)
    {
    temp -= 6.283185307;
    }
  while (temp < -3.1415927)
    {
    temp += 6.283185307;
    }
  *tmid = *s1 + temp * 0.5;
  ++tmid;
  ++s0;
  ++s1;

    // NumberOfJoints +1 for Robot rotation.
  for (idx = 0; idx < this->NumberOfJoints; ++idx)
    {
    // Find the shortest path
    temp = *s0 - *s1;
    if (temp > 3.1415927)
      {
      temp -= 6.2831853;
      }
    if (temp < -3.1415927)
      {
      temp += 6.2831853;
      }
    *tmid = *s1 + temp * 0.5;
    ++tmid;
    ++s0;
    ++s1;
    }

  // Convert back to range 0->2PI.
  this->Wrap(middle);
}




//----------------------------------------------------------------------------
// This method finds a child of a state.  This is a new state a specified
// distance along an axis from the first state.
void vtkImageRobotSpace2D::GetChildState(float *state, int axis, 
					 float distance, float *child)
{
  int idx;

  // First copy the state.
  for (idx = 0; idx < this->GetStateDimensionality(); ++idx)
    {
    child[idx] = state[idx];
    }
  
  // add distance along one direction.
  if (axis < 2)
    {
    child[axis] += distance / GUIDE_POINT_HACK;
    }
  else if (axis == 2)
    {
    child[2] += distance * this->RotationFactor;
    }
  else
    {
    child[axis] += distance * this->Joints[axis - 3]->GetFactor();
    }
  
  this->Wrap(child);
}




//============================================================================
// Methods for drawing the robot.



//----------------------------------------------------------------------------
// This method reinitializes the canvas with the Workspace.
void vtkImageRobotSpace2D::ClearCanvas()
{
  if (this->Canvas && this->WorkSpace)
    {
    this->Canvas->CopyRegionData(this->WorkSpace);
    }
}


//----------------------------------------------------------------------------
// This method draws the robot on the canvas.
void vtkImageRobotSpace2D::DrawRobot(float *state)
{
  int idx;
  
  // Make sure we have a canvas.
  if ( ! this->Canvas || ! this->Robot)
    {
    return;
    }

  // Set the parameters from the state.
  this->Robot->SetX(*state++);
  this->Robot->SetY(*state++);
  this->Robot->SetTheta(*state++);
  for (idx = 0; idx < this->NumberOfJoints; ++idx)
    {
    this->Joints[idx]->SetTheta(*state++);
    }
  
  this->Robot->Draw(this->Canvas);
}


//----------------------------------------------------------------------------
// This method animates a path
void vtkImageRobotSpace2D::AnimatePath(vtkClaw *planner)
{
  int idx, idxJoint;
  int numberOfStates;
  float *state;
  //vtkImageXViewer *viewer;
  
  if ( !planner || !this->Canvas)
    {
    return;
    }

  //viewer = new vtkImageXViewer;  
  //viewer->SetInput(this->Canvas->GetOutput());
  
  numberOfStates = planner->GetPathLength();
  state = this->NewState();
  for(idx = 0; idx < numberOfStates; ++idx)
    {
    planner->GetPathState(idx, state);
    // Set the parameters from the state.
    this->Robot->SetX(*state++);
    this->Robot->SetY(*state++);
    this->Robot->SetTheta(*state++);
    for (idxJoint = 0; idxJoint < this->NumberOfJoints; ++idxJoint)
      {
      this->Joints[idxJoint]->SetTheta(*state++);
      }
    
    this->ClearCanvas();
    this->Robot->Draw(this->Canvas);
    //viewer->Render();
    printf("%d: pause:", idx);
    getchar();
    }
  
  //viewer->Delete();
}

    









