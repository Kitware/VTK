// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2004 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkUnstructuredGridPartialPreIntegration
 * @brief   performs piecewise linear ray integration.
 *
 *
 *
 * vtkUnstructuredGridPartialPreIntegration performs piecewise linear ray
 * integration.  This will give the same results as
 * vtkUnstructuredGridLinearRayIntegration (with potentially a error due to
 * table lookup quantization), but should be notably faster.  The algorithm
 * used is given by Moreland and Angel, "A Fast High Accuracy Volume
 * Renderer for Unstructured Data."
 *
 * This class is thread safe only after the first instance is created.
 *
 */

#ifndef vtkUnstructuredGridPartialPreIntegration_h
#define vtkUnstructuredGridPartialPreIntegration_h

#include "vtkMath.h"                  // For all the inline methods
#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkUnstructuredGridVolumeRayIntegrator.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkPartialPreIntegrationTransferFunction;
class vtkVolumeProperty;

class VTKRENDERINGVOLUME_EXPORT vtkUnstructuredGridPartialPreIntegration
  : public vtkUnstructuredGridVolumeRayIntegrator
{
public:
  vtkTypeMacro(vtkUnstructuredGridPartialPreIntegration, vtkUnstructuredGridVolumeRayIntegrator);
  static vtkUnstructuredGridPartialPreIntegration* New();
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

  ///@{
  /**
   * Looks up Psi (as defined by Moreland and Angel, "A Fast High Accuracy
   * Volume Renderer for Unstructured Data") in a table.  The table must be
   * created first, which happens on the first instantiation of this class
   * or when BuildPsiTable is first called.
   */
  static float Psi(float taufD, float taubD);
  static float* GetPsiTable(int& size);
  static void BuildPsiTable();
  ///@}

protected:
  vtkUnstructuredGridPartialPreIntegration();
  ~vtkUnstructuredGridPartialPreIntegration() override;

  vtkVolumeProperty* Property;

  vtkPartialPreIntegrationTransferFunction* TransferFunctions;
  vtkTimeStamp TransferFunctionsModified;
  int NumIndependentComponents;

  enum
  {
    PSI_TABLE_SIZE = 512
  };

  static float PsiTable[PSI_TABLE_SIZE * PSI_TABLE_SIZE];
  static int PsiTableBuilt;

private:
  vtkUnstructuredGridPartialPreIntegration(
    const vtkUnstructuredGridPartialPreIntegration&) = delete;
  void operator=(const vtkUnstructuredGridPartialPreIntegration&) = delete;
};

inline float vtkUnstructuredGridPartialPreIntegration::Psi(float taufD, float taubD)
{
  float gammaf = taufD / (taufD + 1);
  float gammab = taubD / (taubD + 1);
  int gammafi = vtkMath::Floor(gammaf * static_cast<int>(PSI_TABLE_SIZE));
  int gammabi = vtkMath::Floor(gammab * static_cast<int>(PSI_TABLE_SIZE));
  return PsiTable[gammafi * PSI_TABLE_SIZE + gammabi];
}

inline float* vtkUnstructuredGridPartialPreIntegration::GetPsiTable(int& size)
{
  size = PSI_TABLE_SIZE;
  return PsiTable;
}

inline void vtkUnstructuredGridPartialPreIntegration::IntegrateRay(double length,
  double intensity_front, double attenuation_front, double intensity_back, double attenuation_back,
  float color[4])
{
  float taufD = length * attenuation_front;
  float taubD = length * attenuation_back;
  float Psi = vtkUnstructuredGridPartialPreIntegration::Psi(taufD, taubD);
  float zeta = static_cast<float>(exp(-0.5 * (taufD + taubD)));
  float alpha = 1 - zeta;

  float newintensity =
    (1 - color[3]) * (intensity_front * (1 - Psi) + intensity_back * (Psi - zeta));
  // Is setting the RGB values the same the right thing to do?
  color[0] += newintensity;
  color[1] += newintensity;
  color[2] += newintensity;
  color[3] += (1 - color[3]) * alpha;
}

inline void vtkUnstructuredGridPartialPreIntegration::IntegrateRay(double length,
  const double color_front[3], double attenuation_front, const double color_back[3],
  double attenuation_back, float color[4])
{
  float taufD = length * attenuation_front;
  float taubD = length * attenuation_back;
  float Psi = vtkUnstructuredGridPartialPreIntegration::Psi(taufD, taubD);
  float zeta = static_cast<float>(exp(-0.5 * (taufD + taubD)));
  float alpha = 1 - zeta;

  color[0] += (1 - color[3]) * (color_front[0] * (1 - Psi) + color_back[0] * (Psi - zeta));
  color[1] += (1 - color[3]) * (color_front[1] * (1 - Psi) + color_back[1] * (Psi - zeta));
  color[2] += (1 - color[3]) * (color_front[2] * (1 - Psi) + color_back[2] * (Psi - zeta));
  color[3] += (1 - color[3]) * alpha;
}

VTK_ABI_NAMESPACE_END
#endif // vtkUnstructuredGridPartialPreIntegration_h
