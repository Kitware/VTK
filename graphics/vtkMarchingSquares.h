/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarchingSquares.h
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

#include "vtkStructuredPointsToPolyDataFilter.hh"

#define VTK_MAX_CONTOURS 256

class vtkMarchingSquares : public vtkStructuredPointsToPolyDataFilter
{
public:
  vtkMarchingSquares();
  char *GetClassName() {return "vtkMarchingSquares";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the i-j-k index range which define a plane on which to generate 
  // contour lines. Using this ivar it is possible to input a 3D volume
  // directly and then generate contour lines on one of the i-j-k planes, or 
  // a portion of a plane.
  vtkSetVectorMacro(ImageRange,int,6);
  vtkGetVectorMacro(ImageRange,int,6);
  void SetImageRange(int imin, int imax, int jmin, int jmax, int kmin, int kmax);

  void SetValue(int i, float value);
  float GetValue(int i) {return this->Values[i];};

  // Description:
  // Retrieve an array of contour values (NumberOfContours values will be returned).
  vtkGetVectorMacro(Values,float,VTK_MAX_CONTOURS);

  // Description:
  // Set/get the number of contour values. The number of values set (using SetValue)
  // should match the NumberOfContours ivar value.
  vtkSetMacro(NumberOfContours,int);
  vtkGetMacro(NumberOfContours,int);

  void GenerateValues(int numContours, float range[2]);
  void GenerateValues(int numContours, float range1, float range2);

  void SetLocator(vtkPointLocator *locator);
  void SetLocator(vtkPointLocator& locator) {this->SetLocator(&locator);};
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Create default locator. Used to create one when none is specified. The locator is
  // used to merge coincident points.
  void CreateDefaultLocator();

protected:
  void Execute();

  float Values[VTK_MAX_CONTOURS];
  int NumberOfContours;
  float Range[2];
  int ImageRange[6];
  vtkPointLocator *Locator;
  int SelfCreatedLocator;

};

#endif

