/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClipConvexPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkClipConvexPolyData - clip any dataset with user-specified implicit function or input scalar data
// .SECTION Description
// vtkClipConvexPolyData is a filter that clips a convex polydata with a set
// of planes. Its main usage is for clipping a bounding volume with frustum
// planes (used later one in volume rendering).

#ifndef __vtkClipConvexPolyData_h
#define __vtkClipConvexPolyData_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkPlaneCollection;
class vtkPlane;
class vtkClipConvexPolyDataInternals;

class VTKFILTERSGENERAL_EXPORT vtkClipConvexPolyData : public vtkPolyDataAlgorithm
{
public:
  static vtkClipConvexPolyData *New();
  vtkTypeMacro(vtkClipConvexPolyData,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set all the planes at once using a vtkPlanes implicit function.
  // This also sets the D value.
  void SetPlanes(vtkPlaneCollection *planes);
  vtkGetObjectMacro(Planes,vtkPlaneCollection);

  // Description:
  // Redefines this method, as this filter depends on time of its components
  // (planes)
  virtual unsigned long int GetMTime();

protected:
  vtkClipConvexPolyData();
  ~vtkClipConvexPolyData();

  // The method that does it all...
  int RequestData(vtkInformation *request,
                  vtkInformationVector **inputVector,
                  vtkInformationVector *outputVector);

  // Description:
  // Clip the input with a given plane `p'.
  // tolerance ?
  void ClipWithPlane(vtkPlane *p,
                     double tolerance);

  // Description:
  // Tells if clipping the input by plane `p' creates some degeneracies.
  bool HasDegeneracies(vtkPlane *p);

  // Description:
  // Delete calculation data.
  void ClearInternals();

  // Description:
  // ?
  void ClearNewVertices();

  // Description:
  // ?
  void RemoveEmptyPolygons();

  vtkPlaneCollection *Planes;
  vtkClipConvexPolyDataInternals *Internal;

private:
  vtkClipConvexPolyData(const vtkClipConvexPolyData&);  // Not implemented.
  void operator=(const vtkClipConvexPolyData&);  // Not implemented.
};

#endif
