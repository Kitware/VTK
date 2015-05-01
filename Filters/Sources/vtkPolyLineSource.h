/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyLineSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolyLineSource - create a poly line from a list of input points
// .SECTION Description
// vtkPolyLineSource is a source object that creates a poly line from
// user-specified points. The output is a vtkPolyLine.

#ifndef vtkPolyLineSource_h
#define vtkPolyLineSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPoints;

class VTKFILTERSSOURCES_EXPORT vtkPolyLineSource : public vtkPolyDataAlgorithm
{
public:
  static vtkPolyLineSource* New();
  vtkTypeMacro(vtkPolyLineSource, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the number of points in the poly line.
  void SetNumberOfPoints(vtkIdType numPoints);
  vtkIdType GetNumberOfPoints();

  // Description:
  // Resize while preserving data.
  void Resize(vtkIdType numPoints);

  // Description:
  // Set a point location.
  void SetPoint(vtkIdType id, double x, double y, double z);

  // Description:
  // Get the points.
  void SetPoints(vtkPoints* points);
  vtkGetObjectMacro(Points, vtkPoints);

  // Description:
  // Set whether to close the poly line by connecting the last and first points.
  vtkSetMacro(Closed, int);
  vtkGetMacro(Closed, int);
  vtkBooleanMacro(Closed, int);

protected:
  vtkPolyLineSource();
  ~vtkPolyLineSource();

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector *);

  vtkPoints* Points;

  int Closed;

private:
  vtkPolyLineSource(const vtkPolyLineSource&);  // Not implemented.
  void operator=(const vtkPolyLineSource&);     // Not implemented.
};

#endif
