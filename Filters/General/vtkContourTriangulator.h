/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourTriangulator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkContourTriangulator - Fill all 2D contours to create polygons
// .SECTION Description
// vtkContourTriangulator will generate triangles to fill all of the 2D
// contours in its input.  The contours may be concave, and may even
// contain holes i.e. a contour may contain an internal contour that
// is wound in the opposite direction to indicate that it is a hole.
// .SECTION Caveats
// The triangulation of is done in O(n) time for simple convex
// inputs, but for non-convex inputs the worst-case time is O(n^2*m^2)
// where n is the number of points and m is the number of holes.
// The best triangulation algorithms, in contrast, are O(n log n).
// The resulting triangles may be quite narrow, the algorithm does
// not attempt to produce high-quality triangles.
// .SECTION Thanks
// Thanks to David Gobbi for contributing this class to VTK.

#ifndef vtkContourTriangulator_h
#define vtkContourTriangulator_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkCellArray;
class vtkIdList;

class VTKFILTERSGENERAL_EXPORT vtkContourTriangulator : public vtkPolyDataAlgorithm
{
public:
  static vtkContourTriangulator *New();
  vtkTypeMacro(vtkContourTriangulator,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Check if there was a triangulation failure in the last update.
  vtkGetMacro(TriangulationError, int);

  // Description:
  // Generate errors when the triangulation fails.
  // Note that triangulation failures are often minor, because they involve
  // tiny triangles that are too small to see.
  vtkSetMacro(TriangulationErrorDisplay, int);
  vtkBooleanMacro(TriangulationErrorDisplay, int);
  vtkGetMacro(TriangulationErrorDisplay, int);

  // Description:
  // A robust method for triangulating a polygon.
  // It cleans up the polygon and then applies the ear-cut triangulation.
  // A zero return value indicates that triangulation failed.
  static int TriangulatePolygon(
    vtkIdList *polygon, vtkPoints *points, vtkCellArray *triangles);

  // Description:
  // Given some closed contour lines, create a triangle mesh that
  // fills those lines.  The input lines must be single-segment lines,
  // not polylines.  The input lines do not have to be in order.
  // Only numLines starting from firstLine will be used.
  static int TriangulateContours(
    vtkPolyData *data, vtkIdType firstLine, vtkIdType numLines,
    vtkCellArray *outputPolys, const double normal[3]);

protected:
  vtkContourTriangulator();
  ~vtkContourTriangulator();

  virtual int RequestData(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  int TriangulationError;
  int TriangulationErrorDisplay;

private:
  vtkContourTriangulator(const vtkContourTriangulator&);  // Not implemented.
  void operator=(const vtkContourTriangulator&);  // Not implemented.
};

#endif
