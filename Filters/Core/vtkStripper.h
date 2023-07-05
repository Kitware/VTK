// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStripper
 * @brief   create triangle strips and/or poly-lines
 *
 *
 * vtkStripper is a filter that generates triangle strips and/or poly-lines
 * from input polygons, triangle strips, and lines. Input polygons are
 * assembled into triangle strips only if they are triangles; other types of
 * polygons are passed through to the output and not stripped. (Use
 * vtkTriangleFilter to triangulate non-triangular polygons prior to running
 * this filter if you need to strip all the data.) The filter will pass
 * through (to the output) vertices if they are present in the input
 * polydata. Also note that if triangle strips or polylines are defined in
 * the input they are passed through and not joined nor extended. (If you wish
 * to strip these use vtkTriangleFilter to fragment the input into triangles
 * and lines prior to running vtkStripper.)
 *
 * The ivar MaximumLength can be used to control the maximum
 * allowable triangle strip and poly-line length.
 *
 * By default, this filter discards any cell data associated with the input.
 * Thus is because the cell structure changes and and the old cell data
 * is no longer valid. When PassCellDataAsFieldData flag is set,
 * the cell data is passed as FieldData to the output using the following rule:
 * 1) for every cell in the output that is not a triangle strip,
 *    the cell data is inserted once per cell in the output field data.
 * 2) for every triangle strip cell in the output:
 *    ii) 1 tuple is inserted for every point(j|j>=2) in the strip.
 *    This is the cell data for the cell formed by (j-2, j-1, j) in
 *    the input.
 * The field data order is same as cell data i.e. (verts,line,polys,tsrips).
 *
 * If there is a ghost cell array in the input, the ghost array is discarded.
 * Any cell tagged as ghost is skipped when stripping. Ghost points are kept.
 *
 * @warning
 * If triangle strips or poly-lines exist in the input data they will
 * be passed through to the output data. This filter will only construct
 * triangle strips if triangle polygons are available; and will only
 * construct poly-lines if lines are available.
 *
 * @sa
 * vtkTriangleFilter
 */

#ifndef vtkStripper_h
#define vtkStripper_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkStripper : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkStripper, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with MaximumLength set to 1000.
   */
  static vtkStripper* New();

  ///@{
  /**
   * Specify the maximum number of triangles in a triangle strip,
   * and/or the maximum number of lines in a poly-line.
   */
  vtkSetClampMacro(MaximumLength, int, 4, 100000);
  vtkGetMacro(MaximumLength, int);
  ///@}

  ///@{
  /**
   * Enable/Disable passing of the CellData in the input to
   * the output as FieldData. Note the field data is transformed.
   */
  vtkBooleanMacro(PassCellDataAsFieldData, vtkTypeBool);
  vtkSetMacro(PassCellDataAsFieldData, vtkTypeBool);
  vtkGetMacro(PassCellDataAsFieldData, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If on, the output polygonal dataset will have a celldata array that
   * holds the cell index of the original 3D cell that produced each output
   * cell. This is useful for picking. The default is off to conserve
   * memory.
   */
  vtkSetMacro(PassThroughCellIds, vtkTypeBool);
  vtkGetMacro(PassThroughCellIds, vtkTypeBool);
  vtkBooleanMacro(PassThroughCellIds, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If on, the output polygonal dataset will have a pointdata array that
   * holds the point index of the original vertex that produced each output
   * vertex. This is useful for picking. The default is off to conserve
   * memory.
   */
  vtkSetMacro(PassThroughPointIds, vtkTypeBool);
  vtkGetMacro(PassThroughPointIds, vtkTypeBool);
  vtkBooleanMacro(PassThroughPointIds, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If on, the output polygonal segments will be joined if they are
   * contiguous. This is useful after slicing a surface. The default
   * is off.
   */
  vtkSetMacro(JoinContiguousSegments, vtkTypeBool);
  vtkGetMacro(JoinContiguousSegments, vtkTypeBool);
  vtkBooleanMacro(JoinContiguousSegments, vtkTypeBool);
  ///@}

protected:
  vtkStripper();
  ~vtkStripper() override = default;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int MaximumLength;
  vtkTypeBool PassCellDataAsFieldData;
  vtkTypeBool PassThroughCellIds;
  vtkTypeBool PassThroughPointIds;
  vtkTypeBool JoinContiguousSegments;

private:
  vtkStripper(const vtkStripper&) = delete;
  void operator=(const vtkStripper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
