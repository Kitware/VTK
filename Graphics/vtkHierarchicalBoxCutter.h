/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxCutter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalBoxCutter - Cut vtkHierarchicalBoxDataSet with user-specified implicit function
// .SECTION Description
// This filter uses vtkCutter to cut vtkHierarchicalBoxDataSet with 
// user-specified implicit function.

// .SECTION See Also
// vtkCutter

#ifndef __vtkHierarchicalBoxCutter_h
#define __vtkHierarchicalBoxCutter_h

#include "vtkHierarchicalBoxToPolyDataFilter.h"

class vtkCutter;
class vtkDataObject;
class vtkHierarchicalBoxDataSet;
class vtkImplicitFunction;

class VTK_GRAPHICS_EXPORT vtkHierarchicalBoxCutter : public vtkHierarchicalBoxToPolyDataFilter
{
public:
  static vtkHierarchicalBoxCutter *New();

  vtkTypeRevisionMacro(vtkHierarchicalBoxCutter,
                       vtkHierarchicalBoxToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set a particular contour value at contour number i. The index i ranges 
  // between 0<=i<NumberOfContours.
  void SetValue(int i, double value); 
  
  // Description:
  // Get the ith contour value.
  double GetValue(int i); 

  // Description:
  // Get a pointer to an array of contour values. There will be
  // GetNumberOfContours() values in the list.
  double *GetValues(); 

  // Description:
  // Fill a supplied list with contour values. There will be
  // GetNumberOfContours() values in the list. Make sure you allocate
  // enough memory to hold the list.
  void GetValues(double *contourValues);
  
  // Description:
  // Set the number of contours to place into the list. You only really
  // need to use this method to reduce list size. The method SetValue()
  // will automatically increase list size as needed.
  void SetNumberOfContours(int number); 

  // Description:
  // Get the number of contours in the list of contour values.
  int GetNumberOfContours(); 

  // Description:
  // Generate numContours equally spaced contour values between specified
  // range. Contour values will include min/max range values.
  void GenerateValues(int numContours, double range[2]); 

  // Description:
  // Generate numContours equally spaced contour values between specified
  // range. Contour values will include min/max range values.
  void GenerateValues(int numContours, double rangeStart, double rangeEnd); 

  // Description
  // Specify the implicit function to perform the cutting.
  void SetCutFunction(vtkImplicitFunction*);
  vtkImplicitFunction* GetCutFunction();

  // Description:
  // Override GetMTime because we delegate to vtkContourValues and refer to
  // vtkImplicitFunction.
  unsigned long GetMTime();

protected:
  vtkHierarchicalBoxCutter();
  ~vtkHierarchicalBoxCutter();

  virtual void ExecuteData(vtkDataObject*);

  vtkCutter* Cutter;

private:
  void InternalImageDataCopy(vtkHierarchicalBoxCutter *src);

private:
  vtkHierarchicalBoxCutter(const vtkHierarchicalBoxCutter&);  // Not implemented. 
  void operator=(const vtkHierarchicalBoxCutter&);  // Not implemented.
};


#endif



