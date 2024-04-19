// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageXYZToLAB
 * @brief   Converts XYZ components to LAB.
 *
 * For each pixel int the XYZ color space, this
 * filter output the color in the Lab color space
 * Output type must be the same as input type.
 */

#ifndef vtkImageXYZToLAB_h
#define vtkImageXYZToLAB_h

#include "vtkImagingColorModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGCOLOR_EXPORT vtkImageXYZToLAB : public vtkThreadedImageAlgorithm
{
public:
  static vtkImageXYZToLAB* New();
  vtkTypeMacro(vtkImageXYZToLAB, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkImageXYZToLAB();
  ~vtkImageXYZToLAB() override = default;

  void ThreadedExecute(vtkImageData* inData, vtkImageData* outData, int ext[6], int id) override;

private:
  vtkImageXYZToLAB(const vtkImageXYZToLAB&) = delete;
  void operator=(const vtkImageXYZToLAB&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
