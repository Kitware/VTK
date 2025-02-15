// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBilinearQuadIntersection
 * @brief   Class to perform non planar quad intersection
 *
 * Class for non planar quad intersection.
 * This class is an updated and fixed version of the code by Ramsey et al.
 * (http://shaunramsey.com/research/bp/).
 */

#ifndef vtkBilinearQuadIntersection_h
#define vtkBilinearQuadIntersection_h

#include "vtkCommonComputationalGeometryModule.h" // For export macro
#include "vtkVector.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkBilinearQuadIntersection
{
public:
  vtkBilinearQuadIntersection(const vtkVector3d& pt00, const vtkVector3d& Pt01,
    const vtkVector3d& Pt10, const vtkVector3d& Pt11);
  vtkBilinearQuadIntersection() = default;

  ///@{
  /**
   * Get direct access to the underlying point data
   */
  double* GetP00Data();
  double* GetP01Data();
  double* GetP10Data();
  double* GetP11Data();
  //}@

  /**
   * Compute cartesian coordinates of point in the quad
   * using parametric coordinates
   */
  vtkVector3d ComputeCartesianCoordinates(double u, double v);

  /**
   * Compute the intersection between a ray r->q and the quad
   */
  bool RayIntersection(const vtkVector3d& r, const vtkVector3d& q, vtkVector3d& uv);

private:
  vtkVector3d Point00;
  vtkVector3d Point01;
  vtkVector3d Point10;
  vtkVector3d Point11;
  int AxesSwapping = 0;
};
VTK_ABI_NAMESPACE_END
#endif // vtkBilinearQuadIntersection_h
// VTK-HeaderTest-Exclude: vtkBilinearQuadIntersection.h
