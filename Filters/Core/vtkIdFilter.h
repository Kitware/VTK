// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkIdFilter
 * @brief   generate scalars or field data from point and cell ids
 *
 * vtkIdFilter is a filter to that generates scalars or field data
 * using cell and point ids. That is, the point attribute data scalars
 * or field data are generated from the point ids, and the cell
 * attribute data scalars or field data are generated from the
 * cell ids.
 *
 * Typically this filter is used with vtkLabeledDataMapper (and possibly
 * vtkSelectVisiblePoints) to create labels for points and cells, or labels
 * for the point or cell data scalar values.
 */

#ifndef vtkIdFilter_h
#define vtkIdFilter_h

#include "vtkDataSetAlgorithm.h"
#include "vtkDeprecation.h"       // For VTK_DEPRECATED_IN_9_4_0
#include "vtkFiltersCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTK_DEPRECATED_IN_9_4_0(
  "Please use `vtkGenerateIds` instead.") VTKFILTERSCORE_EXPORT vtkIdFilter
  : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkIdFilter, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with PointIds and CellIds on; and ids being generated
   * as scalars.
   */
  static vtkIdFilter* New();

  ///@{
  /**
   * Enable/disable the generation of point ids. Default is on.
   */
  vtkSetMacro(PointIds, vtkTypeBool);
  vtkGetMacro(PointIds, vtkTypeBool);
  vtkBooleanMacro(PointIds, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable/disable the generation of point ids. Default is on.
   */
  vtkSetMacro(CellIds, vtkTypeBool);
  vtkGetMacro(CellIds, vtkTypeBool);
  vtkBooleanMacro(CellIds, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the flag which controls whether to generate scalar data
   * or field data. If this flag is off, scalar data is generated.
   * Otherwise, field data is generated. Default is off.
   */
  vtkSetMacro(FieldData, vtkTypeBool);
  vtkGetMacro(FieldData, vtkTypeBool);
  vtkBooleanMacro(FieldData, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the name of the Ids array for points, if generated. By default,
   * set to "vtkIdFilter_Ids" for backwards compatibility.
   */
  vtkSetStringMacro(PointIdsArrayName);
  vtkGetStringMacro(PointIdsArrayName);
  ///@}

  ///@{
  /**
   * Set/Get the name of the Ids array for points, if generated. By default,
   * set to "vtkIdFilter_Ids" for backwards compatibility.
   */
  vtkSetStringMacro(CellIdsArrayName);
  vtkGetStringMacro(CellIdsArrayName);
  ///@}
protected:
  vtkIdFilter();
  ~vtkIdFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkTypeBool PointIds;
  vtkTypeBool CellIds;
  vtkTypeBool FieldData;
  char* PointIdsArrayName;
  char* CellIdsArrayName;

private:
  vtkIdFilter(const vtkIdFilter&) = delete;
  void operator=(const vtkIdFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
