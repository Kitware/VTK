// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGoldenBallSource
 * @brief   Create a faceted approximation to a ball (i.e., a solid sphere).
 *
 * vtkGoldenBallSource creates a ball (represented by tetrahedra) of specified
 * radius and center. The resolution (number of points) can be specified
 * and will be equal to the number of tetrahedra in the output approximation.
 *
 * The name derives from the golden angle (pi * (sqrt(5) - 1)) used to space points
 * circumferentially so they are approximately equidistant from their neighbors.
 * For this reason, the technique is sometimes called the "Fibonacci spiral,"
 * after the planar spiral shape which gets projected to the sphere.
 */

#ifndef vtkGoldenBallSource_h
#define vtkGoldenBallSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSSOURCES_EXPORT vtkGoldenBallSource : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkGoldenBallSource, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct sphere with radius=0.5 and default resolution 20.
   */
  static vtkGoldenBallSource* New();

  //@{
  /**
   * Set radius of sphere. Default is .5.
   */
  vtkSetClampMacro(Radius, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Set the center of the sphere. Default is 0,0,0.
   */
  vtkSetVector3Macro(Center, double);
  vtkGetVectorMacro(Center, double, 3);
  //@}

  //@{
  /**
   * Set the number of points used to approximate the sphere.
   * The minimum is 4, the default is 20, and there is no maximum.
   */
  vtkSetClampMacro(Resolution, int, 4, VTK_INT_MAX);
  vtkGetMacro(Resolution, int);
  //@}

  //@{
  /**
   * Set/get whether to include a point at the center of the ball.
   * The default is not to include the center.
   */
  vtkSetMacro(IncludeCenterPoint, int);
  vtkGetMacro(IncludeCenterPoint, int);
  vtkBooleanMacro(IncludeCenterPoint, int);
  //@}

  //@{
  /**
   * Set/get whether to include "normal" vectors at each point.
   * The default is to include normals. These are vectors of unit
   * length which point outward from the center of the ball. The
   * center point (if included) has a zero-length vector.
   */
  vtkSetMacro(GenerateNormals, int);
  vtkGetMacro(GenerateNormals, int);
  vtkBooleanMacro(GenerateNormals, int);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  vtkGoldenBallSource();
  ~vtkGoldenBallSource() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double Radius;
  double Center[3];
  int Resolution;
  int IncludeCenterPoint;
  int GenerateNormals;
  int OutputPointsPrecision;

private:
  vtkGoldenBallSource(const vtkGoldenBallSource&) = delete;
  void operator=(const vtkGoldenBallSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
