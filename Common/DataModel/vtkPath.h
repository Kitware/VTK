// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

VTK_ABI_NAMESPACE_BEGIN
class vtkIntArray;

class VTKCOMMONDATAMODEL_EXPORT vtkPath : public vtkPointSet
{
public:
  static vtkPath* New();

  vtkTypeMacro(vtkPath, vtkPointSet);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_PATH; }

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

  ///@{
  /**
   * Insert the next control point in the path.
   */
  void InsertNextPoint(float pts[3], int code);
  void InsertNextPoint(double pts[3], int code);
  void InsertNextPoint(double x, double y, double z, int code);
  ///@}

  ///@{
  /**
   * Set/Get the array of control point codes:
   */
  void SetCodes(vtkIntArray*);
  vtkIntArray* GetCodes();
  ///@}

  /**
   * vtkPath doesn't use cells. These methods return trivial values.
   */
  vtkIdType GetNumberOfCells() override { return 0; }
  using vtkDataSet::GetCell;
  vtkCell* GetCell(vtkIdType) override { return nullptr; }
  void GetCell(vtkIdType, vtkGenericCell*) override;
  int GetCellType(vtkIdType) override { return 0; }

  using vtkDataSet::GetCellPoints;
  /**
   * vtkPath doesn't use cells, this method just clears ptIds.
   */
  void GetCellPoints(vtkIdType, vtkIdList* ptIds) override;

  /**
   * vtkPath doesn't use cells, this method just clears cellIds.
   */
  void GetPointCells(vtkIdType ptId, vtkIdList* cellIds) override;

  /**
   * Return the maximum cell size in this poly data.
   */
  int GetMaxCellSize() override { return 0; }

  ///@{
  /**
   * Get the maximum/minimum spatial dimensionality of the data
   * which is the maximum/minimum dimension of all cells.
   */
  int GetMaxSpatialDimension() override { return 0; }
  int GetMinSpatialDimension() override { return 0; }
  ///@}

  /**
   * Method allocates initial storage for points. Use this method before the
   * method vtkPath::InsertNextPoint().
   */
  void Allocate(vtkIdType size = 1000, int extSize = 1000);

  /**
   * Begin inserting data all over again. Memory is not freed but otherwise
   * objects are returned to their initial state.
   */
  void Reset();

  ///@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkPath* GetData(vtkInformation* info);
  static vtkPath* GetData(vtkInformationVector* v, int i = 0);
  ///@}

protected:
  vtkPath();
  ~vtkPath() override;

private:
  vtkPath(const vtkPath&) = delete;
  void operator=(const vtkPath&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
