/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRobotSegment2D.h
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
// .NAME vtkRobotSegment2D - The smallest robot building block, a line segment.
// .SECTION Description
// vtkRobotSegment2D defines a line segment wich can be used to build bigger
// robots.

#ifndef __vtkRobotSegment2D_h
#define __vtkRobotSegment2D_h

#include "vtkRobot2D.h"

class VTK_EXPORT vtkRobotSegment2D : public vtkRobot2D
{
public:
  vtkRobotSegment2D();
  ~vtkRobotSegment2D();
  char *GetClassName() {return "vtkRobotSegment2D";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the two points that define a segment.
  vtkSetVector2Macro(PointA, float);
  vtkGetVector2Macro(PointA, float); 
  vtkSetVector2Macro(PointB, float);
  vtkGetVector2Macro(PointB, float);

  void TransformDraw(float x, float y, float s, float c,vtkImagePaint *canvas);
  void GetBounds(float bounds[4]);
  int TransformCollide(vtkImageRegion *distanceMap, 
		       float x, float y, float s, float c);

  
protected:
  float PointA[2];
  float PointB[2];

  int CollideSegment(float x0, float y0, short d0,
		     float x1, float y1, short d1,
		     float length, short *map, 
		     int xInc, int yInc);
};

#endif















