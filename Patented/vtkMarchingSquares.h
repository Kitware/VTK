/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarchingSquares.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

     THIS CLASS IS PATENTED UNDER UNITED STATES PATENT NUMBER 4,710,876
     "System and Method for the Display of Surface Structures Contained
     Within the Interior Region of a Solid Body".
     Application of this software for commercial purposes requires 
     a license grant from GE. Contact:

         Carl B. Horton
         Sr. Counsel, Intellectual Property
         3000 N. Grandview Blvd., W-710
         Waukesha, WI  53188
         Phone:  (262) 513-4022
         E-Mail: Carl.Horton@med.ge.com

     for more information.

=========================================================================*/
// .NAME vtkMarchingSquares - generate isoline(s) from structured points set
// .SECTION Description
// vtkMarchingSquares is a filter that takes as input a structured points set
// and generates on output one or more isolines.  One or more contour values 
// must be specified to generate the isolines.  Alternatively, you can specify 
// a min/max scalar range and the number of contours to generate a series of 
// evenly spaced contour values. 
//
// To generate contour lines the input data must be of topological dimension 2 
// (i.e., an image). If not, you can use the ImageRange ivar to select an
// image plane from an input volume. This avoids having to extract a plane first
// (using vtkExtractSubVolume).  The filter deals with this by first
// trying to use the input data directly, and if not a 2D image, then uses the 
// ImageRange ivar to reduce it to an image.

// .SECTION Caveats
// This filter is specialized to images. If you are interested in 
// contouring other types of data, use the general vtkContourFilter.
// .SECTION See Also
// vtkContourFilter vtkMarchingCubes vtkSliceCubes vtkDividingCubes

#ifndef __vtkMarchingSquares_h
#define __vtkMarchingSquares_h

#include "vtkPolyDataSource.h"
#include "vtkImageData.h"
#include "vtkContourValues.h"

class VTK_PATENTED_EXPORT vtkMarchingSquares : public vtkPolyDataSource
{
public:
  static vtkMarchingSquares *New();
  vtkTypeRevisionMacro(vtkMarchingSquares,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  void SetInput(vtkImageData *input);
  vtkImageData *GetInput();  
  
  // Description:
  // Set/Get the i-j-k index range which define a plane on which to generate 
  // contour lines. Using this ivar it is possible to input a 3D volume
  // directly and then generate contour lines on one of the i-j-k planes, or 
  // a portion of a plane.
  vtkSetVectorMacro(ImageRange,int,6);
  vtkGetVectorMacro(ImageRange,int,6);
  void SetImageRange(int imin, int imax, int jmin, int jmax, 
                     int kmin, int kmax);

  // Methods to set contour values
  void SetValue(int i, float value);
  float GetValue(int i);
  float *GetValues();
  void GetValues(float *contourValues);
  void SetNumberOfContours(int number);
  int GetNumberOfContours();
  void GenerateValues(int numContours, float range[2]);
  void GenerateValues(int numContours, float rangeStart, float rangeEnd);

  // Because we delegate to vtkContourValues
  unsigned long int GetMTime();

  void SetLocator(vtkPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified. 
  // The locator is used to merge coincident points.
  void CreateDefaultLocator();

protected:
  vtkMarchingSquares();
  ~vtkMarchingSquares();

  void Execute();

  vtkContourValues *ContourValues;
  int ImageRange[6];
  vtkPointLocator *Locator;
private:
  vtkMarchingSquares(const vtkMarchingSquares&);  // Not implemented.
  void operator=(const vtkMarchingSquares&);  // Not implemented.
};

// Description:
// Set a particular contour value at contour number i. The index i ranges 
// between 0<=i<NumberOfContours.
inline void vtkMarchingSquares::SetValue(int i, float value)
{this->ContourValues->SetValue(i,value);}

// Description:
// Get the ith contour value.
inline float vtkMarchingSquares::GetValue(int i)
{return this->ContourValues->GetValue(i);}

// Description:
// Get a pointer to an array of contour values. There will be
// GetNumberOfContours() values in the list.
inline float *vtkMarchingSquares::GetValues()
{return this->ContourValues->GetValues();}

// Description:
// Fill a supplied list with contour values. There will be
// GetNumberOfContours() values in the list. Make sure you allocate
// enough memory to hold the list.
inline void vtkMarchingSquares::GetValues(float *contourValues)
{this->ContourValues->GetValues(contourValues);}

// Description:
// Set the number of contours to place into the list. You only really
// need to use this method to reduce list size. The method SetValue()
// will automatically increase list size as needed.
inline void vtkMarchingSquares::SetNumberOfContours(int number)
{this->ContourValues->SetNumberOfContours(number);}

// Description:
// Get the number of contours in the list of contour values.
inline int vtkMarchingSquares::GetNumberOfContours()
{return this->ContourValues->GetNumberOfContours();}

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
inline void vtkMarchingSquares::GenerateValues(int numContours, float range[2])
{this->ContourValues->GenerateValues(numContours, range);}

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
inline void vtkMarchingSquares::GenerateValues(int numContours, float
                                             rangeStart, float rangeEnd)
{this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);}

#endif

