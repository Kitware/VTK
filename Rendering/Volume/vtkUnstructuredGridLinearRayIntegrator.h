// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2004 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkUnstructuredGridLinearRayIntegrator
 * @brief   performs piecewise linear ray integration.
 *
 *
 *
 * vtkUnstructuredGridLinearRayIntegrator performs piecewise linear ray
 * integration.  Considering that transfer functions in VTK are piecewise
 * linear, this class should give the "correct" integration under most
 * circumstances.  However, the computations performed are fairly hefty and
 * should, for the most part, only be used as a benchmark for other, faster
 * methods.
 *
 * @sa
 * vtkUnstructuredGridPartialPreIntegration
 *
 */

#ifndef vtkUnstructuredGridLinearRayIntegrator_h
#define vtkUnstructuredGridLinearRayIntegrator_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkUnstructuredGridVolumeRayIntegrator.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkLinearRayIntegratorTransferFunction;
class vtkVolumeProperty;

class VTKRENDERINGVOLUME_EXPORT vtkUnstructuredGridLinearRayIntegrator
  : public vtkUnstructuredGridVolumeRayIntegrator
{
public:
  vtkTypeMacro(vtkUnstructuredGridLinearRayIntegrator, vtkUnstructuredGridVolumeRayIntegrator);
  static vtkUnstructuredGridLinearRayIntegrator* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize(vtkVolume* volume, vtkDataArray* scalars) override;

  void Integrate(vtkDoubleArray* intersectionLengths, vtkDataArray* nearIntersections,
    vtkDataArray* farIntersections, float color[4]) override;

  ///@{
  /**
   * Integrates a single ray segment.  \c color is blended with the result
   * (with \c color in front).  The result is written back into \c color.
   */
  static void IntegrateRay(double length, double intensity_front, double attenuation_front,
    double intensity_back, double attenuation_back, float color[4]);
  static void IntegrateRay(double length, const double color_front[3], double attenuation_front,
    const double color_back[3], double attenuation_back, float color[4]);
  ///@}

  /**
   * Computes Psi (as defined by Moreland and Angel, "A Fast High Accuracy
   * Volume Renderer for Unstructured Data").
   */
  static float Psi(float length, float attenuation_front, float attenuation_back);

protected:
  vtkUnstructuredGridLinearRayIntegrator();
  ~vtkUnstructuredGridLinearRayIntegrator() override;

  vtkVolumeProperty* Property;

  vtkLinearRayIntegratorTransferFunction* TransferFunctions;
  vtkTimeStamp TransferFunctionsModified;
  int NumIndependentComponents;

private:
  vtkUnstructuredGridLinearRayIntegrator(const vtkUnstructuredGridLinearRayIntegrator&) = delete;
  void operator=(const vtkUnstructuredGridLinearRayIntegrator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkUnstructuredGridLinearRayIntegrator_h
