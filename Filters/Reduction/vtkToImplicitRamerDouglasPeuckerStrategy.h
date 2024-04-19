// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France
#ifndef vtkToImplicitRamerDouglasPeuckerStrategy_h
#define vtkToImplicitRamerDouglasPeuckerStrategy_h

#include "vtkFiltersReductionModule.h" // for export
#include "vtkToImplicitStrategy.h"

#include <memory>

VTK_ABI_NAMESPACE_BEGIN
/**
 * @class vtkToImplicitRamerDouglasPeuckerStrategy
 *
 * A strategy for creating constant or affine by parts implicit arrays from explicit memory arrays
 * based on the Ramer-Douglas-Peucker algorithm(*).
 *
 * (*)References:
 * - https://en.wikipedia.org/wiki/Ramer%E2%80%93Douglas%E2%80%93Peucker_algorithm
 * - https://cartography-playground.gitlab.io/playgrounds/douglas-peucker-algorithm/
 * - Urs Ramer, An iterative procedure for the polygonal approximation of plane curves, Computer
 * Graphics and Image Processing, Volume 1, Issue 3, 1972, Pages 244-256, ISSN 0146-664X,
 * https://doi.org/10.1016/S0146-664X(72)80017-0.
 * (https://www.sciencedirect.com/science/article/pii/S0146664X72800170)
 * - DOUGLAS, DAVID & PEUCKER, THOMAS. (1973). Algorithms for the Reduction of the Number of Points
 * Required to Represent a Digitized Line or Its Caricature. Cartographica: The International
 * Journal for Geographic Information and Geovisualization. 10.
 * 112-122. 10.3138/FM57-6770-U75U-7727.
 *
 */
class VTKFILTERSREDUCTION_EXPORT vtkToImplicitRamerDouglasPeuckerStrategy final
  : public vtkToImplicitStrategy
{
public:
  static vtkToImplicitRamerDouglasPeuckerStrategy* New();
  vtkTypeMacro(vtkToImplicitRamerDouglasPeuckerStrategy, vtkToImplicitStrategy);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Implements parent API
   */
  vtkToImplicitStrategy::Optional EstimateReduction(vtkDataArray*) override;
  vtkSmartPointer<vtkDataArray> Reduce(vtkDataArray*) override;
  ///@}

  /**
   * Destroys intermediate result of Ramer-Douglas-Peucker algorithm on last array passed to
   * `EstimateReduction`
   */
  void ClearCache() override;

protected:
  vtkToImplicitRamerDouglasPeuckerStrategy();
  ~vtkToImplicitRamerDouglasPeuckerStrategy() override;

private:
  vtkToImplicitRamerDouglasPeuckerStrategy(
    const vtkToImplicitRamerDouglasPeuckerStrategy&) = delete;
  void operator=(const vtkToImplicitRamerDouglasPeuckerStrategy&) = delete;

  struct vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};
VTK_ABI_NAMESPACE_END

#endif // vtkToImplicitRamerDouglasPeuckerStrategy_h
