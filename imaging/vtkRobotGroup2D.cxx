/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRobotGroup2D.cxx
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

#include <stdlib.h>
#include "vtkRobotGroup2D.h"

//----------------------------------------------------------------------------
vtkRobotGroup2D::vtkRobotGroup2D()
{
  this->NumberOfRobots = 0;
  this->Robots = NULL;
  this->RobotsArrayLength = 0;
}


//----------------------------------------------------------------------------
vtkRobotGroup2D::~vtkRobotGroup2D()
{
  int idx;
  
  for (idx = 0; idx < this->NumberOfRobots; ++idx)
    {
    if ( ! this->Robots[idx])
      {
      this->Robots[idx]->Delete();
      }
    }
  
  if (this->Robots)
    {
    delete [] this->Robots;
    }
}


//----------------------------------------------------------------------------
void vtkRobotGroup2D::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  this->vtkRobot2D::PrintSelf(os, indent);
  os << indent << "NumberOfRobots: " << this->NumberOfRobots << "\n";
  for (idx = 0; idx < this->NumberOfRobots; ++idx)
    {
    os << indent << "Robot" << idx << ":\n";
    this->Robots[idx]->PrintSelf(os, indent.GetNextIndent());
    }
}



//----------------------------------------------------------------------------
void vtkRobotGroup2D::AddRobot(vtkRobot2D *robot)
{
  if (robot == NULL)
    {
    return;
    }
  
  // Make sure array is long enough
  if (this->NumberOfRobots >= this->RobotsArrayLength)
    {
    vtkRobot2D **newArray;
    int idx;
    this->RobotsArrayLength += 10;
    newArray = (vtkRobot2D **)malloc(sizeof(void *) * this->RobotsArrayLength);
    for (idx = 0; idx < this->NumberOfRobots; ++idx)
      {
      newArray[idx] = this->Robots[idx];
      }
    if (this->Robots)
      {
      delete [] this->Robots;
      }
    this->Robots = newArray;
    }
  
  // Add the new robot
  this->Robots[this->NumberOfRobots] = robot;
  ++(this->NumberOfRobots);
}




//----------------------------------------------------------------------------
// Description:
// Translate the robot (X, Y, sin(Theta), cos(Theta)), and then draw it.
void vtkRobotGroup2D::TransformDraw(float x, float y, float s, float c,
				    vtkImagePaint *canvas)
{
  int idx;

  // just draw each robot in the group.
  for (idx = 0; idx < this->NumberOfRobots; ++idx)
    {
    this->Robots[idx]->TransformDraw(x, y, s, c, canvas);
    }
}





//----------------------------------------------------------------------------
// Description:
// Returns the bounds of the whole groop.
void vtkRobotGroup2D::GetBounds(float bounds[4])
{
  float temp[4];
  int idx;
  
  if (this->NumberOfRobots < 0)
    {
    vtkErrorMacro(<< "GetBounds: No Robots.");
    return;
    }
  
  this->Robots[0]->GetBounds(bounds);
  for (idx = 1; idx < this->NumberOfRobots; ++idx)
    {
    this->Robots[idx]->GetBounds(temp);
    bounds[0] = (bounds[0] < temp[0]) ? bounds[0] : temp[0];
    bounds[1] = (bounds[1] > temp[1]) ? bounds[1] : temp[1];
    bounds[2] = (bounds[2] < temp[2]) ? bounds[2] : temp[2];
    bounds[3] = (bounds[3] > temp[3]) ? bounds[3] : temp[3];
    }
}

  
//----------------------------------------------------------------------------
// Description:
// Returns 1 if the robot is in collision.  each pixel of distanceMap should
// contain the distance to a boundary.  (Manhatten distance).
int vtkRobotGroup2D::TransformCollide(vtkImageRegion *distanceMap,
				      float x, float y, float s, float c)
{
  int idx;
  
  for (idx = 0; idx < this->NumberOfRobots; ++idx)
    {
    if (this->Robots[idx]->TransformCollide(distanceMap, x, y, s, c))
      {
      return 1;
      }
    }
  return 0;
}

  













