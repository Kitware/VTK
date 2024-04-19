// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImplicitProjectOnPlaneDistance
 *
 * This class receive a plannar polygon as input. Given a point, it can
 * evaluate the L0 or L2 norm between the projection of this point on the plan
 * of the polygon and the polygon itself.
 *
 * An interesting use of this class is to enable the L0 norm and evaluate the
 * "projected distance" between every vertex of a mesh and the given plannar polygon.
 * As a result, all the vertices that project onto the polygon will correspond to the
 * value 0 and other ones will receive the value 1.
 * From there, we can use a clip to keep only the part of the mesh "below" the polygon.
 *
 * TLDR: This filter allows to clip using the extrusion of any plannar polygon.
 */

#ifndef vtkImplicitProjectOnPlaneDistance_h
#define vtkImplicitProjectOnPlaneDistance_h

#include "vtkImplicitFunction.h"

#include "vtkAbstractCellLocator.h" // User defined cellLocator
#include "vtkFiltersCoreModule.h"   // For export macro
#include "vtkSmartPointer.h"        // It has vtkSmartPointer fields

VTK_ABI_NAMESPACE_BEGIN
class vtkGenericCell;
class vtkPolyData;
class vtkPlane;

class VTKFILTERSCORE_EXPORT vtkImplicitProjectOnPlaneDistance : public vtkImplicitFunction
{
public:
  enum class NormType
  {
    L0 = 0,
    L2 = 2
  };

  static vtkImplicitProjectOnPlaneDistance* New();
  vtkTypeMacro(vtkImplicitProjectOnPlaneDistance, vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Return the MTime also considering the Input dependency.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Project x onto the plane defined by the Input polydata and evaluate the
   * distance to the geometry defined by the Input polydata.
   */
  using vtkImplicitFunction::EvaluateFunction;
  double EvaluateFunction(double x[3]) override;

  /**
   * Evaluate function gradient of nearest triangle to point x[3].
   * WARNING: not implemented as it is of no use in this context.
   */
  void EvaluateGradient(double x[3], double g[3]) override;

  /**
   * Set the input vtkPolyData used for the implicit function
   * evaluation. This polydata needs to be planar.
   */
  void SetInput(vtkPolyData* input);

  ///@{
  /**
   * Set/get the tolerance used for the locator.
   * Default is 0.01.
   */
  vtkGetMacro(Tolerance, double);
  vtkSetMacro(Tolerance, double);
  ///@}

  ///@{
  /**
   * Set the norm to use:
   * L0: 0 when the projection is inside the input polygon, 1 otherwise
   * L2: Euclidean distance between the projection and the polygon (default)
   */
  NormType GetNorm() const { return Norm; }
  void SetNorm(NormType n)
  {
    Norm = n;
    Modified();
  }
#ifndef __VTK_WRAP_JAVA__
  // The Java wrappers cannot resolve this signature from the one above,
  // see https://gitlab.kitware.com/vtk/vtk/-/issues/17744
  void SetNorm(int n)
  {
    Norm = static_cast<NormType>(n);
    Modified();
  }
#endif
  ///@}

  ///@{
  /**
   * Set/get the Locator used by to compute the distance.
   * A vtkStaticCellLocator is provided by default if
   * none is given by the user.
   */
  vtkGetSmartPointerMacro(Locator, vtkAbstractCellLocator);
  vtkSetSmartPointerMacro(Locator, vtkAbstractCellLocator);
  ///@}

protected:
  vtkImplicitProjectOnPlaneDistance();
  ~vtkImplicitProjectOnPlaneDistance() override = default;

  /**
   * Create a default locator (vtkStaticCellLocator).
   * Used to create one when none is specified by the user.
   */
  void CreateDefaultLocator();

  double Tolerance;
  NormType Norm;

  vtkSmartPointer<vtkPolyData> Input;
  vtkSmartPointer<vtkAbstractCellLocator> Locator;
  vtkSmartPointer<vtkPlane> ProjectionPlane;

  // Stored here to avoid multiple allocation / dealloction
  vtkSmartPointer<vtkGenericCell> UnusedCell;
  double Bounds[6];

private:
  vtkImplicitProjectOnPlaneDistance(const vtkImplicitProjectOnPlaneDistance&) = delete;
  void operator=(const vtkImplicitProjectOnPlaneDistance&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
