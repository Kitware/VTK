// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGridSynchronizedTemplates3D
 * @brief   generate isosurface from structured grids
 *
 *
 * vtkGridSynchronizedTemplates3D is a 3D implementation of the synchronized
 * template algorithm.
 *
 * @warning
 * This filter is specialized to 3D grids.
 *
 * @sa
 * vtkContourFilter vtkSynchronizedTemplates3D
 */

#ifndef vtkGridSynchronizedTemplates3D_h
#define vtkGridSynchronizedTemplates3D_h

#include "vtkContourValues.h"     // Because it passes all the calls to it
#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkStructuredGrid;

class VTKFILTERSCORE_EXPORT vtkGridSynchronizedTemplates3D : public vtkPolyDataAlgorithm
{
public:
  static vtkGridSynchronizedTemplates3D* New();
  vtkTypeMacro(vtkGridSynchronizedTemplates3D, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Because we delegate to vtkContourValues
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set/Get the computation of normals. Normal computation is fairly
   * expensive in both time and storage. If the output data will be
   * processed by filters that modify topology or geometry, it may be
   * wise to turn Normals and Gradients off.
   */
  vtkSetMacro(ComputeNormals, vtkTypeBool);
  vtkGetMacro(ComputeNormals, vtkTypeBool);
  vtkBooleanMacro(ComputeNormals, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the computation of gradients. Gradient computation is
   * fairly expensive in both time and storage. Note that if
   * ComputeNormals is on, gradients will have to be calculated, but
   * will not be stored in the output dataset.  If the output data
   * will be processed by filters that modify topology or geometry, it
   * may be wise to turn Normals and Gradients off.
   */
  vtkSetMacro(ComputeGradients, vtkTypeBool);
  vtkGetMacro(ComputeGradients, vtkTypeBool);
  vtkBooleanMacro(ComputeGradients, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Set/Get the computation of scalars.
   */
  vtkSetMacro(ComputeScalars, vtkTypeBool);
  vtkGetMacro(ComputeScalars, vtkTypeBool);
  vtkBooleanMacro(ComputeScalars, vtkTypeBool);
  ///@}

  ///@{
  /**
   * If this is enabled (by default), the output will be triangles
   * otherwise, the output will be the intersection polygons
   */
  vtkSetMacro(GenerateTriangles, vtkTypeBool);
  vtkGetMacro(GenerateTriangles, vtkTypeBool);
  vtkBooleanMacro(GenerateTriangles, vtkTypeBool);
  ///@}

  /**
   * Set a particular contour value at contour number i. The index i ranges
   * between 0<=i<NumberOfContours.
   */
  void SetValue(int i, double value) { this->ContourValues->SetValue(i, value); }

  /**
   * Get the ith contour value.
   */
  double GetValue(int i) { return this->ContourValues->GetValue(i); }

  /**
   * Get a pointer to an array of contour values. There will be
   * GetNumberOfContours() values in the list.
   */
  double* GetValues() { return this->ContourValues->GetValues(); }

  /**
   * Fill a supplied list with contour values. There will be
   * GetNumberOfContours() values in the list. Make sure you allocate
   * enough memory to hold the list.
   */
  void GetValues(double* contourValues) { this->ContourValues->GetValues(contourValues); }

  /**
   * Set the number of contours to place into the list. You only really
   * need to use this method to reduce list size. The method SetValue()
   * will automatically increase list size as needed.
   */
  void SetNumberOfContours(int number) { this->ContourValues->SetNumberOfContours(number); }

  /**
   * Get the number of contours in the list of contour values.
   */
  vtkIdType GetNumberOfContours() { return this->ContourValues->GetNumberOfContours(); }

  /**
   * Generate numContours equally spaced contour values between specified
   * range. Contour values will include min/max range values.
   */
  void GenerateValues(int numContours, double range[2])
  {
    this->ContourValues->GenerateValues(numContours, range);
  }

  /**
   * Generate numContours equally spaced contour values between specified
   * range. Contour values will include min/max range values.
   */
  void GenerateValues(int numContours, double rangeStart, double rangeEnd)
  {
    this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
  }

  /**
   * Main execution.
   */
  void ThreadedExecute(
    vtkStructuredGrid* input, vtkInformationVector** inputVector, vtkInformation* outInfo);

  /**
   * This filter will initiate streaming so that no piece requested
   * from the input will be larger than this value (KiloBytes).
   */
  void SetInputMemoryLimit(long limit);

  ///@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the vtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  vtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkGridSynchronizedTemplates3D();
  ~vtkGridSynchronizedTemplates3D() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkTypeBool ComputeNormals;
  vtkTypeBool ComputeGradients;
  vtkTypeBool ComputeScalars;
  vtkTypeBool GenerateTriangles;

  vtkContourValues* ContourValues;

  int MinimumPieceSize[3];
  int OutputPointsPrecision;

private:
  vtkGridSynchronizedTemplates3D(const vtkGridSynchronizedTemplates3D&) = delete;
  void operator=(const vtkGridSynchronizedTemplates3D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
