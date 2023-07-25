// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPTextureMapToSphere
 * @brief   generate texture coordinates by mapping points to sphere
 *
 * vtkPTextureMapToSphere inherits from vtkTextureMapToSphere to handle multi-processing
 * environment.
 *
 * @sa
 * vtkTextureMapToPlane vtkTextureMapToCylinder
 * vtkTransformTexture vtkThresholdTextureCoords
 * vtkTextureMapToSphere
 */

#ifndef vtkPTextureMapToSphere_h
#define vtkPTextureMapToSphere_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkTextureMapToSphere.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataSet;
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPTextureMapToSphere : public vtkTextureMapToSphere
{
public:
  vtkTypeMacro(vtkPTextureMapToSphere, vtkTextureMapToSphere);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPTextureMapToSphere* New();

protected:
  vtkPTextureMapToSphere();
  ~vtkPTextureMapToSphere() override;

  void ComputeCenter(vtkDataSet* dataSet) override;

  void SetController(vtkMultiProcessController*);
  vtkMultiProcessController* Controller;

private:
  vtkPTextureMapToSphere(const vtkPTextureMapToSphere&) = delete;
  void operator=(const vtkPTextureMapToSphere&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
