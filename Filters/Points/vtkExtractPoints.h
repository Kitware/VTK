// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtractPoints
 * @brief   extract points within an implicit function
 *
 *
 * vtkExtractPoints removes points that are either inside or outside of a
 * vtkImplicitFunction. Implicit functions in VTK defined as function of the
 * form f(x,y,z)=c, where values c<=0 are interior values of the implicit
 * function. Typical examples include planes, spheres, cylinders, cones,
 * etc. plus boolean combinations of these functions. (This operation
 * presumes closure on the set, so points on the boundary are also considered
 * to be inside.)
 *
 * Note that while any vtkPointSet type can be provided as input, the output is
 * represented by an explicit representation of points via a
 * vtkPolyData. This output polydata will populate its instance of vtkPoints,
 * but no cells will be defined (i.e., no vtkVertex or vtkPolyVertex are
 * contained in the output). Also, after filter execution, the user can
 * request a vtkIdType* map which indicates how the input points were mapped
 * to the output. A value of map[i] (where i is the ith input point) less
 * than 0 means that the ith input point was removed. (See also the
 * superclass documentation for accessing the removed points through the
 * filter's second output.)
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @warning
 * The vtkExtractEnclosedPoints filter can be used to extract points inside of
 * a volume defined by a manifold, closed polygonal surface. This filter
 * however is much slower than methods based on implicit functions (like this
 * filter).
 *
 * @sa
 * vtkExtractEnclosedPoints vtkSelectEnclosedPoints vtkPointCloudFilter
 * vtkRadiusOutlierRemoval vtkStatisticalOutlierRemoval vtkThresholdPoints
 * vtkImplicitFunction vtkExtractGeometry vtkFitImplicitFunction
 */

#ifndef vtkExtractPoints_h
#define vtkExtractPoints_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPointCloudFilter.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkImplicitFunction;
class vtkPointSet;

class VTKFILTERSPOINTS_EXPORT vtkExtractPoints : public vtkPointCloudFilter
{
public:
  ///@{
  /**
   * Standard methods for instantiating, obtaining type information, and
   * printing information.
   */
  static vtkExtractPoints* New();
  vtkTypeMacro(vtkExtractPoints, vtkPointCloudFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify the implicit function for inside/outside checks.
   */
  virtual void SetImplicitFunction(vtkImplicitFunction*);
  vtkGetObjectMacro(ImplicitFunction, vtkImplicitFunction);
  ///@}

  ///@{
  /**
   * Boolean controls whether to extract points that are inside of implicit
   * function (ExtractInside == true) or outside of implicit function
   * (ExtractInside == false). By default, ExtractInside is true.
   */
  vtkSetMacro(ExtractInside, bool);
  vtkGetMacro(ExtractInside, bool);
  vtkBooleanMacro(ExtractInside, bool);
  ///@}

  /**
   * Return the MTime taking into account changes to the implicit function
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkExtractPoints();
  ~vtkExtractPoints() override;

  vtkImplicitFunction* ImplicitFunction;
  bool ExtractInside;

  // All derived classes must implement this method. Note that a side effect of
  // the class is to populate the PointMap. Zero is returned if there is a failure.
  int FilterPoints(vtkPointSet* input) override;

private:
  vtkExtractPoints(const vtkExtractPoints&) = delete;
  void operator=(const vtkExtractPoints&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
