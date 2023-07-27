// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDensifyPointCloudFilter
 * @brief   add points to a point cloud to make it denser
 *
 * vtkDensifyPointCloudFilter adds new points to an input point cloud. The
 * new points are created in such a way that all points in any local
 * neighborhood are within a target distance of one another. Optionally,
 * attribute data can be interpolated from the input point cloud as well.
 *
 * A high-level overview of the algorithm is as follows. For each input
 * point, the distance to all points in its neighborhood is computed. If any
 * of its neighbors is further than the target distance, the edge connecting
 * the point and its neighbor is bisected and a new point is inserted at the
 * bisection point (optionally the attribute data is interpolated as well). A
 * single pass is completed once all the input points are visited. Then the
 * process repeats to the limit of the maximum number of iterations.
 *
 * @warning
 * This class can generate a lot of points very quickly. The maximum number
 * of iterations is by default set to =1.0 for this reason. Increase the
 * number of iterations very carefully. Also the MaximumNumberOfPoints
 * data member can be set to limit the explosion of points. It is also
 * recommended that a N closest neighborhood is used.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkVoxelGridFilter vtkEuclideanClusterExtraction vtkBoundedPointSource
 */

#ifndef vtkDensifyPointCloudFilter_h
#define vtkDensifyPointCloudFilter_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSPOINTS_EXPORT vtkDensifyPointCloudFilter : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static vtkDensifyPointCloudFilter* New();
  vtkTypeMacro(vtkDensifyPointCloudFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * This enum is used to specify how the local point neighborhood is
   * defined.  A radius-based neighborhood in one where all points inside a
   * specified radius is part of the neighborhood. A N closest neighborhood is
   * one in which the N closest points are part of the neighborhood. (Note that
   * in some cases, if points are precisely the same distance apart, the N closest
   * may not return all points within an expected radius.)
   */
  enum NeighborhoodType
  {
    RADIUS = 0,
    N_CLOSEST = 1
  };

  ///@{
  /**
   * Specify how the local point neighborhood is defined. By default an N
   * closest neighborhood is used. This tends to avoid explosive point
   * creation.
   */
  vtkSetMacro(NeighborhoodType, int);
  vtkGetMacro(NeighborhoodType, int);
  void SetNeighborhoodTypeToRadius() { this->SetNeighborhoodType(RADIUS); }
  void SetNeighborhoodTypeToNClosest() { this->SetNeighborhoodType(N_CLOSEST); }
  ///@}

  ///@{
  /**
   * Define a local neighborhood for each point in terms of a local
   * radius. By default, the radius is 1.0. This data member is relevant only
   * if the neighborhood type is RADIUS.
   */
  vtkSetClampMacro(Radius, double, 1, VTK_DOUBLE_MAX);
  vtkGetMacro(Radius, double);
  ///@}

  ///@{
  /**
   * Define a local neighborhood in terms of the N closest points. By default
   * the number of the closest points is =6. This data member is relevant
   * only if the neighborhood type is N_CLOSEST.
   */
  vtkSetClampMacro(NumberOfClosestPoints, int, 1, VTK_INT_MAX);
  vtkGetMacro(NumberOfClosestPoints, int);
  ///@}

  ///@{
  /**
   * Set / get the target point distance. Points will be created in an
   * iterative fashion until all points in their local neighborhood are the
   * target distance apart or less. Note that the process may terminate early
   * due to the limit on the maximum number of iterations. By default the target
   * distance is set to 0.5. Note that the TargetDistance should be less than
   * the Radius or nothing will change on output.
   */
  vtkSetClampMacro(TargetDistance, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(TargetDistance, double);
  ///@}

  ///@{
  /**
   * The maximum number of iterations to run. By default the maximum is
   * one.
   */
  vtkSetClampMacro(MaximumNumberOfIterations, int, 1, VTK_SHORT_MAX);
  vtkGetMacro(MaximumNumberOfIterations, int);
  ///@}

  ///@{
  /**
   * Set a limit on the maximum number of points that can be created. This
   * data member serves as a crude barrier to explosive point creation; it does
   * not guarantee that precisely these many points will be created. Once this
   * limit is hit, it may result in premature termination of the
   * algorithm. Consider it a pressure relief valve.
   */
  vtkSetClampMacro(MaximumNumberOfPoints, vtkIdType, 1, VTK_ID_MAX);
  vtkGetMacro(MaximumNumberOfPoints, vtkIdType);
  ///@}

  ///@{
  /**
   * Turn on/off the interpolation of attribute data from the input point
   * cloud to new, added points.
   */
  vtkSetMacro(InterpolateAttributeData, bool);
  vtkGetMacro(InterpolateAttributeData, bool);
  vtkBooleanMacro(InterpolateAttributeData, bool);
  ///@}

protected:
  vtkDensifyPointCloudFilter();
  ~vtkDensifyPointCloudFilter() override;

  // Data members
  int NeighborhoodType;
  double Radius;
  int NumberOfClosestPoints;
  double TargetDistance;
  int MaximumNumberOfIterations;
  bool InterpolateAttributeData;
  vtkIdType MaximumNumberOfPoints;

  // Pipeline management
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkDensifyPointCloudFilter(const vtkDensifyPointCloudFilter&) = delete;
  void operator=(const vtkDensifyPointCloudFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
