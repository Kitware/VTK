/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourValues.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class vtkDoubleArray;

class VTKCOMMONMISC_EXPORT vtkContourValues : public vtkObject
{
public:
  /**
   * Construct object with a single contour value at 0.0.
   */
  static vtkContourValues *New();

  vtkTypeMacro(vtkContourValues,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

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
  double *GetValues();

  /**
   * Fill a supplied list with contour values. Make sure you've
   * allocated memory of size GetNumberOfContours().
   */
  void GetValues(double *contourValues);

  /**
   * Set the number of contours to place into the list. You only really
   * need to use this method to reduce list size. The method SetValue()
   * will automatically increase list size as needed.
   */
  void SetNumberOfContours(const int number);

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


protected:
  vtkContourValues();
  ~vtkContourValues() VTK_OVERRIDE;

  vtkDoubleArray *Contours;

private:
  vtkContourValues(const vtkContourValues&) VTK_DELETE_FUNCTION;
  void operator=(const vtkContourValues&) VTK_DELETE_FUNCTION;
};

#endif
