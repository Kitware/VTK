// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkUnstructuredGridVolumeRayCastFunction
 * @brief   a superclass for ray casting functions
 *
 *
 * vtkUnstructuredGridVolumeRayCastFunction is a superclass for ray casting functions that
 * can be used within a vtkUnstructuredGridVolumeRayCastMapper.
 *
 * @sa
 * vtkUnstructuredGridVolumeRayCastMapper vtkUnstructuredGridVolumeRayIntegrator
 */

#ifndef vtkUnstructuredGridVolumeRayCastFunction_h
#define vtkUnstructuredGridVolumeRayCastFunction_h

#include "vtkObject.h"
#include "vtkRenderingVolumeModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderer;
class vtkVolume;
class vtkUnstructuredGridVolumeRayCastIterator;

class VTKRENDERINGVOLUME_EXPORT vtkUnstructuredGridVolumeRayCastFunction : public vtkObject
{
public:
  vtkTypeMacro(vtkUnstructuredGridVolumeRayCastFunction, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual void Initialize(vtkRenderer* ren, vtkVolume* vol) = 0;

  virtual void Finalize() = 0;

  /**
   * Returns a new object that will iterate over all the intersections of a
   * ray with the cells of the input.  The calling code is responsible for
   * deleting the returned object.
   */
  VTK_NEWINSTANCE
  virtual vtkUnstructuredGridVolumeRayCastIterator* NewIterator() = 0;

protected:
  vtkUnstructuredGridVolumeRayCastFunction() = default;
  ~vtkUnstructuredGridVolumeRayCastFunction() override = default;

private:
  vtkUnstructuredGridVolumeRayCastFunction(
    const vtkUnstructuredGridVolumeRayCastFunction&) = delete;
  void operator=(const vtkUnstructuredGridVolumeRayCastFunction&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
