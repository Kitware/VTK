/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConeSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkConeSource - generate polygonal cone 
// .SECTION Description
// vtkConeSource creates a cone centered at origin and pointing down the 
// x-axis. Depending upon the resolution of this object, different 
// representations are created. If resolution=0 a line is created; if 
// resolution=1, a single triangle is created; if resolution=2, two 
// crossed triangles are created. For resolution > 2, a 3D cone (with 
// resolution number of sides) is created. It also is possible to control 
// whether the bottom of the cone is capped with a (resolution-sided) 
// polygon, and to specify the height and radius of the cone.

#ifndef __vtkConeSource_h
#define __vtkConeSource_h

#include "vtkPolySource.h"

class vtkConeSource : public vtkPolySource 
{
public:
  vtkConeSource(int res=6);
  char *GetClassName() {return "vtkConeSource";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the height of the cone.
  vtkSetClampMacro(Height,float,0.0,VTK_LARGE_FLOAT)
  vtkGetMacro(Height,float);

  // Description:
  // Set the radius of the cone.
  vtkSetClampMacro(Radius,float,0.0,VTK_LARGE_FLOAT)
  vtkGetMacro(Radius,float);

  // Description:
  // Set the number of facets used to represent the cone.
  vtkSetClampMacro(Resolution,int,0,VTK_CELL_SIZE)
  vtkGetMacro(Resolution,int);

  // Description:
  // Set the angle of the cone. As a side effect, this method sets radius.
  void SetAngle (float angle);
  float GetAngle ();

  // Description:
  // Turn on/off whether to cap the cone with a polygon.
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);

protected:
  void Execute();
  float Height;
  float Radius;
  int Resolution;
  int Capping;

};

#endif


