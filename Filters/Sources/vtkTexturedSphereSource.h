// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTexturedSphereSource
 * @brief   create a sphere centered at the origin
 *
 * vtkTexturedSphereSource creates a polygonal sphere of specified radius
 * centered at the origin. The resolution (polygonal discretization) in both
 * the latitude (phi) and longitude (theta) directions can be specified.
 * It also is possible to create partial sphere by specifying maximum phi and
 * theta angles.
 */

#ifndef vtkTexturedSphereSource_h
#define vtkTexturedSphereSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSSOURCES_EXPORT vtkTexturedSphereSource : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkTexturedSphereSource, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct sphere with radius=0.5 and default resolution 8 in both Phi
   * and Theta directions.
   */
  static vtkTexturedSphereSource* New();

  ///@{
  /**
   * Set radius of sphere.
   */
  vtkSetClampMacro(Radius, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Radius, double);
  ///@}

  ///@{
  /**
   * Set the number of points in the longitude direction.
   */
  vtkSetClampMacro(ThetaResolution, int, 4, VTK_INT_MAX);
  vtkGetMacro(ThetaResolution, int);
  ///@}

  ///@{
  /**
   * Set the number of points in the latitude direction.
   */
  vtkSetClampMacro(PhiResolution, int, 4, VTK_INT_MAX);
  vtkGetMacro(PhiResolution, int);
  ///@}

  ///@{
  /**
   * Set the maximum longitude angle.
   */
  vtkSetClampMacro(Theta, double, 0.0, 360.0);
  vtkGetMacro(Theta, double);
  ///@}

  ///@{
  /**
   * Set the maximum latitude angle (0 is at north pole).
   */
  vtkSetClampMacro(Phi, double, 0.0, 180.0);
  vtkGetMacro(Phi, double);
  ///@}

  ///@{
  /**
   * Set/get the desired precision for the output points.
   * vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkTexturedSphereSource(int res = 8);
  ~vtkTexturedSphereSource() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  double Radius;
  double Theta;
  double Phi;
  int ThetaResolution;
  int PhiResolution;
  int OutputPointsPrecision;

private:
  vtkTexturedSphereSource(const vtkTexturedSphereSource&) = delete;
  void operator=(const vtkTexturedSphereSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
