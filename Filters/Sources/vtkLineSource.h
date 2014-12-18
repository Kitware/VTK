/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkLineSource - create a line defined by two end points
// .SECTION Description
// vtkLineSource is a source object that creates a polyline defined by
// two endpoints. The number of segments composing the polyline is
// controlled by setting the object resolution.
//
// .SECTION Thanks
// This class was extended by Philippe Pebay, Kitware SAS 2011, to support
// broken lines as well as simple lines.

#ifndef vtkLineSource_h
#define vtkLineSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPoints;

class VTKFILTERSSOURCES_EXPORT vtkLineSource : public vtkPolyDataAlgorithm
{
public:
  static vtkLineSource *New();
  vtkTypeMacro(vtkLineSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set position of first end point.
  vtkSetVector3Macro(Point1,double);
  vtkGetVectorMacro(Point1,double,3);
  void SetPoint1(float[3]);

  // Description:
  // Set position of other end point.
  vtkSetVector3Macro(Point2,double);
  vtkGetVectorMacro(Point2,double,3);
  void SetPoint2(float[3]);

  // Description:
  // Set/Get the list of points defining a broken line
  virtual void SetPoints(vtkPoints*);
  vtkGetObjectMacro(Points,vtkPoints);

  // Description:
  // Divide line into Resolution number of pieces.
  vtkSetClampMacro(Resolution,int,1,VTK_INT_MAX);
  vtkGetMacro(Resolution,int);

  // Description:
  // Set/get the desired precision for the output points.
  // vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
  // vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);

protected:
  vtkLineSource(int res=1);
  virtual ~vtkLineSource();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  double Point1[3];
  double Point2[3];
  int Resolution;
  int OutputPointsPrecision;

  // Description:
  // The list of points defining a broken line
  // NB: The Point1/Point2 definition of a single line segment is used by default
  vtkPoints* Points;

private:
  vtkLineSource(const vtkLineSource&);  // Not implemented.
  void operator=(const vtkLineSource&);  // Not implemented.
};

#endif
