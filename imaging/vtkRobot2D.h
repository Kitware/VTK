/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRobot2D.h
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
// .NAME vtkRobot2D - Robot2D super class for robots and pieces of robots.
// .SECTION Description
// vtkRobot2D is a superclass of robot parts.  The resulting robots
// are used to create state spaces for CLAW to search.

#ifndef __vtkRobot2D_h
#define __vtkRobot2D_h

#include <math.h>
#include "vtkObject.h"
#include "vtkImageDraw.h"

class vtkRobot2D : public vtkObject
{
public:
  vtkRobot2D();
  ~vtkRobot2D();
  char *GetClassName() {return "vtkRobot2D";};

  void Draw(vtkImageDraw *canvas);
  virtual void TransformDraw(float x, float y, float s, float c,
			     vtkImageDraw *canvas) = 0;
  virtual void GetBounds(float bounds[4]) = 0;
  int Collide(vtkImageRegion *distanceMap)
  {return this->TransformCollide(distanceMap, 0, 0, 0, 1);};
  virtual int TransformCollide(vtkImageRegion *distanceMap, 
			       float x, float y, float s, float c) = 0;
  
protected:
  //void MultiplyTransforms(float in1[3][3], float in2[3][3], float out[3][3]);
  //void CopyTransforms(float in[3][3], float out[3][3]);
  //void RotateTransform(float theat, float t[3][3]);
  //void IdentityTransform(float t[3][3]);
  //void TranslateTransform(float x, float y, float t[3][3]);
  //void ApplyTransform(float t[3][3], float in[3], float out[3]);
};

#endif















