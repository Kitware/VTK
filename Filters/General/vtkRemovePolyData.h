// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRemovePolyData
 * @brief   Removes vtkPolyData cells from an input vtkPolyData
 *
 * vtkRemovePolyData is a filter that removes cells from an input vtkPolyData
 * (defined in the first input #0), and produces an output vtkPolyData (which
 * may be empty).  The cells to remove are specified in the following ways:
 * 1) a list of cell ids can be provided; 2) a list of point ids can be
 * provided - any cell using one or more of the points indicated is removed;
 * and 3) one or more additional vtkPolyData inputs can be provided -
 * matching cells are deleted. These three methods can be used in combination
 * if desired. Point and cell attribute data associated with the remaining
 * cells are copied to the output.
 *
 * @warning
 * The filter vtkAppendPolyData enables appending multiple input
 * vtkPolyData's together. So vtkAppendPolyData functions as an
 * approximate inverse operation to vtkRemovePolyData.
 *
 * @warning
 * The output point type is the same as the (first) input point type.
 *
 * @sa
 * vtkAppendPolyData
 */

#ifndef vtkRemovePolyData_h
#define vtkRemovePolyData_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkDataArray;
class vtkPoints;
class vtkPolyData;
class vtkIdTypeArray;

class VTKFILTERSGENERAL_EXPORT vtkRemovePolyData : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods to instantiate, obtain information, and print information.
   *
   */
  static vtkRemovePolyData* New();
  vtkTypeMacro(vtkRemovePolyData, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Remove a vtkPolyData dataset from the list of data.
   */
  void RemoveInputData(vtkPolyData*);

  ///@{
  /**
   * Get any input of this filter.
   */
  vtkPolyData* GetInput(int idx);
  vtkPolyData* GetInput() { return this->GetInput(0); }
  ///@}

  ///@{
  /**
   * Set/Get the list of cell ids to delete. These are cell ids of the input
   * polydata - note that polydata with mixed cell types (e.g., verts, lines,
   * polys, and/or strips), the cell ids begin with the vertex cells, then
   * line cells, then polygonal cells, and finally triangle strips.
   */
  void SetCellIds(vtkIdTypeArray*);
  vtkGetObjectMacro(CellIds, vtkIdTypeArray);
  ///@}

  ///@{
  /**
   * Set/Get the list of points ids to delete. Any cells using any of the
   * points listed are removed.
   */
  void SetPointIds(vtkIdTypeArray*);
  vtkGetObjectMacro(PointIds, vtkIdTypeArray);
  ///@}

  ///@{
  /**
   * ExactMatch controls how the matching of cells when additional input
   * vtkPolyDatas are provided. When ExactMatch is enabled, then if any
   * input0 cell Ci uses all of the point ids in cells specified in inputs
   * [1,N) Cn, and the number of point ids in Ci == Cn, then a match occurs
   * and the cell is marked for deletion. Without ExactMatch enabled, if Ci
   * uses all of the points in Cn, even though the cell connectivity list
   * sizes are not the same size, a match occurs. This can be used to perform
   * tricks like marking all of the cells that use a point or edge to be
   * deleted. ExactMatch is disabled by default since it takes a extra
   * computation time.
   */
  vtkSetMacro(ExactMatch, bool);
  vtkGetMacro(ExactMatch, bool);
  vtkBooleanMacro(ExactMatch, bool);
  ///@}

protected:
  vtkRemovePolyData();
  ~vtkRemovePolyData() override;

  // Usual data generation related methods
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkIdTypeArray* CellIds;
  vtkIdTypeArray* PointIds;
  bool ExactMatch;

  vtkRemovePolyData(const vtkRemovePolyData&) = delete;
  void operator=(const vtkRemovePolyData&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
