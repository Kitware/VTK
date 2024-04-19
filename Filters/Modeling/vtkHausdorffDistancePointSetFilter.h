// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) 2011 LTSI INSERM U642
// SPDX-License-Identifier: BSD-3-Clause
/** @class vtkHausdorffDistancePointSetFilter
 *  @brief Compute Hausdorff distance between two point sets
 *
 * This class computes the relative and hausdorff distances from two point
 * sets (input port 0 and input port 1). If no topology is specified (ie.
 * vtkPointSet or vtkPolyData without vtkPolys), the distances are
 * computed between point location. If polys exist (ie triangulation),
 * the TargetDistanceMethod allows for an interpolation of the cells to
 * ensure a better minimal distance exploration.
 *
 * The outputs (port 0 and 1) have the same geometry and topology as its
 * respective input port. Two FieldData arrays are added : HausdorffDistance
 * and RelativeDistance. The former is equal on both outputs whereas the
 * latter may differ. A PointData containing the specific point minimal
 * distance is also added to both outputs.
 *
 * @author Frederic Commandeur
 * @author Jerome Velut
 * @author LTSI
 *
 * @see https://www.vtkjournal.org/browse/publication/839
 */

#ifndef vtkHausdorffDistancePointSetFilter_h
#define vtkHausdorffDistancePointSetFilter_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSMODELING_EXPORT vtkHausdorffDistancePointSetFilter : public vtkPointSetAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for construction, type and printing.
   */
  static vtkHausdorffDistancePointSetFilter* New();
  vtkTypeMacro(vtkHausdorffDistancePointSetFilter, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Get the Relative Distance from A to B and B to A.
   */
  vtkGetVector2Macro(RelativeDistance, double);
  ///@}

  ///@{
  /**
   * Get the Hausdorff Distance.
   */
  vtkGetMacro(HausdorffDistance, double);
  ///@}

  enum DistanceMethod
  {
    POINT_TO_POINT,
    POINT_TO_CELL
  };

  ///@{
  /**
   * Specify the strategy for computing the distance. If no topology is specified (ie.
   * vtkPointSet or vtkPolyData without vtkPolys), the distances are
   * computed between point location. If polys exist (i.e. triangulation),
   * the TargetDistanceMethod allows for an interpolation of the cells to
   * ensure a better minimal distance exploration.
   *
   */
  vtkSetMacro(TargetDistanceMethod, int);
  vtkGetMacro(TargetDistanceMethod, int);
  void SetTargetDistanceMethodToPointToPoint() { this->SetTargetDistanceMethod(POINT_TO_POINT); }
  void SetTargetDistanceMethodToPointToCell() { this->SetTargetDistanceMethod(POINT_TO_CELL); }
  const char* GetTargetDistanceMethodAsString();
  ///@}

protected:
  vtkHausdorffDistancePointSetFilter();
  ~vtkHausdorffDistancePointSetFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  int TargetDistanceMethod;   //!< point-to-point if 0, point-to-cell if 1
  double RelativeDistance[2]; //!< relative distance between inputs
  double HausdorffDistance;   //!< hausdorff distance (max(relative distance))

private:
  vtkHausdorffDistancePointSetFilter(const vtkHausdorffDistancePointSetFilter&) = delete;
  void operator=(const vtkHausdorffDistancePointSetFilter&) = delete;
};
inline const char* vtkHausdorffDistancePointSetFilter::GetTargetDistanceMethodAsString()
{
  if (this->TargetDistanceMethod == POINT_TO_POINT)
  {
    return "PointToPoint";
  }
  else
  {
    return "PointToCell";
  }
}
VTK_ABI_NAMESPACE_END
#endif
