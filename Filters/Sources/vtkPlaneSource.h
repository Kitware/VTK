/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlaneSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
// be parallel.) The resolution of the plane (i.e., number of subdivisions) is
// controlled by the ivars XResolution and YResolution.
//
// By default, the plane is centered at the origin and perpendicular to the
// z-axis, with width and height of length 1 and resolutions set to 1.
//
// There are three convenience methods that allow you to easily move the
// plane.  The first, SetNormal(), allows you to specify the plane
// normal. The effect of this method is to rotate the plane around the center
// of the plane, aligning the plane normal with the specified normal. The
// rotation is about the axis defined by the cross product of the current
// normal with the new normal. The second, SetCenter(), translates the center
// of the plane to the specified center point. The third method, Push(),
// allows you to translate the plane along the plane normal by the distance
// specified. (Negative Push values translate the plane in the negative
// normal direction.)  Note that the SetNormal(), SetCenter() and Push()
// methods modify the Origin, Point1, and/or Point2 instance variables.

// .SECTION Caveats
// The normal to the plane will point in the direction of the cross product
// of the first axis (Origin->Point1) with the second (Origin->Point2). This
// also affects the normals to the generated polygons.

#ifndef vtkPlaneSource_h
#define vtkPlaneSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSSOURCES_EXPORT vtkPlaneSource : public vtkPolyDataAlgorithm
{
public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkPlaneSource,vtkPolyDataAlgorithm);

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
  vtkSetVector3Macro(Origin,double);
  vtkGetVectorMacro(Origin,double,3);

  // Description:
  // Specify a point defining the first axis of the plane.
  void SetPoint1(double x, double y, double z);
  void SetPoint1(double pnt[3]);
  vtkGetVectorMacro(Point1,double,3);

  // Description:
  // Specify a point defining the second axis of the plane.
  void SetPoint2(double x, double y, double z);
  void SetPoint2(double pnt[3]);
  vtkGetVectorMacro(Point2,double,3);

  // Description:
  // Set/Get the center of the plane. Works in conjunction with the plane
  // normal to position the plane. Don't use this method to define the plane.
  // Instead, use it to move the plane to a new center point.
  void SetCenter(double x, double y, double z);
  void SetCenter(double center[3]);
  vtkGetVectorMacro(Center,double,3);

  // Description:
  // Set/Get the plane normal. Works in conjunction with the plane center to
  // orient the plane. Don't use this method to define the plane. Instead, use
  // it to rotate the plane around the current center point.
  void SetNormal(double nx, double ny, double nz);
  void SetNormal(double n[3]);
  vtkGetVectorMacro(Normal,double,3);

  // Description:
  // Translate the plane in the direction of the normal by the
  // distance specified.  Negative values move the plane in the
  // opposite direction.
  void Push(double distance);

  // Description:
  // Set/get the desired precision for the output points.
  // vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
  // vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);

protected:
  vtkPlaneSource();
  ~vtkPlaneSource() {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int XResolution;
  int YResolution;
  double Origin[3];
  double Point1[3];
  double Point2[3];
  double Normal[3];
  double Center[3];
  int OutputPointsPrecision;

  int UpdatePlane(double v1[3], double v2[3]);
private:
  vtkPlaneSource(const vtkPlaneSource&);  // Not implemented.
  void operator=(const vtkPlaneSource&);  // Not implemented.
};

#endif
