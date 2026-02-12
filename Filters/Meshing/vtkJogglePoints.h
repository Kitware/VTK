// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkJogglePoints
 * @brief   randomly perturb (or jitter) point positions
 *
 * vtkJogglePoints randomly perturbs (alternatively: jitters, jiggles) point
 * positions. It operates on any input vtkPointSet, modifying only the point
 * positions. The amount of joggle, and constraints on the direction of
 * joggle can be specified. The input topology and point and cell attribute
 * data are preserved on output - the output dataset type is the same as the
 * input.  Typically, all input points to this filter are joggled. However,
 * if an optional input point data array of type unsigned char is provided,
 * then only those points with an array value > 0 are joggled.
 *
 * This utility class may be used to randomize probes and data. It is very
 * useful for improving the behavior and performance of Voronoi and Delaunay
 * tessellations as it removes degeneracies. (For a theoretical treatment of
 * the joggle operation, especially as it pertains to convex hull, Voronoi,
 * and Delaunay generation, see the QHull documentation (qhull.org)).
 *
 * @warning
 * Large amount of joggle may turn topology inside out, and/or cause
 * surface self-intersection.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * vtkVoronoiFlower2D vtkVoronoiFlower3D vtkGeneralizedSurfaceNets3D
 * vtkDelaunay2D vtkDelaunay3D vtkLabeledImagePointSampler
 * vtkFillPointCloud
 */

#ifndef vtkJogglePoints_h
#define vtkJogglePoints_h

#include "vtkFiltersMeshingModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"
#include <random> // For random number generation

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSMESHING_EXPORT vtkJogglePoints : public vtkPointSetAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static vtkJogglePoints* New();
  vtkTypeMacro(vtkJogglePoints, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the maximum joggle distance. The joggle radius can be
   * specified as an absolute number, or a fraction of the input
   * dataset bounding box (diagonal) length. By default, it is a relative
   * number.
   */
  vtkSetClampMacro(Radius, double, 0, VTK_DOUBLE_MAX);
  vtkGetMacro(Radius, double);
  vtkSetMacro(RadiusIsAbsolute, vtkTypeBool);
  vtkBooleanMacro(RadiusIsAbsolute, vtkTypeBool);
  vtkGetMacro(RadiusIsAbsolute, vtkTypeBool);
  ///@}

  ///@{
  enum JoggleConstraints
  {
    UNCONSTRAINED = 0,
    XY_PLANE = 1,
    XZ_PLANE = 2,
    YZ_PLANE = 3
  };

  /**
   * Specify constraints on the point perturbations (i.e., joggle motion). By
   * default, points are unconstrained and randomly joggled in the x-y-z
   * directions (UNCONSTRAINED). Other options include motion constraints in
   * the x-y plane (XY_PLANE), the x-z plane (XZ_PLANE), and the y-z plane
   * (YZ_PLANE).
   */
  vtkSetClampMacro(Joggle, int, UNCONSTRAINED, YZ_PLANE);
  vtkGetMacro(Joggle, int);
  void SetJoggleToUnconstrained() { this->SetJoggle(UNCONSTRAINED); }
  void SetJoggleToXYPlane() { this->SetJoggle(XY_PLANE); }
  void SetJoggleToXZPlane() { this->SetJoggle(XZ_PLANE); }
  void SetJoggleToYZPlane() { this->SetJoggle(YZ_PLANE); }
  ///@}

protected:
  vtkJogglePoints();
  ~vtkJogglePoints() override;

  double Radius;
  vtkTypeBool RadiusIsAbsolute;
  int Joggle;

  // Support second input
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkJogglePoints(const vtkJogglePoints&) = delete;
  void operator=(const vtkJogglePoints&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
