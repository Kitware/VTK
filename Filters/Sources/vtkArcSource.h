/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArcSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkArcSource - create an arc between two end points
// .SECTION Description
// vtkArcSource is a source object that creates an arc defined by two
// endpoints and a center. The number of segments composing the polyline 
// is controlled by setting the object resolution.
// Alternatively, one can use a better API (that does not allow for 
// inconsistent nor ambiguous inputs), using a starting point, a normal,
// and an angle. The default API being the original one, in order to use
// the improved API, one must switch the UseNormalAndAngle flag to TRUE.

// The development of an improved, consistent API (based on point, normal,
// and angle) was supported by CEA/DIF - Commissariat a l'Energie Atomique, 
// Centre DAM Ile-De-France, BP12, F-91297 Arpajon, France, and implemented
// by Philippe Pebay, Kitware SAS 2012.

#ifndef __vtkArcSource_h
#define __vtkArcSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSSOURCES_EXPORT vtkArcSource : public vtkPolyDataAlgorithm
{
public:
  static vtkArcSource *New();
  vtkTypeMacro(vtkArcSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set position of first end point.
  vtkSetVector3Macro(Point1,double);
  vtkGetVectorMacro(Point1,double,3);

  // Description:
  // Set position of other end point.
  vtkSetVector3Macro(Point2,double);
  vtkGetVectorMacro(Point2,double,3);

  // Description:
  // Set position of the center of the circle that define the arc.
  // Note: you can use the function vtkMath::Solve3PointCircle to
  // find the center from 3 points located on a circle.
  vtkSetVector3Macro(Center,double);
  vtkGetVectorMacro(Center,double,3);

  // Description:
  // Set normal vector.
  // Note: This is only used when UseNormalAndRadius is ON.
  vtkSetVector3Macro(Normal,double);
  vtkGetVectorMacro(Normal,double,3);

  // Description:
  // Set polar vector.
  // Note: This is only used when UseNormalAndRadius is ON.
  vtkSetVector3Macro(PolarVector,double);
  vtkGetVectorMacro(PolarVector,double,3);

  // Description:
  // Angular sector occupied by the arc, beginning at Point1.
  // Note: This is only used when UseNormalAndRadius is ON.
  vtkSetClampMacro(Angle,double,-360.0,360.0);
  vtkGetMacro(Angle,double);

  // Description:
  // Divide line into resolution number of pieces.
  // Note: if Resolution is set to 1 (default), the arc is a
  // straight line.
  vtkSetClampMacro(Resolution,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(Resolution,int);

  // Description:
  // Use the angle that is a negative coterminal of the vectors angle:
  // the longest angle.
  // Note: false by default.
  vtkSetMacro(Negative, bool);
  vtkGetMacro(Negative, bool);
  vtkBooleanMacro(Negative, bool);

  // Description:
  // Activate the API based on normal and radius.
  // The previous API (which remains the default) allows for inconsistent
  // (when Point1 and Point2 are not equidistant from Center) or 
  // ambiguous (when Point1, Point2, and Center are aligned).
  // Note: false by default.
  vtkSetMacro(UseNormalAndAngle, bool);
  vtkGetMacro(UseNormalAndAngle, bool);
  vtkBooleanMacro(UseNormalAndAngle, bool);

protected:
  vtkArcSource(int res=1);
  ~vtkArcSource() {};

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  double Point1[3];
  double Point2[3];
  double Center[3];
  double Normal[3];
  double PolarVector[3];
  double Angle;
  int Resolution;
  bool Negative;
  bool UseNormalAndAngle;

private:
  vtkArcSource(const vtkArcSource&);  // Not implemented.
  void operator=(const vtkArcSource&);  // Not implemented.
};

#endif

