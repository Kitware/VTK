/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRobotJoint2D.h
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
// .NAME vtkRobotJoint2D - Join two robots with a flexible rotation joint.
// .SECTION Description
// vtkRobotJoint2D will connect two robots with a flexible rotational joint.
// This adds an extra degree of freedom.  The Joint angle is specified
// by Theta (units radians).  This adds an extra degree of 
// freedom to the robot.  There is no limitation on rotation of the joint yet.
// RobotA is stationary, and RobotB is rotated around the PivotPoint.


#ifndef __vtkRobotJoint2D_h
#define __vtkRobotJoint2D_h

#include "vtkRobot2D.h"

class VTK_EXPORT vtkRobotJoint2D : public vtkRobot2D
{
public:
  vtkRobotJoint2D();
  ~vtkRobotJoint2D();
  char *GetClassName() {return "vtkRobotJoint2D";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the two robots to join
  vtkSetObjectMacro(RobotA, vtkRobot2D);
  vtkGetObjectMacro(RobotA, vtkRobot2D);
  vtkSetObjectMacro(RobotB, vtkRobot2D);
  vtkGetObjectMacro(RobotB, vtkRobot2D);
  
  // Description:
  // Set/Get the rotation Theta in radians.
  vtkSetMacro(Theta, float);
  vtkGetMacro(Theta, float);
  
  // Description:
  // Set/Get the Pivot point of the rotation.
  vtkSetVector2Macro(Pivot, float);
  vtkGetVector2Macro(Pivot, float);

  void TransformDraw(float x, float y, float s, float c,vtkImagePaint *canvas);
  void GetBounds(float bounds[4]);
  int TransformCollide(vtkImageRegion *distanceMap, 
		       float x, float y, float s, float c);
  
  // Description::
  // Set/Get the factor to scale Theta to have same "units" as translation.
  // Externally computed for now.
  vtkSetMacro(Factor,float);
  vtkGetMacro(Factor,float);
  
  
protected:
  vtkRobot2D *RobotA;
  vtkRobot2D *RobotB;  
  float Pivot[2];
  float Theta;
  // Factor to scale theta to same "units" as translation.
  float Factor;
};

#endif















