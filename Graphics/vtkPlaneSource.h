/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlaneSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPlaneSource - create an array of quadrilaterals located in a plane
// .SECTION Description
// vtkPlaneSource creates an m x n array of quadrilaterals arranged as
// a regular tiling in a plane. The plane is defined by specifying an
// origin point, and then two other points that, together with the
// origin, define two axes for the plane. These axes do not have to be
// orthogonal - so you can create a parallelogram. (The axes must not
// be parallel.) By default, the plane is centered at the origin and
// perpendicular to the z-axis, with width and height of length 1. The
// resolution of the plane (i.e., number of subdivisions) is
// controlled by the ivars XResolution and YResolution.
//
// There are three convenience methods that allow you to easily move
// the plane.  The first, SetNormal(), allows you to specify the plane
// normal. The effect of this method is to rotate the plane around the
// center of the plane, aligning the plane normal with the specified
// normal. The second, SetCenter(), translates the center of the plane
// to the specified center point. The third method, Push(), allows you
// to translate the plane along the plane normal by the distance
// specified. (Negative Push values translate the plane in the
// negative normal direction.)  Note that the SetNormal(), SetCenter()
// and Push() methods modify the Origin, Point1, and/or Point2 ivars.

// .SECTION Caveats
// The normal to the plane will point in the direction of the cross product
// of the first axis (Origin->Point1) with the second (Origin->Point2). This
// also affects the normals to the generated polygons.

#ifndef __vtkPlaneSource_h
#define __vtkPlaneSource_h

#include "vtkPolyDataSource.h"

class VTK_GRAPHICS_EXPORT vtkPlaneSource : public vtkPolyDataSource 
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeRevisionMacro(vtkPlaneSource,vtkPolyDataSource);

  // Description:
  // Construct plane perpendicular to z-axis, resolution 1x1, width
  // and height 1.0, and centered at the origin.
  static vtkPlaneSource *New();

  // Description:
  // Specify the resolution of the plane along the first axes.
  vtkSetMacro(XResolution,int);
  vtkGetMacro(XResolution,int);

  // Description:
  // Specify the resolution of the plane along the second axes.
  vtkSetMacro(YResolution,int);
  vtkGetMacro(YResolution,int);

  // Description:
  // Set the number of x-y subdivisions in the plane.
  void SetResolution(const int xR, const int yR);
  void GetResolution(int& xR,int& yR) {
    xR=this->XResolution; yR=this->YResolution;};

  // Description:
  // Specify a point defining the origin of the plane.
  vtkSetVector3Macro(Origin,float);
  vtkGetVectorMacro(Origin,float,3);

  // Description:
  // Specify a point defining the first axis of the plane.
  void SetPoint1(float x, float y, float z);
  void SetPoint1(float pnt[3]);
  vtkGetVectorMacro(Point1,float,3);

  // Description:
  // Specify a point defining the second axis of the plane.
  void SetPoint2(float x, float y, float z);
  void SetPoint2(float pnt[3]);
  vtkGetVectorMacro(Point2,float,3);

  // Description:
  // Set/Get the center of the plane. Works in conjunction with the plane 
  // normal to position the plane. Don't use this method to define the plane.
  // Instead, use it to move the plane to a new center point.
  void SetCenter(float x, float y, float z);
  void SetCenter(float center[3]);
  vtkGetVectorMacro(Center,float,3);

  // Description:
  // Set/Get the plane normal. Works in conjunction with the plane center to
  // orient the plane. Don't use this method to define the plane. Instead, use
  // it to rotate the plane around the current center point.
  void SetNormal(float nx, float ny, float nz);
  void SetNormal(float n[3]);
  vtkGetVectorMacro(Normal,float,3);

  // Description:
  // Translate the plane in the direction of the normal by the
  // distance specified.  Negative values move the plane in the
  // opposite direction.
  void Push(float distance);

protected:
  vtkPlaneSource();
  ~vtkPlaneSource() {};

  void Execute();

  int XResolution;
  int YResolution;
  float Origin[3];
  float Point1[3];
  float Point2[3];
  float Normal[3];
  float Center[3];

  int UpdatePlane(float v1[3], float v2[3]);
private:
  vtkPlaneSource(const vtkPlaneSource&);  // Not implemented.
  void operator=(const vtkPlaneSource&);  // Not implemented.
};

#endif


