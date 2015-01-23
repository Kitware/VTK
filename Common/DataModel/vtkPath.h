/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPath.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPath - concrete dataset representing a path defined by Bezier
// curves.
// .SECTION Description
// vtkPath provides a container for paths composed of line segment and
// 2nd/3rd order Bezier curves.

#ifndef vtkPath_h
#define vtkPath_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkPointSet.h"

class vtkIntArray;

class VTKCOMMONDATAMODEL_EXPORT vtkPath : public vtkPointSet
{
public:
  static vtkPath *New();

  vtkTypeMacro(vtkPath,vtkPointSet);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Return what type of dataset this is.
  int GetDataObjectType() {return VTK_PATH;}

  // Description:
  // Enumeration of recognized control point types:
  // - MOVE_TO: Point defining the origin of a new segment, not connected to
  //   the previous point.
  // - LINE_TO: Draw a line from the previous point to the current one
  // - CONIC_CURVE: 2nd order (conic/quadratic) point. Must appear
  //   in sets of 2, e.g. (0,0) MOVE_TO (0,1) CONIC_CURVE (1,2) CONIC_CURVE
  //   defines a quadratic Bezier curve that passes through (0,0) and (1,2)
  //   using (0,1) as a control (off) point.
  // - CUBIC_CURVE: 3rd order (cubic) control point. Must appear in sets of
  //   3, e.g. (0,0) MOVE_TO (0,1) CUBIC_CURVE (1,2) CUBIC_CURVE (4,0)
  //   CUBIC_CURVE defines a cubic Bezier curve that passes through (0,0)
  //   and (4,0), using (0,1) and (1,2) as control (off) points.
  enum ControlPointType
    {
    MOVE_TO = 0,
    LINE_TO,
    CONIC_CURVE,
    CUBIC_CURVE
    };

  // Description:
  // Insert the next control point in the path.
  void InsertNextPoint(float pts[3], int code);
  void InsertNextPoint(double pts[3], int code);
  void InsertNextPoint(double x, double y, double z, int code);

  // Description:
  // Set/Get the array of control point codes:
  void SetCodes(vtkIntArray *);
  vtkIntArray *GetCodes();

  // Description:
  //vtkPath doesn't use cells. These methods return trivial values.
  vtkIdType GetNumberOfCells() { return 0; }
  vtkCell *GetCell(vtkIdType) { return NULL; }
  void GetCell(vtkIdType, vtkGenericCell *);
  int GetCellType(vtkIdType) { return 0; }

  // Description:
  // vtkPath doesn't use cells, this method just clears ptIds.
  void GetCellPoints(vtkIdType, vtkIdList *ptIds);

  // Description:
  // vtkPath doesn't use cells, this method just clears cellIds.
  void GetPointCells(vtkIdType ptId, vtkIdList *cellIds);

  // Description:
  // Return the maximum cell size in this poly data.
  int GetMaxCellSize() { return 0; }

  // Description:
  // Method allocates initial storage for points. Use this method before the
  // method vtkPath::InsertNextPoint().
  void Allocate(vtkIdType size=1000, int extSize=1000);

  // Description:
  // Begin inserting data all over again. Memory is not freed but otherwise
  // objects are returned to their initial state.
  void Reset();

  // Description:
  // Retrieve an instance of this class from an information object.
  static vtkPath* GetData(vtkInformation* info);
  static vtkPath* GetData(vtkInformationVector* v, int i=0);

protected:
  vtkPath();
  ~vtkPath();

private:
  vtkPath(const vtkPath&);  // Not implemented.
  void operator=(const vtkPath&);  // Not implemented.
};

#endif
