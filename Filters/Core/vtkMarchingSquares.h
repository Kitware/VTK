// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMarchingSquares
 * @brief   generate isoline(s) from structured points set
 *
 * vtkMarchingSquares is a filter that takes as input a structured points set
 * and generates on output one or more isolines.  One or more contour values
 * must be specified to generate the isolines.  Alternatively, you can specify
 * a min/max scalar range and the number of contours to generate a series of
 * evenly spaced contour values.
 *
 * To generate contour lines the input data must be of topological dimension 2
 * (i.e., an image). If not, you can use the ImageRange ivar to select an
 * image plane from an input volume. This avoids having to extract a plane first
 * (using vtkExtractSubVolume).  The filter deals with this by first
 * trying to use the input data directly, and if not a 2D image, then uses the
 * ImageRange ivar to reduce it to an image.
 *
 * @warning
 * This filter is specialized to images. If you are interested in
 * contouring other types of data, use the general vtkContourFilter.
 * @sa
 * vtkContourFilter vtkMarchingCubes vtkSliceCubes vtkDividingCubes
 */

#ifndef vtkMarchingSquares_h
#define vtkMarchingSquares_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

#include "vtkContourValues.h" // Passes calls to vtkContourValues

VTK_ABI_NAMESPACE_BEGIN
class vtkImageData;
class vtkIncrementalPointLocator;

class VTKFILTERSCORE_EXPORT vtkMarchingSquares : public vtkPolyDataAlgorithm
{
public:
  static vtkMarchingSquares* New();
  vtkTypeMacro(vtkMarchingSquares, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the i-j-k index range which define a plane on which to generate
   * contour lines. Using this ivar it is possible to input a 3D volume
   * directly and then generate contour lines on one of the i-j-k planes, or
   * a portion of a plane.
   */
  vtkSetVectorMacro(ImageRange, int, 6);
  vtkGetVectorMacro(ImageRange, int, 6);
  void SetImageRange(int imin, int imax, int jmin, int jmax, int kmin, int kmax);
  ///@}

  ///@{
  /**
   * Methods to set contour values
   */
  void SetValue(int i, double value);
  double GetValue(int i);
  double* GetValues();
  void GetValues(double* contourValues);
  void SetNumberOfContours(int number);
  vtkIdType GetNumberOfContours();
  void GenerateValues(int numContours, double range[2]);
  void GenerateValues(int numContours, double rangeStart, double rangeEnd);
  ///@}

  /**
   * Because we delegate to vtkContourValues
   */
  vtkMTimeType GetMTime() override;

  void SetLocator(vtkIncrementalPointLocator* locator);
  vtkGetObjectMacro(Locator, vtkIncrementalPointLocator);

  /**
   * Create default locator. Used to create one when none is specified.
   * The locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

protected:
  vtkMarchingSquares();
  ~vtkMarchingSquares() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkContourValues* ContourValues;
  int ImageRange[6];
  vtkIncrementalPointLocator* Locator;

private:
  vtkMarchingSquares(const vtkMarchingSquares&) = delete;
  void operator=(const vtkMarchingSquares&) = delete;
};

/**
 * Set a particular contour value at contour number i. The index i ranges
 * between 0<=i<NumberOfContours.
 */
inline void vtkMarchingSquares::SetValue(int i, double value)
{
  this->ContourValues->SetValue(i, value);
}

/**
 * Get the ith contour value.
 */
inline double vtkMarchingSquares::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

/**
 * Get a pointer to an array of contour values. There will be
 * GetNumberOfContours() values in the list.
 */
inline double* vtkMarchingSquares::GetValues()
{
  return this->ContourValues->GetValues();
}

/**
 * Fill a supplied list with contour values. There will be
 * GetNumberOfContours() values in the list. Make sure you allocate
 * enough memory to hold the list.
 */
inline void vtkMarchingSquares::GetValues(double* contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

/**
 * Set the number of contours to place into the list. You only really
 * need to use this method to reduce list size. The method SetValue()
 * will automatically increase list size as needed.
 */
inline void vtkMarchingSquares::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

/**
 * Get the number of contours in the list of contour values.
 */
inline vtkIdType vtkMarchingSquares::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void vtkMarchingSquares::GenerateValues(int numContours, double range[2])
{
  this->ContourValues->GenerateValues(numContours, range);
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void vtkMarchingSquares::GenerateValues(int numContours, double rangeStart, double rangeEnd)
{
  this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
}

VTK_ABI_NAMESPACE_END
#endif
