/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourValues.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkContourValues - helper object to manage setting and generating contour values
// .SECTION Description
// vtkContourValues is a general class to manage the creation, generation,
// and retrieval of contour values. This class serves as a helper class for
// contouring classes, or those classes operating on lists of contour values.

// .SECTION See Also
// vtkContourFilter

#ifndef __vtkContourValues_h
#define __vtkContourValues_h

#include "vtkObject.h"
#include "vtkIntArray.h"

class vtkFloatArray;

class VTK_COMMON_EXPORT vtkContourValues : public vtkObject
{
public:
  // Description:
  // Construct object with a single contour value at 0.0.
  static vtkContourValues *New();

  vtkTypeMacro(vtkContourValues,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the ith contour value.
  void SetValue(int i, float value);

  // Description:
  // Get the ith contour value. The return value will be clamped if the
  // index i is out of range.
  float GetValue(int i);

  // Description:
  // Return a pointer to a list of contour values. The contents of the
  // list will be garbage if the number of contours <= 0.
  float *GetValues();

  // Description:
  // Fill a supplied list with contour values. Make sure you've
  // allocated memory of size GetNumberOfContours().
  void GetValues(float *contourValues);

  // Description:
  // Set the number of contours to place into the list. You only really
  // need to use this method to reduce list size. The method SetValue()
  // will automatically increase list size as needed.
  void SetNumberOfContours(const int number);

  // Description:
  // Return the number of contours in the
  int GetNumberOfContours();

  // Description:
  // Generate numContours equally spaced contour values between specified
  // range. Contour values will include min/max range values.
  void GenerateValues(int numContours, float range[2]);

  // Description:
  // Generate numContours equally spaced contour values between specified
  // range. Contour values will include min/max range values.
  void GenerateValues(int numContours, float rangeStart, float rangeEnd);


protected:
  vtkContourValues();
  ~vtkContourValues();
  vtkContourValues(const vtkContourValues&);
  void operator=(const vtkContourValues&);

  vtkFloatArray *Contours;

};

#endif
