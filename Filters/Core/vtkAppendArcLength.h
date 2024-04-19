// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAppendArcLength
 * @brief   appends Arc length for input poly lines.
 *
 * vtkAppendArcLength is used for filter such as plot-over-line. In such cases,
 * we need to add an attribute array that is the arc_length over the length of
 * the probed line. That's when vtkAppendArcLength can be used. It adds a new
 * point-data array named "arc_length" with the computed arc length for each of
 * the polylines in the input. For all other cell types, the arc length is set
 * to 0.
 * @warning
 * This filter assumes that cells don't share points.
 */

#ifndef vtkAppendArcLength_h
#define vtkAppendArcLength_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkAppendArcLength : public vtkPolyDataAlgorithm
{
public:
  static vtkAppendArcLength* New();
  vtkTypeMacro(vtkAppendArcLength, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkAppendArcLength();
  ~vtkAppendArcLength() override;

  ///@{
  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkAppendArcLength(const vtkAppendArcLength&) = delete;
  void operator=(const vtkAppendArcLength&) = delete;
  ///@}
};

VTK_ABI_NAMESPACE_END
#endif
