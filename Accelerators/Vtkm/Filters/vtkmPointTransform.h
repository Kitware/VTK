// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkmPointTransform
 * @brief transform points via viskores PointTransform filter
 *
 * vtkmPointTransform is a filter to transform point coordinates. For now it
 * does not support transforming associated point normals and vectors, as well
 * as cell normals and vectors with the point coordinates.
 */

#ifndef vtkmPointTransform_h
#define vtkmPointTransform_h

#include "vtkAcceleratorsVTKmFiltersModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"
#include "vtkmAlgorithm.h"           // For vtkmAlgorithm
#include "vtkmlib/vtkmInitializer.h" // Need for initializing viskores

#ifndef __VTK_WRAP__
#define vtkPointSetAlgorithm vtkmAlgorithm<vtkPointSetAlgorithm>
#endif

VTK_ABI_NAMESPACE_BEGIN
class vtkHomogeneousTransform;

class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmPointTransform : public vtkPointSetAlgorithm
{
public:
  vtkTypeMacro(vtkmPointTransform, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
#ifndef __VTK_WRAP__
#undef vtkPointSetAlgorithm
#endif
  static vtkmPointTransform* New();

  ///@{
  /**
   * Specify the transform object used to transform the points
   */
  void SetTransform(vtkHomogeneousTransform* tf);
  vtkGetObjectMacro(Transform, vtkHomogeneousTransform);
  ///@}

  int FillInputPortInformation(int port, vtkInformation* info) override;

protected:
  vtkmPointTransform();
  ~vtkmPointTransform() override;
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  vtkHomogeneousTransform* Transform;

private:
  vtkmPointTransform(const vtkmPointTransform&) = delete;
  void operator=(const vtkmPointTransform&) = delete;
  vtkmInitializer Initializer;
};

VTK_ABI_NAMESPACE_END
#endif
