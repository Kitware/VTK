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
/**
 * @class   vtkPath
 * @brief   concrete dataset representing a path defined by Bezier
 * curves.
 *
 * vtkPath provides a container for paths composed of line segments,
 * 2nd-order (quadratic) and 3rd-order (cubic) Bezier curves.
*/

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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() VTK_OVERRIDE {return VTK_PATH;}

  /**
   * Enumeration of recognized control point types:
   * - MOVE_TO: Point defining the origin of a new segment, not connected to
   * the previous point.
   * - LINE_TO: Draw a line from the previous point to the current one
   * - CONIC_CURVE: 2nd order (conic/quadratic) point. Must appear
   * in sets of 2, e.g. (0,0) MOVE_TO (0,1) CONIC_CURVE (1,2) CONIC_CURVE
   * defines a quadratic Bezier curve that passes through (0,0) and (1,2)
   * using (0,1) as a control (off) point.
   * - CUBIC_CURVE: 3rd order (cubic) control point. Must appear in sets of
   * 3, e.g. (0,0) MOVE_TO (0,1) CUBIC_CURVE (1,2) CUBIC_CURVE (4,0)
   * CUBIC_CURVE defines a cubic Bezier curve that passes through (0,0)
   * and (4,0), using (0,1) and (1,2) as control (off) points.
   */
  enum ControlPointType
  {
    MOVE_TO = 0,
    LINE_TO,
    CONIC_CURVE,
    CUBIC_CURVE
  };

  //@{
  /**
   * Insert the next control point in the path.
   */
  void InsertNextPoint(float pts[3], int code);
  void InsertNextPoint(double pts[3], int code);
  void InsertNextPoint(double x, double y, double z, int code);
  //@}

  //@{
  /**
   * Set/Get the array of control point codes:
   */
  void SetCodes(vtkIntArray *);
  vtkIntArray *GetCodes();
  //@}

  /**
   * vtkPath doesn't use cells. These methods return trivial values.
   */
  vtkIdType GetNumberOfCells() VTK_OVERRIDE { return 0; }
  vtkCell *GetCell(vtkIdType)  VTK_OVERRIDE { return NULL; }
  void GetCell(vtkIdType, vtkGenericCell *) VTK_OVERRIDE;
  int GetCellType(vtkIdType)   VTK_OVERRIDE { return 0; }

  /**
   * vtkPath doesn't use cells, this method just clears ptIds.
   */
  void GetCellPoints(vtkIdType, vtkIdList *ptIds) VTK_OVERRIDE;

  /**
   * vtkPath doesn't use cells, this method just clears cellIds.
   */
  void GetPointCells(vtkIdType ptId, vtkIdList *cellIds) VTK_OVERRIDE;

  /**
   * Return the maximum cell size in this poly data.
   */
  int GetMaxCellSize() VTK_OVERRIDE { return 0; }

  /**
   * Method allocates initial storage for points. Use this method before the
   * method vtkPath::InsertNextPoint().
   */
  void Allocate(vtkIdType size=1000, int extSize=1000);

  /**
   * Begin inserting data all over again. Memory is not freed but otherwise
   * objects are returned to their initial state.
   */
  void Reset();

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkPath* GetData(vtkInformation* info);
  static vtkPath* GetData(vtkInformationVector* v, int i=0);
  //@}

protected:
  vtkPath();
  ~vtkPath() VTK_OVERRIDE;

private:
  vtkPath(const vtkPath&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPath&) VTK_DELETE_FUNCTION;
};

#endif
