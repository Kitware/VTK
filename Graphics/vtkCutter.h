/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCutter.h
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
// .NAME vtkCutter - Cut vtkDataSet with user-specified implicit function
// .SECTION Description
// vtkCutter is a filter to cut through data using any subclass of 
// vtkImplicitFunction. That is, a polygonal surface is created
// corresponding to the implicit function F(x,y,z) = value(s), where
// you can specify one or more values used to cut with.
//
// In VTK, cutting means reducing a cell of dimension N to a cut surface
// of dimension N-1. For example, a tetrahedron when cut by a plane (i.e.,
// vtkPlane implicit function) will generate triangles. (Clipping takes
// a N dimensional cell and creates N dimension primitives.)
//
// vtkCutter is generally used to "slice-through" a dataset, generating
// a surface that can be visualized. It is also possible to use vtkCutter
// to do a form of volume rendering. vtkCutter does this by generating
// multiple cut surfaces (usually planes) which are ordered (and rendered)
// from back-to-front. The surfaces are set translucent to give a 
// volumetric rendering effect.

// .SECTION See Also
// vtkImplicitFunction vtkClipPolyData

#ifndef __vtkCutter_h
#define __vtkCutter_h

#include "vtkDataSetToPolyDataFilter.h"
#include "vtkContourValues.h"
#include "vtkImplicitFunction.h"

#define VTK_SORT_BY_VALUE 0
#define VTK_SORT_BY_CELL 1

class VTK_GRAPHICS_EXPORT vtkCutter : public vtkDataSetToPolyDataFilter
{
public:
  vtkTypeRevisionMacro(vtkCutter,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with user-specified implicit function; initial value of 0.0; and
  // generating cut scalars turned off.
  static vtkCutter *New();

  // Description:
  // Set a particular contour value at contour number i. The index i ranges 
  // between 0<=i<NumberOfContours.
  void SetValue(int i, float value) 
    {this->ContourValues->SetValue(i,value);}
  
  // Description:
  // Get the ith contour value.
  float GetValue(int i) 
    {return this->ContourValues->GetValue(i);}

  // Description:
  // Get a pointer to an array of contour values. There will be
  // GetNumberOfContours() values in the list.
  float *GetValues() 
    {return this->ContourValues->GetValues();}

  // Description:
  // Fill a supplied list with contour values. There will be
  // GetNumberOfContours() values in the list. Make sure you allocate
  // enough memory to hold the list.
  void GetValues(float *contourValues)
    {this->ContourValues->GetValues(contourValues);}
  
  // Description:
  // Set the number of contours to place into the list. You only really
  // need to use this method to reduce list size. The method SetValue()
  // will automatically increase list size as needed.
  void SetNumberOfContours(int number) 
    {this->ContourValues->SetNumberOfContours(number);}

  // Description:
  // Get the number of contours in the list of contour values.
  int GetNumberOfContours() 
    {return this->ContourValues->GetNumberOfContours();}

  // Description:
  // Generate numContours equally spaced contour values between specified
  // range. Contour values will include min/max range values.
  void GenerateValues(int numContours, float range[2]) 
    {this->ContourValues->GenerateValues(numContours, range);}

  // Description:
  // Generate numContours equally spaced contour values between specified
  // range. Contour values will include min/max range values.
  void GenerateValues(int numContours, float rangeStart, float rangeEnd) 
    {this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);}

  // Description:
  // Override GetMTime because we delegate to vtkContourValues and refer to
  // vtkImplicitFunction.
  unsigned long GetMTime();

  // Description
  // Specify the implicit function to perform the cutting.
  vtkSetObjectMacro(CutFunction,vtkImplicitFunction);
  vtkGetObjectMacro(CutFunction,vtkImplicitFunction);

  // Description:
  // If this flag is enabled, then the output scalar values will be
  // interpolated from the implicit function values, and not the input scalar
  // data.
  vtkSetMacro(GenerateCutScalars,int);
  vtkGetMacro(GenerateCutScalars,int);
  vtkBooleanMacro(GenerateCutScalars,int);

  // Description:
  // Specify a spatial locator for merging points. By default, 
  // an instance of vtkMergePoints is used.
  void SetLocator(vtkPointLocator *locator);
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
  void SetSortByToSortByValue() 
    {this->SetSortBy(VTK_SORT_BY_VALUE);}
  void SetSortByToSortByCell() 
    {this->SetSortBy(VTK_SORT_BY_CELL);}
  const char *GetSortByAsString();

  // Description:
  // Create default locator. Used to create one when none is specified. The 
  // locator is used to merge coincident points.
  void CreateDefaultLocator();

protected:
  vtkCutter(vtkImplicitFunction *cf=NULL);
  ~vtkCutter();

  void Execute();
  vtkImplicitFunction *CutFunction;
  
  vtkPointLocator *Locator;
  int SortBy;
  vtkContourValues *ContourValues;
  int GenerateCutScalars;
private:
  vtkCutter(const vtkCutter&);  // Not implemented.
  void operator=(const vtkCutter&);  // Not implemented.
};

// Description:
// Return the sorting procedure as a descriptive character string.
inline const char *vtkCutter::GetSortByAsString(void)
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


#endif


