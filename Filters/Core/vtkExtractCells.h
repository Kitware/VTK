// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkExtractCells
 * @brief   subset a vtkDataSet to create a vtkUnstructuredGrid
 *
 * Given a vtkDataSet and a list of cell ids, create a vtkUnstructuredGrid
 * composed of these cells.  If the cell list is empty when vtkExtractCells
 * executes, it will set up the ugrid, point and cell arrays, with no points,
 * cells or data.
 *
 * @warning
 * This class is templated. It may run slower than serial execution if the code
 * is not optimized during compilation. Build in Release or ReleaseWithDebugInfo.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 */

#ifndef vtkExtractCells_h
#define vtkExtractCells_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkSmartPointer.h"      // For vtkSmartPointer
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkIdList;
class vtkExtractCellsIdList;

class VTKFILTERSCORE_EXPORT vtkExtractCells : public vtkUnstructuredGridAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for construction, type info, and printing.
   */
  vtkTypeMacro(vtkExtractCells, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkExtractCells* New();
  ///@}

  /**
   * Set the list of cell IDs that the output vtkUnstructuredGrid will be
   * composed of.  Replaces any other cell ID list supplied so far.  (Set to
   * nullptr to free memory used by cell list.)  The cell ids should be >=0.
   */
  void SetCellList(vtkIdList* l);

  /**
   * Add the supplied list of cell IDs to those that will be included in the
   * output vtkUnstructuredGrid. The cell ids should be >=0.
   */
  void AddCellList(vtkIdList* l);

  /**
   * Add this range of cell IDs to those that will be included in the output
   * vtkUnstructuredGrid. Note that (from < to), and (from >= 0).
   */
  void AddCellRange(vtkIdType from, vtkIdType to);

  ///@{
  /**
   * Another way to provide ids using a pointer to vtkIdType array.
   */
  void SetCellIds(const vtkIdType* ptr, vtkIdType numValues);
  void AddCellIds(const vtkIdType* ptr, vtkIdType numValues);
  ///@}

  ///@{
  /**
   * If all cells are being extracted, this filter can use fast path to speed up
   * the extraction. In that case, one can set this flag to true. When set to
   * true, cell ids added via the various methods are simply ignored.
   * Defaults to false.
   */
  vtkSetMacro(ExtractAllCells, bool);
  vtkGetMacro(ExtractAllCells, bool);
  vtkBooleanMacro(ExtractAllCells, bool);
  ///@}

  ///@{
  /**
   * If the cell ids specified are already sorted and unique, then set this to
   * true to avoid the filter from doing time-consuming sorts and uniquification
   * operations. Defaults to false.
   */
  vtkSetMacro(AssumeSortedAndUniqueIds, bool);
  vtkGetMacro(AssumeSortedAndUniqueIds, bool);
  vtkBooleanMacro(AssumeSortedAndUniqueIds, bool);
  ///@}

  ///@{
  /**
   * If on, the output dataset will have a celldata array that
   * holds the cell index of the original 3D cell that produced each output
   * cell. The default is on
   */
  vtkSetMacro(PassThroughCellIds, bool);
  vtkGetMacro(PassThroughCellIds, bool);
  vtkBooleanMacro(PassThroughCellIds, bool);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

  ///@{
  /**
   * Specify the number of input cells in a batch, where a batch defines
   * a subset of the input cells operated on during threaded
   * execution. Generally this is only used for debugging or performance
   * studies (since batch size affects the thread workload).
   *
   * Default is 1000.
   */
  vtkSetClampMacro(BatchSize, unsigned int, 1, VTK_INT_MAX);
  vtkGetMacro(BatchSize, unsigned int);
  ///@}
protected:
  vtkExtractCells();
  ~vtkExtractCells() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;
  bool Copy(vtkDataSet* input, vtkUnstructuredGrid* output);

  vtkSmartPointer<vtkExtractCellsIdList> CellList;
  bool ExtractAllCells = false;
  bool AssumeSortedAndUniqueIds = false;
  bool PassThroughCellIds = true;
  int OutputPointsPrecision = DEFAULT_PRECISION;
  unsigned int BatchSize = 1000;

private:
  vtkExtractCells(const vtkExtractCells&) = delete;
  void operator=(const vtkExtractCells&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
