/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericCutter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericCutter - Cut a vtkGenericDataSet with user-specified
// implicit function
// .SECTION Description
// vtkGenericCutter is a filter to cut through data using any subclass of 
// vtkImplicitFunction. That is, a polygonal surface is created
// corresponding to the implicit function F(x,y,z) = value(s), where
// you can specify one or more values used to cut with.
//
// In VTK, cutting means reducing a cell of dimension N to a cut surface
// of dimension N-1. For example, a tetrahedron when cut by a plane (i.e.,
// vtkPlane implicit function) will generate triangles. (In comparison,
// clipping takes a N dimensional cell and creates N dimension primitives.)
//
// vtkGenericCutter is generally used to "slice-through" a dataset, generating
// a surface that can be visualized. It is also possible to use
// vtkGenericCutter to do a form of volume rendering. vtkGenericCutter does
// this by generating multiple cut surfaces (usually planes) which are ordered
// (and rendered) from back-to-front. The surfaces are set translucent to give
// a volumetric rendering effect.


// Caveats we can not control to iterate over cell against contour values
// as this is very expensive to iterate over cell, thus it should be done
// only once
// -> removed: Return the sorting procedure as a descriptive character string.


// .SECTION See Also
// vtkImplicitFunction vtkClipPolyData

#ifndef __vtkGenericCutter_h
#define __vtkGenericCutter_h

#include "vtkGenericDataSetToPolyDataFilter.h"

class vtkContourValues;

class vtkImplicitFunction;
class vtkPointLocator;

class VTK_GENERIC_FILTERING_EXPORT vtkGenericCutter : public vtkGenericDataSetToPolyDataFilter
{
public:
  vtkTypeRevisionMacro(vtkGenericCutter,vtkGenericDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with user-specified implicit function; initial value of 0.0; and
  // generating cut scalars turned off.
  static vtkGenericCutter *New();

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

  // Description:
  // Override GetMTime because we delegate to vtkContourValues and refer to
  // vtkImplicitFunction.
  unsigned long GetMTime();

  // Description
  // Specify the implicit function to perform the cutting.
  virtual void SetCutFunction(vtkImplicitFunction*);
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
  // Create default locator. Used to create one when none is specified. The 
  // locator is used to merge coincident points.
  void CreateDefaultLocator();

protected:
  vtkGenericCutter(vtkImplicitFunction *cf=NULL);
  ~vtkGenericCutter();

  void Execute();
  // Description:
  // Actual implementation of the cutter operation.
  void UnstructuredGridCutter();
  
  vtkImplicitFunction *CutFunction;
  vtkPointLocator *Locator;
  vtkContourValues *ContourValues;
  int GenerateCutScalars;
private:
  vtkGenericCutter(const vtkGenericCutter&);  // Not implemented.
  void operator=(const vtkGenericCutter&);  // Not implemented.
};

#endif


