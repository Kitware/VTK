// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkProjectPointsToPlane
 * @brief   project all input points to a plane
 *
 * vtkProjectPointsToPlane is a filter that operates on a vtkPointSet (and
 * its subclasses), projecting all input points to a plane. There are
 * multiple options as to what plane to project to: The user may specify one
 * of the x-y-z planes, the best coordinate plane, a user-defined plane, or
 * the closest fitting plane (using a least-squares method). On output, the
 * points will lie on the specified plane, and any cells connected to the
 * points (if any) will be deformed accordingly. On output, the filter will
 * not modify dataset topology, nor modify point or cell attributes. Only the
 * point coordinates (geometry) will be modified. (Note that the filter will
 * operate on input point sets with or without cells.)
 *
 * @warning
 * It is possible that cells connected to the projected points will become
 * invalid after the projection operation.
 *
 * @sa
 * vtkPlane
 */

#ifndef vtkProjectPointsToPlane_h
#define vtkProjectPointsToPlane_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSPOINTS_EXPORT vtkProjectPointsToPlane : public vtkPointSetAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and
   * printing.
   */
  static vtkProjectPointsToPlane* New();
  vtkTypeMacro(vtkProjectPointsToPlane, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Define the options available for point projection. By default,
   * the filter projects to the z-plane.
   */
  enum PlaneProjectionType
  {
    X_PLANE = 1,
    Y_PLANE = 2,
    Z_PLANE = 3,
    SPECIFIED_PLANE = 4,
    BEST_COORDINATE_PLANE = 5,
    BEST_FIT_PLANE = 6
  };

  ///@{
  /**
   * Specify the type of projection to perform. Points may be projected to
   * the 1) x-plane; 2) y-plane; 3) z-plane; 4) user-specified plane; 5) the
   * plane most orthogonal to one of the coordinate axes x, y, or z; or 6)
   * best fitting plane. For option #4, the user must also specify a plane
   * Origin and Normal. For all options, after filter execution, the plane
   * onto which the points are projected is returned in the Origin and Normal
   * data members. Note that the BEST_COORDINATE_PLANE first performs a plane
   * fitting, and then selects the x, y, or z coordinate plane most orthogonal
   * to the fitted plane normal.
   */
  vtkSetClampMacro(ProjectionType, int, X_PLANE, BEST_FIT_PLANE);
  vtkGetMacro(ProjectionType, int);
  void SetProjectionTypeToXPlane() { this->SetProjectionType(X_PLANE); }
  void SetProjectionTypeToYPlane() { this->SetProjectionType(Y_PLANE); }
  void SetProjectionTypeToZPlane() { this->SetProjectionType(Z_PLANE); }
  void SetProjectionTypeToSpecifiedPlane() { this->SetProjectionType(SPECIFIED_PLANE); }
  void SetProjectionTypeToBestCoordinatePlane() { this->SetProjectionType(BEST_COORDINATE_PLANE); }
  void SetProjectionTypeToBestFitPlane() { this->SetProjectionType(BEST_FIT_PLANE); }
  ///@}

  ///@{
  /**
   * Set/Get the plane normal and origin. On input, these methods are used to specify
   * the plane to use for projection (if the ProjectionType==SpecifiedPlane); and
   * on output the methods return the plane on which the points were projected.
   */
  vtkSetVector3Macro(Origin, double);
  vtkSetVector3Macro(Normal, double);
  vtkGetVector3Macro(Origin, double);
  vtkGetVector3Macro(Normal, double);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output points. See the
   * documentation for the vtkAlgorithm::Precision enum for an explanation of
   * the available precision settings. By default, the output precision is
   * DEFAULT_PRECISION (i.e., the input and output types are the same) - this
   * can cause issues if projecting integral point types.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkProjectPointsToPlane();
  ~vtkProjectPointsToPlane() override = default;

  int ProjectionType;
  double Origin[3];
  double Normal[3];
  int OutputPointsPrecision;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkProjectPointsToPlane(const vtkProjectPointsToPlane&) = delete;
  void operator=(const vtkProjectPointsToPlane&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
