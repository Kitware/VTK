/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlaneSource.hh
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
// .NAME vtkPlaneSource - create an array of quadrilaterals located in a plane
// .SECTION Description
// vtkPlaneSource creates an m x n array of quadrilaterals arranged as a
// regular tiling in a plane. The plane is defined by specifying an origin
// point, and then two other points that, together with the origin, define two
// axes for the plane. These axes do not have to be orthogonal - so you can 
// create a parallelogram. (The axes must not be parallel.) By default, the 
// plane is centered at the origin and perpendicular to the z-axis, with 
// width and height of length 1. The resolution of the plane (i.e., number 
// of subdivisions) is controlled by the ivars XResolution and YResolution.
//
// There are three conveience methods that allow you to easily move the plane. 
// The first, SetNormal(), allows you to specify the plane normal. The effect
// of this method is to rotate the plane around the center of the plane, 
// aligning the plane normal with the specified normal. The second, SetCenter(),
// translates the center of the plane to the specified center point. The third
// method, Push(), allows you to translate the plane along the plane normal by 
// the distance specified. (Negative Push values translate the plane in the 
// negative normal direction.)  Note that the SetNormal(), SetCenter() and Push() 
// methods modify the Origin, Point1, and/or Point2 ivars.

// .SECTION Caveats
// The normal to the plane will point in the direction of the cross product
// of the first axis (Origin->Point1) with the second (Origin->Point2). This
// also affects the normals to the generated polygons.

#ifndef __vtkPlaneSource_h
#define __vtkPlaneSource_h

#include "vtkPolySource.hh"

class vtkPlaneSource : public vtkPolySource 
{
public:
  vtkPlaneSource();
  void PrintSelf(ostream& os, vtkIndent indent);
  char *GetClassName() {return "vtkPlaneSource";};

  // Description:
  // Specify the resolution of the plane along the first axes.
  vtkSetMacro(XResolution,int);
  vtkGetMacro(XResolution,int);

  // Description:
  // Specify the resolution of the plane along the second axes.
  vtkSetMacro(YResolution,int);
  vtkGetMacro(YResolution,int);

  // Convenience functions.
  void SetResolution(const int xR, const int yR);
  void GetResolution(int& xR,int& yR) {xR=this->XResolution; yR=this->YResolution;};

  // Description:
  // Specify a point defining the origin of the plane.
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);

  // Description:
  // Specify a point defining the first axis of the plane.
  vtkSetVector3Macro(Point1,float);
  vtkGetVectorMacro(Point1,float,3);

  // Description:
  // Specify a point defining the second axis of the plane.
  vtkSetVector3Macro(Point2,float);
  vtkGetVectorMacro(Point2,float,3);

  // Description:
  // Set/Get the plane normal. Works in conjunction with the plane center to
  // orient the plane.
  void SetNormal(float nx, float ny, float nz);
  void SetNormal(float n[3]);
  vtkGetVectorMacro(Normal,float,3);

  // Description:
  // Set/Get the center of the plane. Works in conjunction with the plane normal
  // to position the plane.
  void SetCenter(float x, float y, float z);
  void SetCenter(float center[3]);
  vtkGetVectorMacro(Center,float,3);

  void Push(float distance);

protected:
  void Execute();

  int XResolution;
  int YResolution;
  float Origin[3];
  float Point1[3];
  float Point2[3];
  float Normal[3];
  float Center[3];

  int UpdatePlane(float v1[3], float v2[3]);
};

#endif


