/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourValues.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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

#include "vtkIntArray.h"

class vtkFloatArray;

class VTK_EXPORT vtkContourValues : public vtkObject
{
public:
  vtkContourValues();
  ~vtkContourValues();
  static vtkContourValues *New() {return new vtkContourValues;};
  const char *GetClassName() {return "vtkContourValues";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void SetValue(int i, float value);
  float GetValue(int i);
  float *GetValues();
  void GetValues(float *contourValues);

  void SetNumberOfContours(const int number);
  int GetNumberOfContours();

  void GenerateValues(int numContours, float range[2]);
  void GenerateValues(int numContours, float rangeStart, float rangeEnd);

protected:
  vtkFloatArray *Contours;

};

#endif
