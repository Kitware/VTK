/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRobotGroup2D.h
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
// .NAME vtkRobotGroup2D - RobotGroup2D Groups multiple robots into one.
// .SECTION Description
// vtkRobotGroup2D takes multiple robots and groups them into one robot.
// Each sub robot has a 3D state (x,y,theta) to position itself in the whole.
// The relative positions of sub robots is fixed.

#ifndef __vtkRobotGroup2D_h
#define __vtkRobotGroup2D_h

#include "vtkRobot2D.h"

class vtkRobotGroup2D : public vtkRobot2D
{
public:
  vtkRobotGroup2D();
  ~vtkRobotGroup2D();
  char *GetClassName() {return "vtkRobotGroup2D";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a robot to the group.
  void AddRobot(vtkRobot2D *robot);
  
  // Get the number of robots in the group.
  vtkGetMacro(NumberOfRobots, int);
  
  void TransformDraw(float x, float y, float s, float c,vtkImagePaint *canvas);
  void GetBounds(float bounds[4]);
  int TransformCollide(vtkImageRegion *distanceMap, 
		       float x, float y, float s, float c);

protected:
  int NumberOfRobots;
  vtkRobot2D **Robots;
  int RobotsArrayLength;
};

#endif















