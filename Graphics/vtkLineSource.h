/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLineSource.h
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
// .NAME vtkLineSource - create a line defined by two end points
// .SECTION Description
// vtkLineSource is a source object that creates a polyline defined by
// two endpoints. The number of segments composing the polyline is
// controlled by setting the object resolution.

#ifndef __vtkLineSource_h
#define __vtkLineSource_h

#include "vtkPolyDataSource.h"

class VTK_GRAPHICS_EXPORT vtkLineSource : public vtkPolyDataSource 
{
public:
  static vtkLineSource *New();
  vtkTypeRevisionMacro(vtkLineSource,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set position of first end point.
  vtkSetVector3Macro(Point1,float);
  vtkGetVectorMacro(Point1,float,3);

  // Description:
  // Set position of other end point.
  vtkSetVector3Macro(Point2,float);
  vtkGetVectorMacro(Point2,float,3);

  // Description:
  // Divide line into resolution number of pieces.
  vtkSetClampMacro(Resolution,int,1,VTK_LARGE_INTEGER);
  vtkGetMacro(Resolution,int);

protected:
  vtkLineSource(int res=1);
  ~vtkLineSource() {};

  void Execute();
  float Point1[3];
  float Point2[3];
  int Resolution;
private:
  vtkLineSource(const vtkLineSource&);  // Not implemented.
  void operator=(const vtkLineSource&);  // Not implemented.
};

#endif


