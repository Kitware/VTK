// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDiskSource
 * @brief   create a disk with hole in center
 *
 * vtkDiskSource creates a polygonal disk with a hole in the center. The
 * disk has zero height. The user can specify the inner and outer radius
 * of the disk, the radial and circumferential resolution of the
 * polygonal representation, and the center and plane normal of the disk
 * (i.e., the center and disk normal control the position and orientation
 * of the disk).
 *
 * @sa
 * vtkLinearExtrusionFilter
 */

#ifndef vtkDiskSource_h
#define vtkDiskSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkTransform;

class VTKFILTERSSOURCES_EXPORT vtkDiskSource : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods to  instantiate the class, obtain type information,
   * and print the state of the object.
   */
  static vtkDiskSource* New();
  vtkTypeMacro(vtkDiskSource, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Specify inner radius of hole in disk.
   */
  vtkSetClampMacro(InnerRadius, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(InnerRadius, double);
  ///@}

  ///@{
  /**
   * Specify outer radius of disk.
   */
  vtkSetClampMacro(OuterRadius, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(OuterRadius, double);
  ///@}

  ///@{
  /**
   * Set the number of points in radius direction.
   */
  vtkSetClampMacro(RadialResolution, int, 1, VTK_INT_MAX);
  vtkGetMacro(RadialResolution, int);
  ///@}

  ///@{
  /**
   * Set the number of points in circumferential direction.
   */
  vtkSetClampMacro(CircumferentialResolution, int, 3, VTK_INT_MAX);
  vtkGetMacro(CircumferentialResolution, int);
  ///@}

  ///@{
  /**
   * Set the center of the disk. The default is (0, 0, 0).
   */
  vtkSetVector3Macro(Center, double);
  vtkGetVectorMacro(Center, double, 3);
  ///@}

  ///@{
  /**
   * Set/get plane normal. The default is (0, 0, 1).
   */
  vtkSetVector3Macro(Normal, double);
  vtkGetVectorMacro(Normal, double, 3);
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
  vtkDiskSource();
  ~vtkDiskSource() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  vtkSmartPointer<vtkTransform> GetTransformation();
  double InnerRadius;
  double OuterRadius;
  double Center[3];
  double Normal[3];
  int RadialResolution;
  int CircumferentialResolution;
  int OutputPointsPrecision;

private:
  vtkDiskSource(const vtkDiskSource&) = delete;
  void operator=(const vtkDiskSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
