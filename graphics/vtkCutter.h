/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCutter.h
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
// .NAME vtkCutter - Cut vtkDataSet with user-specified implicit function
// .SECTION Description
// vtkCutter is a filter to cut through data using any subclass of 
// vtkImplicitFunction. That is, a polygonal surface is created
// corresponding to the implicit function F(x,y,z) = value(s), where
// you can specify one or more values used to cut with.
// .SECTION See Also
// vtkImplicitFunction vtkClipPolyData

#ifndef __vtkCutter_h
#define __vtkCutter_h

#include "vtkDataSetToPolyFilter.h"
#include "vtkContourValues.h"

#define VTK_SORT_BY_VALUE 0
#define VTK_SORT_BY_CELL 1

class vtkImplicitFunction;

class VTK_EXPORT vtkCutter : public vtkDataSetToPolyFilter
{
public:
  vtkCutter(vtkImplicitFunction *cf=NULL);
  static vtkCutter *New() {return new vtkCutter;};
  char *GetClassName() {return "vtkCutter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Methods to set contour values
  void SetValue(int i, float value);
  float GetValue(int i);
  float *GetValues();
  void GetValues(float *contourValues);
  void SetNumberOfContours(int number);
  int GetNumberOfContours();
  void GenerateValues(int numContours, float range[2]);
  void GenerateValues(int numContours, float rangeStart, float rangeEnd);

  // Because we delegate to vtkContourValues & refer to vtkImplicitFunction
  unsigned long int GetMTime();

  // Description
  // Specify the implicit function to perform the cutting.
  vtkSetObjectMacro(CutFunction,vtkImplicitFunction);
  vtkGetObjectMacro(CutFunction,vtkImplicitFunction);

  // Description:
  // If this flag is enabled, then the output scalar values will be interpolated
  // from the implicit function values, and not the input scalar data.
  vtkSetMacro(GenerateCutScalars,int);
  vtkGetMacro(GenerateCutScalars,int);
  vtkBooleanMacro(GenerateCutScalars,int);

  void SetLocator(vtkPointLocator *locator);
  void SetLocator(vtkPointLocator& locator) {this->SetLocator(&locator);};
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Set the sorting order for the generated polydata. There are two
  // possibilities:
  //   Sort by value = 0 - This is the most efficient sort. For each cell,
  //      all contour values are processed. This is the default.
  //   Sort by cell = 1 - For each contour value, all cells are processed.
  //      This order should be used if the extracted polygons must be rendered
  //      in a back-to-front or front-to-back order. This is very problem 
  //      dependent.
  // For most applications, the default order is fine (and faster).
  vtkSetClampMacro(SortBy,int,VTK_SORT_BY_VALUE,VTK_SORT_BY_CELL);
  vtkGetMacro(SortBy,int);
  void SetSortByToSortByValue() {this->SetSortBy(VTK_SORT_BY_VALUE);};
  void SetSortByToSortByCell() {this->SetSortBy(VTK_SORT_BY_CELL);};
  char *GetSortByAsString();

  // Description:
  // Create default locator. Used to create one when none is specified. The 
  // locator is used to merge coincident points.
  void CreateDefaultLocator();

protected:
  void Execute();
  vtkImplicitFunction *CutFunction;
  
  vtkPointLocator *Locator;
  int SelfCreatedLocator;
  int SortBy;
  vtkContourValues *ContourValues;
  int GenerateCutScalars;
};

// Description:
// Return the sorting procedure as a descriptive character string.
inline char *vtkCutter::GetSortByAsString(void)
{
  if ( this->SortBy == VTK_SORT_BY_VALUE ) 
    {
    return "SortByValue";
    }
  else 
    {
    return "SortByCell";
    }
}

// Description:
// Set a particular contour value at contour number i. The index i ranges 
// between 0<=i<NumberOfContours.
inline void vtkCutter::SetValue(int i, float value)
{this->ContourValues->SetValue(i,value);}

// Description:
// Get the ith contour value.
inline float vtkCutter::GetValue(int i)
{return this->ContourValues->GetValue(i);};

// Description:
// Get a pointer to an array of contour values. There will be
// GetNumberOfContours() values in the list.
inline float *vtkCutter::GetValues()
{return this->ContourValues->GetValues();};

// Description:
// Fill a supplied list with contour values. There will be
// GetNumberOfContours() values in the list.Make sure you allocate
// enough memory to hold the list.
inline void vtkCutter::GetValues(float *contourValues)
{this->ContourValues->GetValues(contourValues);};

// Description:
// Set the number of contours to place into the list. You only really
// need to use this method to reduce list size. The method SetValue()
// will automatically increase list size as needed.
inline void vtkCutter::SetNumberOfContours(int number)
{this->ContourValues->SetNumberOfContours(number);};

// Description:
// Get the number of contours in the list of contour values.
inline int vtkCutter::GetNumberOfContours()
{return this->GetNumberOfContours();};

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
inline void vtkCutter::GenerateValues(int numContours, float range[2])
{this->ContourValues->GenerateValues(numContours, range);};

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
inline void vtkCutter::GenerateValues(int numContours, float
                                             rangeStart, float rangeEnd)
{this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);};

#endif


