// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImagePermute
 * @brief    Permutes axes of input.
 *
 * vtkImagePermute reorders the axes of the input. Filtered axes specify
 * the input axes which become X, Y, Z.  The input has to have the
 * same scalar type of the output. The filter does copy the
 * data when it executes. This filter is actually a very thin wrapper
 * around vtkImageReslice.
 */

#ifndef vtkImagePermute_h
#define vtkImagePermute_h

#include "vtkImageReslice.h"
#include "vtkImagingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGCORE_EXPORT vtkImagePermute : public vtkImageReslice
{
public:
  static vtkImagePermute* New();
  vtkTypeMacro(vtkImagePermute, vtkImageReslice);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The filtered axes are the input axes that get relabeled to X,Y,Z.
   */
  void SetFilteredAxes(int x, int y, int z);
  void SetFilteredAxes(const int xyz[3]) { this->SetFilteredAxes(xyz[0], xyz[1], xyz[2]); }
  vtkGetVector3Macro(FilteredAxes, int);
  ///@}

protected:
  vtkImagePermute();
  ~vtkImagePermute() override = default;

  int FilteredAxes[3];

private:
  vtkImagePermute(const vtkImagePermute&) = delete;
  void operator=(const vtkImagePermute&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
