// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPoissonDiskSampler
 * @brief   generate point normals using local tangent planes
 *
 *
 * vtkPoissonDiskSampler performs a poisson disk sampling on the input. It takes any `vtkPointSet`
 * as input and produces a `vtkPointSet`. If the input has cells (typically if the input is a
 * `vtkPolyData` or a `vtkUnstructuredGrid`), cells are removed in the output point set.
 *
 * PoissonDisk sampling is done by doing "dart throwing". It is very similar to the implementation
 * proposed by <a
 * href="http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.476.9482&rep=rep1&type=pdf">
 * Dipp\'e and Wold in 1986</a>. Points are drawn randomly one by one and added
 * in the output. Points within a range of `Radius` (input parameter) are discarded from the
 * output. This process is repeated until there are no more points unprocessed.
 */

#ifndef vtkPoissonDiskSampler_h
#define vtkPoissonDiskSampler_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractPointLocator;
class vtkIdList;

class VTKFILTERSPOINTS_EXPORT vtkPoissonDiskSampler : public vtkPointSetAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static vtkPoissonDiskSampler* New();
  vtkTypeMacro(vtkPoissonDiskSampler, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Setter / Getter for `Radius`. It is used to determinate the minimum distance that there should
   * be between 2 nearest points in the output.
   */
  vtkSetMacro(Radius, double);
  vtkGetMacro(Radius, double);
  ///@}

  ///@{
  /**
   * Specify a point locator. By default a vtkKdTreePointLocator is
   * used. The locator performs efficient searches to locate points
   * around a sample point.
   */
  void SetLocator(vtkAbstractPointLocator* locator);
  vtkGetObjectMacro(Locator, vtkAbstractPointLocator);
  ///@}

protected:
  vtkPoissonDiskSampler();
  ~vtkPoissonDiskSampler() override;

  /**
   * Radius used to query point neighbors using the `Locator`.
   */
  double Radius;

  /**
   * Locator being used to query point neighbors.
   */
  vtkAbstractPointLocator* Locator;

  // Pipeline management
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkPoissonDiskSampler(const vtkPoissonDiskSampler&) = delete;
  void operator=(const vtkPoissonDiskSampler&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
