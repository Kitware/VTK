/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSynchronizedTemplates2D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

     THIS CLASS IS PATENT PENDING.

     Application of this software for commercial purposes requires 
     a license grant from Kitware. Contact:
         Ken Martin
         Kitware
         469 Clifton Corporate Parkway,
         Clifton Park, NY 12065
         Phone:1-518-371-3971 
     for more information.

=========================================================================*/
// .NAME vtkSynchronizedTemplates2D - generate isoline(s) from a structured points set
// .SECTION Description
// vtkSynchronizedTemplates2D is a 2D implementation of the synchronized 
// template algorithm. Note that vtkContourFilter will automatically
// use this class when appropriate if vtk was built with patents.

// .SECTION Caveats
// This filter is specialized to 2D images.

// .SECTION See Also
// vtkContourFilter vtkSynchronizedTemplates3D

#ifndef __vtkSynchronizedTemplates2D_h
#define __vtkSynchronizedTemplates2D_h

#include "vtkPolyDataSource.h"
#include "vtkContourValues.h"

class vtkImageData;
class vtkKitwareContourFilter;

class VTK_PATENTED_EXPORT vtkSynchronizedTemplates2D : public vtkPolyDataSource
{
public:
  static vtkSynchronizedTemplates2D *New();
  vtkTypeRevisionMacro(vtkSynchronizedTemplates2D,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();
  
// Description:
  // Because we delegate to vtkContourValues
  unsigned long int GetMTime();

  // Description:
  // Set a particular contour value at contour number i. The index i ranges 
  // between 0<=i<NumberOfContours.
  void SetValue(int i, float value) {this->ContourValues->SetValue(i,value);}

  // Description:
  // Get the ith contour value.
  float GetValue(int i) {return this->ContourValues->GetValue(i);}

  // Description:
  // Get a pointer to an array of contour values. There will be
  // GetNumberOfContours() values in the list.
  float *GetValues() {return this->ContourValues->GetValues();}

  // Description:
  // Fill a supplied list with contour values. There will be
  // GetNumberOfContours() values in the list. Make sure you allocate
  // enough memory to hold the list.
  void GetValues(float *contourValues) {
    this->ContourValues->GetValues(contourValues);}

  // Description:
  // Set the number of contours to place into the list. You only really
  // need to use this method to reduce list size. The method SetValue()
  // will automatically increase list size as needed.
  void SetNumberOfContours(int number) {
    this->ContourValues->SetNumberOfContours(number);}

  // Description:
  // Get the number of contours in the list of contour values.
  int GetNumberOfContours() {
    return this->ContourValues->GetNumberOfContours();}

  // Description:
  // Generate numContours equally spaced contour values between specified
  // range. Contour values will include min/max range values.
  void GenerateValues(int numContours, float range[2]) {
    this->ContourValues->GenerateValues(numContours, range);}

  // Description:
  // Generate numContours equally spaced contour values between specified
  // range. Contour values will include min/max range values.
  void GenerateValues(int numContours, float rangeStart, float rangeEnd)
    {this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);}

  // Description:
  // Option to set the point scalars of the output.  The scalars will be the 
  // iso value of course.  By default this flag is on.
  vtkSetMacro(ComputeScalars,int);
  vtkGetMacro(ComputeScalars,int);
  vtkBooleanMacro(ComputeScalars,int);

  // Description:
  // If you want to contour by an arbitrary array, then set its name here.
  // By default this in NULL and the filter will use the active scalar array.
  vtkGetStringMacro(InputScalarsSelection);
  void SelectInputScalars(const char *fieldName) 
    {this->SetInputScalarsSelection(fieldName);}
  
protected:
  vtkSynchronizedTemplates2D();
  ~vtkSynchronizedTemplates2D();

  void Execute();
  vtkContourValues *ContourValues;

  int ComputeScalars;

  char *InputScalarsSelection;
  vtkSetStringMacro(InputScalarsSelection);

private:
  //BTX
  friend class VTK_PATENTED_EXPORT vtkKitwareContourFilter;
  //ETX
private:
  vtkSynchronizedTemplates2D(const vtkSynchronizedTemplates2D&);  // Not implemented.
  void operator=(const vtkSynchronizedTemplates2D&);  // Not implemented.
};


#endif

