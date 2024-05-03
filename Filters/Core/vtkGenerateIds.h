// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGenerateIds
 * @brief   generate scalars or field data from point and cell ids
 *
 * vtkGenerateIds is a filter that generates scalars or field data
 * using cell and point ids. That is, the point attribute data scalars
 * or field data are generated from the point ids, and the cell
 * attribute data scalars or field data are generated from the
 * cell ids.
 *
 * Typically this filter is used with vtkLabeledDataMapper (and possibly
 * vtkSelectVisiblePoints) to create labels for points and cells, or labels
 * for the point or cell data scalar values.
 *
 * This filter supports vtkDataSet and vtkHyperTeeGrid instances as input.
 * In the case of vtkHyperTreeGrid, only cell ids are generated.
 */

#ifndef vtkGenerateIds_h
#define vtkGenerateIds_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

#include <string>

VTK_ABI_NAMESPACE_BEGIN
class vtkCellData;
class vtkPointData;
class VTKFILTERSCORE_EXPORT vtkGenerateIds : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtkGenerateIds, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with PointIds and CellIds on; and ids being generated
   * as scalars.
   */
  static vtkGenerateIds* New();

  ///@{
  /**
   * Enable/disable the generation of point ids. Default is on.
   *
   * @note: Unused if the input is a vtkHyperTreeGrid instance
   * (we do not have point data on HTGs, always off).
   */
  vtkSetMacro(PointIds, bool);
  vtkGetMacro(PointIds, bool);
  vtkBooleanMacro(PointIds, bool);
  ///@}

  ///@{
  /**
   * Enable/disable the generation of cell ids. Default is on.
   *
   * @note: Unused if the input is a vtkHyperTreeGrid instance
   * (we have only cell data on HTGs, always on).
   */
  vtkSetMacro(CellIds, bool);
  vtkGetMacro(CellIds, bool);
  vtkBooleanMacro(CellIds, bool);
  ///@}

  ///@{
  /**
   * Set/Get the flag which controls whether to generate scalar data
   * or field data. If this flag is off, scalar data is generated.
   * Otherwise, field data is generated. Default is off.
   */
  vtkSetMacro(FieldData, bool);
  vtkGetMacro(FieldData, bool);
  vtkBooleanMacro(FieldData, bool);
  ///@}

  ///@{
  /**
   * Set/Get the name of the Ids array for points, if generated. By default,
   * it is set to "vtkPointIds".
   *
   * @note: Unused if the input is a vtkHyperTreeGrid instance.
   */
  vtkSetMacro(PointIdsArrayName, std::string);
  vtkGetMacro(PointIdsArrayName, std::string);
  ///@}

  ///@{
  /**
   * Set/Get the name of the Ids array for points, if generated. By default,
   * it is set to "vtkCellIds".
   */
  vtkSetMacro(CellIdsArrayName, std::string);
  vtkGetMacro(CellIdsArrayName, std::string);
  ///@}

protected:
  vtkGenerateIds() = default;
  ~vtkGenerateIds() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkGenerateIds(const vtkGenerateIds&) = delete;
  void operator=(const vtkGenerateIds&) = delete;

  /**
   * Generate the new data array containing IDs on outputPD.
   */
  void GeneratePointIds(vtkPointData* outputPD, vtkIdType numPts);

  /**
   * Generate the new data array containing IDs on outputCD.
   */
  void GenerateCellIds(vtkCellData* outputCD, vtkIdType numCells);

  bool PointIds = true;
  bool CellIds = true;
  bool FieldData = false;
  std::string PointIdsArrayName = "vtkPointIds";
  std::string CellIdsArrayName = "vtkCellIds";
};

VTK_ABI_NAMESPACE_END
#endif
