/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRobotTransform2D.h
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
// .NAME vtkRobotTransform2D - RobotTransform2D repositions a robot.
// .SECTION Description
// vtkRobotTransform2D will reposition a robot. The robot is first rotated
// by Theta (units radians) and then translated by X and Y.

#ifndef __vtkRobotTransform2D_h
#define __vtkRobotTransform2D_h

#include "vtkRobot2D.h"

class vtkRobotTransform2D : public vtkRobot2D
{
public:
  vtkRobotTransform2D();
  ~vtkRobotTransform2D();
  char *GetClassName() {return "vtkRobotTransform2D";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Set/Get the robot to transform.
  vtkSetObjectMacro(Robot, vtkRobot2D);
  
  // Description:
  // Set/Get the rotation Theta in radians.
  vtkSetMacro(Theta, float);
  vtkGetMacro(Theta, float);
  
  // Description:
  // Set/Get the translation.
  vtkSetMacro(X, float);
  vtkGetMacro(X, float);
  vtkSetMacro(Y, float);
  vtkGetMacro(Y, float);

  void TransformDraw(float x, float y, float s, float c, vtkImageDraw *canvas);
  void GetBounds(float bounds[4]);
  int TransformCollide(vtkImageRegion *distanceMap, 
		       float x, float y, float s, float c);

protected:
  vtkRobot2D *Robot;
  float Theta;
  float X;
  float Y;
};

#endif















