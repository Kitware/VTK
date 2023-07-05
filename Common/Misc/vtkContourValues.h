// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkContourValues
 * @brief   helper object to manage setting and generating contour values
 *
 * vtkContourValues is a general class to manage the creation, generation,
 * and retrieval of contour values. This class serves as a helper class for
 * contouring classes, or those classes operating on lists of contour values.
 *
 * @sa
 * vtkContourFilter
 */

#ifndef vtkContourValues_h
#define vtkContourValues_h

#include "vtkCommonMiscModule.h" // For export macro
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;

class VTKCOMMONMISC_EXPORT vtkContourValues : public vtkObject
{
public:
  /**
   * Construct object with a single contour value at 0.0.
   */
  static vtkContourValues* New();

  vtkTypeMacro(vtkContourValues, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the ith contour value.
   */
  void SetValue(int i, double value);

  /**
   * Get the ith contour value. The return value will be clamped if the
   * index i is out of range.
   */
  double GetValue(int i);

  /**
   * Return a pointer to a list of contour values. The contents of the
   * list will be garbage if the number of contours <= 0.
   */
  double* GetValues();

  /**
   * Fill a supplied list with contour values. Make sure you've
   * allocated memory of size GetNumberOfContours().
   */
  void GetValues(double* contourValues);

  /**
   * Set the number of contours to place into the list. You only really
   * need to use this method to reduce list size. The method SetValue()
   * will automatically increase list size as needed.
   */
  void SetNumberOfContours(int number);

  /**
   * Return the number of contours in the
   */
  int GetNumberOfContours();

  /**
   * Generate numContours equally spaced contour values between specified
   * range. Contour values will include min/max range values.
   */
  void GenerateValues(int numContours, double range[2]);

  /**
   * Generate numContours equally spaced contour values between specified
   * range. Contour values will include min/max range values.
   */
  void GenerateValues(int numContours, double rangeStart, double rangeEnd);

  /**
   * Copy contours.
   */
  void DeepCopy(vtkContourValues* other);

protected:
  vtkContourValues();
  ~vtkContourValues() override;

  vtkDoubleArray* Contours;

private:
  vtkContourValues(const vtkContourValues&) = delete;
  void operator=(const vtkContourValues&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
