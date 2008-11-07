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
// endpoints and a center. The number of segments composing the polyline is
// controlled by setting the object resolution.

#ifndef __vtkArcSource_h
#define __vtkArcSource_h

#include "vtkPolyDataAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkArcSource : public vtkPolyDataAlgorithm 
{
public:
  static vtkArcSource *New();
  vtkTypeRevisionMacro(vtkArcSource,vtkPolyDataAlgorithm);
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
  // Set position of other end point.
  vtkSetVector3Macro(Center,double);
  vtkGetVectorMacro(Center,double,3);
  
  // Description:
  // Divide line into resolution number of pieces.
  vtkSetClampMacro(Resolution,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(Resolution,int);

protected:
  vtkArcSource(int res=1);
  ~vtkArcSource() {};

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  double Point1[3];
  double Point2[3];
  double Center[3];
  int Resolution;

private:
  vtkArcSource(const vtkArcSource&);  // Not implemented.
  void operator=(const vtkArcSource&);  // Not implemented.
};

#endif

