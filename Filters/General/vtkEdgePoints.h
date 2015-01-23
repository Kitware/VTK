/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEdgePoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkEdgePoints - generate points on isosurface
// .SECTION Description
// vtkEdgePoints is a filter that takes as input any dataset and
// generates for output a set of points that lie on an isosurface. The
// points are created by interpolation along cells edges whose end-points are
// below and above the contour value.
// .SECTION Caveats
// vtkEdgePoints can be considered a "poor man's" dividing cubes algorithm
// (see vtkDividingCubes). Points are generated only on the edges of cells,
// not in the interior, and at lower density than dividing cubes. However, it
// is more general than dividing cubes since it treats any type of dataset.

#ifndef vtkEdgePoints_h
#define vtkEdgePoints_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkMergePoints;

class VTKFILTERSGENERAL_EXPORT vtkEdgePoints : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkEdgePoints,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with contour value of 0.0.
  static vtkEdgePoints *New();

  // Description:
  // Set/get the contour value.
  vtkSetMacro(Value,double);
  vtkGetMacro(Value,double);

protected:
  vtkEdgePoints();
  ~vtkEdgePoints();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  double Value;
  vtkMergePoints *Locator;
private:
  vtkEdgePoints(const vtkEdgePoints&);  // Not implemented.
  void operator=(const vtkEdgePoints&);  // Not implemented.
};

#endif
