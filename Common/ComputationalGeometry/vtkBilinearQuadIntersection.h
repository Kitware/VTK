/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLagrangianParticleTracker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class VTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT vtkBilinearQuadIntersection
{
public:
  vtkBilinearQuadIntersection(const vtkVector3d& pt00, const vtkVector3d& Pt01,
    const vtkVector3d& Pt10, const vtkVector3d& Pt11);
  vtkBilinearQuadIntersection() = default;

  //@{
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
   * using parameteric coordinates
   */
  vtkVector3d ComputeCartesianCoordinates(double u, double v);

  /**
   * Compute the intersection between a ray r->d and the quad
   */
  bool RayIntersection(const vtkVector3d& r, const vtkVector3d& d, vtkVector3d& uv);

private:
  vtkVector3d Point00;
  vtkVector3d Point01;
  vtkVector3d Point10;
  vtkVector3d Point11;
  int AxesSwapping = 0;
};
#endif // vtkBilinearQuadIntersection_h
// VTK-HeaderTest-Exclude: vtkBilinearQuadIntersection.h
