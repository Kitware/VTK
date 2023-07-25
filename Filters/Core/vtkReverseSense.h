// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkReverseSense
 * @brief   reverse the ordering of polygonal cells and/or vertex normals
 *
 *
 * vtkReverseSense is a filter that reverses the order of polygonal cells
 * and/or reverses the direction of point and cell normals. Two flags are
 * used to control these operations. Cell reversal means reversing the order
 * of indices in the cell connectivity list. Normal reversal means
 * multiplying the normal vector by -1 (both point and cell normals,
 * if present).
 *
 * @warning
 * Normals can be operated on only if they are present in the data.
 */

#ifndef vtkReverseSense_h
#define vtkReverseSense_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkReverseSense : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkReverseSense, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object so that behavior is to reverse cell ordering and
   * leave normal orientation as is.
   */
  static vtkReverseSense* New();

  ///@{
  /**
   * Flag controls whether to reverse cell ordering.
   */
  vtkSetMacro(ReverseCells, vtkTypeBool);
  vtkGetMacro(ReverseCells, vtkTypeBool);
  vtkBooleanMacro(ReverseCells, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Flag controls whether to reverse normal orientation.
   */
  vtkSetMacro(ReverseNormals, vtkTypeBool);
  vtkGetMacro(ReverseNormals, vtkTypeBool);
  vtkBooleanMacro(ReverseNormals, vtkTypeBool);
  ///@}

protected:
  vtkReverseSense();
  ~vtkReverseSense() override = default;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkTypeBool ReverseCells;
  vtkTypeBool ReverseNormals;

private:
  vtkReverseSense(const vtkReverseSense&) = delete;
  void operator=(const vtkReverseSense&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
