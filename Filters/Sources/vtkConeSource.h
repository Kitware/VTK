/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConeSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkConeSource - generate polygonal cone
// .SECTION Description
// vtkConeSource creates a cone centered at a specified point and pointing in
// a specified direction. (By default, the center is the origin and the
// direction is the x-axis.) Depending upon the resolution of this object,
// different representations are created. If resolution=0 a line is created;
// if resolution=1, a single triangle is created; if resolution=2, two
// crossed triangles are created. For resolution > 2, a 3D cone (with
// resolution number of sides) is created. It also is possible to control
// whether the bottom of the cone is capped with a (resolution-sided)
// polygon, and to specify the height and radius of the cone.

#ifndef vtkConeSource_h
#define vtkConeSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkCell.h" // Needed for VTK_CELL_SIZE

class VTKFILTERSSOURCES_EXPORT vtkConeSource : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkConeSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with default resolution 6, height 1.0, radius 0.5, and
  // capping on. The cone is centered at the origin and points down
  // the x-axis.
  static vtkConeSource *New();

  // Description:
  // Set the height of the cone. This is the height along the cone in
  // its specified direction.
  vtkSetClampMacro(Height,double,0.0,VTK_DOUBLE_MAX)
  vtkGetMacro(Height,double);

  // Description:
  // Set the base radius of the cone.
  vtkSetClampMacro(Radius,double,0.0,VTK_DOUBLE_MAX)
  vtkGetMacro(Radius,double);

  // Description:
  // Set the number of facets used to represent the cone.
  vtkSetClampMacro(Resolution,int,0,VTK_CELL_SIZE)
  vtkGetMacro(Resolution,int);

  // Description:
  // Set the center of the cone. It is located at the middle of the axis of
  // the cone. Warning: this is not the center of the base of the cone!
  // The default is 0,0,0.
  vtkSetVector3Macro(Center,double);
  vtkGetVectorMacro(Center,double,3);

  // Description:
  // Set the orientation vector of the cone. The vector does not have
  // to be normalized. The direction goes from the center of the base toward
  // the apex. The default is (1,0,0).
  vtkSetVector3Macro(Direction,double);
  vtkGetVectorMacro(Direction,double,3);

  // Description:
  // Set the angle of the cone. This is the angle between the axis of the cone
  // and a generatrix. Warning: this is not the aperture! The aperture is
  // twice this angle.
  // As a side effect, the angle plus height sets the base radius of the cone.
  // Angle is expressed in degrees.
  void SetAngle (double angle);
  double GetAngle ();

  // Description:
  // Turn on/off whether to cap the base of the cone with a polygon.
  vtkSetMacro(Capping,int);
  vtkGetMacro(Capping,int);
  vtkBooleanMacro(Capping,int);

  // Description:
  // Set/get the desired precision for the output points.
  // vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
  // vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);

protected:
  vtkConeSource(int res=6);
  ~vtkConeSource() {}

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  double Height;
  double Radius;
  int Resolution;
  int Capping;
  double Center[3];
  double Direction[3];
  int OutputPointsPrecision;

private:
  vtkConeSource(const vtkConeSource&);  // Not implemented.
  void operator=(const vtkConeSource&);  // Not implemented.
};

#endif


