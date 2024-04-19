// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOTKernelSmoothing
 * @brief
 * A VTK Filter to compute Kernel Smoothing
 * using PDF computation from openturns.
 */

#ifndef vtkOTKernelSmoothing_h
#define vtkOTKernelSmoothing_h

#include "vtkFiltersOpenTURNSModule.h" // For export macro
#include "vtkOTFilter.h"

namespace OT
{
class KernelSmoothing;
}

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSOPENTURNS_EXPORT vtkOTKernelSmoothing : public vtkOTFilter
{
public:
  static vtkOTKernelSmoothing* New();
  vtkTypeMacro(vtkOTKernelSmoothing, vtkOTFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Methods to set / get number of points to compute, 129 by default
   */
  vtkSetClampMacro(PointNumber, int, 1, VTK_INT_MAX);
  vtkGetMacro(PointNumber, int);
  ///@}

  ///@{
  /**
   * Methods to set / get flag that triggers
   * Gaussian PDF computation, true by default
   */
  vtkSetMacro(GaussianPDF, bool);
  vtkGetMacro(GaussianPDF, bool);
  ///@}

  ///@{
  /**
   * Methods to set / get flag that triggers
   * Triangular PDF computation, true by default
   */
  vtkSetMacro(TriangularPDF, bool);
  vtkGetMacro(TriangularPDF, bool);
  ///@}

  ///@{
  /**
   * Methods to set / get flag that triggers
   * Epanechnikov PDF computation, true by default
   */
  vtkSetMacro(EpanechnikovPDF, bool);
  vtkGetMacro(EpanechnikovPDF, bool);
  ///@}

  ///@{
  /**
   * Methods to set / get the boundary correction, false by default
   */
  vtkSetMacro(BoundaryCorrection, bool);
  vtkGetMacro(BoundaryCorrection, bool);
  ///@}

protected:
  vtkOTKernelSmoothing();
  ~vtkOTKernelSmoothing() override;

  /**
   * Do the actual computation and store it in output
   */
  int Process(OT::Sample* input) override;

  void ComputePDF(OT::Sample* input, OT::KernelSmoothing* ks, double* range, const char* pdfName);

  int PointNumber;
  bool GaussianPDF;
  bool TriangularPDF;
  bool EpanechnikovPDF;
  bool BoundaryCorrection;

private:
  void operator=(const vtkOTKernelSmoothing&) = delete;
  vtkOTKernelSmoothing(const vtkOTKernelSmoothing&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
