// -*- c++ -*-

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridVolumeRayCastIterator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkUnstructuredGridVolumeRayCastIterator
//
// .SECTION Description
//
// vtkUnstructuredGridVolumeRayCastIterator is a superclass for iterating
// over the intersections of a viewing ray with a group of unstructured
// cells.  These iterators are created with a
// vtkUnstructuredGridVolumeRayCastFunction.
//
// .SECTION See Also
// vtkUnstructuredGridVolumeRayCastFunction
//

#ifndef vtkUnstructuredGridVolumeRayCastIterator_h
#define vtkUnstructuredGridVolumeRayCastIterator_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkObject.h"

class vtkIdList;
class vtkDoubleArray;
class vtkDataArray;

class VTKRENDERINGVOLUME_EXPORT vtkUnstructuredGridVolumeRayCastIterator : public vtkObject
{
public:
  vtkTypeMacro(vtkUnstructuredGridVolumeRayCastIterator, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Initializes the iteration to the start of the ray at the given screen
  // coordinates.
  virtual void Initialize(int x, int y) = 0;

  // Description:
  // Get the intersections of the next several cells.  The cell ids are
  // stored in \c intersectedCells and the length of each ray segment
  // within the cell is stored in \c intersectionLengths.  The point
  // scalars \c scalars are interpolated and stored in \c nearIntersections
  // and \c farIntersections.  \c intersectedCells, \c intersectionLengths,
  // or \c scalars may be \c NULL to suppress passing the associated
  // information.  The number of intersections actually encountered is
  // returned.  0 is returned if and only if no more intersections are to
  // be found.
  virtual vtkIdType GetNextIntersections(vtkIdList *intersectedCells,
                                         vtkDoubleArray *intersectionLengths,
                                         vtkDataArray *scalars,
                                         vtkDataArray *nearIntersections,
                                         vtkDataArray *farIntersections) = 0;

  // Description:
  // Set/get the bounds of the cast ray (in viewing coordinates).  By
  // default the range is [0,1].
  vtkSetVector2Macro(Bounds, double);
  vtkGetVector2Macro(Bounds, double);

  // Descrption:
  // Set/get the maximum number of intersections returned with a call to
  // GetNextIntersections.  Set to 32 by default.
  vtkSetMacro(MaxNumberOfIntersections, vtkIdType);
  vtkGetMacro(MaxNumberOfIntersections, vtkIdType);

protected:
  vtkUnstructuredGridVolumeRayCastIterator();
  ~vtkUnstructuredGridVolumeRayCastIterator();

  double Bounds[2];

  vtkIdType MaxNumberOfIntersections;

private:
  vtkUnstructuredGridVolumeRayCastIterator(const vtkUnstructuredGridVolumeRayCastIterator&);  // Not implemented.
  void operator=(const vtkUnstructuredGridVolumeRayCastIterator&);  // Not implemented.
};

#endif //vtkUnstructuredGridRayCastIterator_h

