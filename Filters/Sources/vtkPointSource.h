// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPointSource
 * @brief   create a random cloud of points
 *
 * vtkPointSource is a source object that creates a user-specified number of
 * points within a specified radius about a specified center point.  By
 * default the location of the points is random within the sphere. It is also
 * possible to generate random points only on the surface of the sphere; or a
 * exponential distribution weighted towards the center point. The output
 * PolyData has the specified number of points and a single cell - a
 * vtkPolyVertex cell referencing all of the points.
 *
 * @note
 * If Lambda set to zero, a uniform distribution is used. Negative lambda
 * values are allowed, but the distribution function becomes inverted.
 *
 * @note
 * If you desire to create complex point clouds (e.g., stellar distributions)
 * then use multiple point sources and then append them together using the
 * an append filter (e.g., vtkAppendPolyData).
 *
 * @sa
 * vtkAppendPolyData
 */

#ifndef vtkPointSource_h
#define vtkPointSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#define VTK_POINT_SHELL 0
#define VTK_POINT_UNIFORM 1
#define VTK_POINT_EXPONENTIAL 2

VTK_ABI_NAMESPACE_BEGIN
class vtkRandomSequence;

class VTKFILTERSSOURCES_EXPORT vtkPointSource : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkPointSource* New();
  vtkTypeMacro(vtkPointSource, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set the number of points to generate.
   */
  vtkSetClampMacro(NumberOfPoints, vtkIdType, 1, VTK_ID_MAX);
  vtkGetMacro(NumberOfPoints, vtkIdType);
  ///@}

  ///@{
  /**
   * Set the center of the point cloud.
   */
  vtkSetVector3Macro(Center, double);
  vtkGetVectorMacro(Center, double, 3);
  ///@}

  ///@{
  /**
   * Set the radius of the point cloud.  If you are
   * generating a Gaussian distribution, then this is
   * the standard deviation for each of x, y, and z.
   */
  vtkSetClampMacro(Radius, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Radius, double);
  ///@}

  ///@{
  /**
   * Specify the point distribution to use.  The default is a uniform
   * distribution.  The shell distribution produces random points on the
   * surface of the sphere Radius=constant, no points in the interior.  The
   * exponential distribution creates more points towards the center point
   * weighted by the exponential function.
   */
  vtkSetClampMacro(Distribution, int, VTK_POINT_SHELL, VTK_POINT_EXPONENTIAL);
  void SetDistributionToShell() { this->SetDistribution(VTK_POINT_SHELL); }
  void SetDistributionToUniform() { this->SetDistribution(VTK_POINT_UNIFORM); }
  void SetDistributionToExponential() { this->SetDistribution(VTK_POINT_EXPONENTIAL); }
  vtkGetMacro(Distribution, int);
  ///@}

  ///@{
  /**
   * If the distribution is set to exponential, then Lambda is used to
   * scale the exponential distribution defined by
   * f(x) = Lambda*exp(-Lambda*radius) where the radius is the distance
   * from the Center of the point source. By default, the value of Lambda
   * is Lambda=1.0.
   */
  vtkSetMacro(Lambda, double);
  vtkGetMacro(Lambda, double);
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

  ///@{
  /**
   * Set/Get a random sequence generator.
   * By default, the generator in vtkMath is used to maintain backwards
   * compatibility.
   */
  virtual void SetRandomSequence(vtkRandomSequence* randomSequence);
  vtkGetObjectMacro(RandomSequence, vtkRandomSequence);
  ///@}

protected:
  vtkPointSource(vtkIdType numPts = 10);
  ~vtkPointSource() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double Random();

  vtkIdType NumberOfPoints;
  double Center[3];
  double Radius;
  int Distribution;
  double Lambda;
  int OutputPointsPrecision;
  vtkRandomSequence* RandomSequence;

private:
  vtkPointSource(const vtkPointSource&) = delete;
  void operator=(const vtkPointSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
