/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMarchingCubes.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

    THIS CLASS IS PATENTED UNDER UNITED STATES PATENT NUMBER 4,710,876
    "System and Method for the Display of Surface Structures Contained
    Within the Interior Region of a Solid Body".
    Application of this software for commercial purposes requires 
    a license grant from GE. Contact:
        Jerald Roehling
        GE Licensing
        One Independence Way
        PO Box 2023
        Princeton, N.J. 08540
        phone 609-734-9823
        e-mail:Roehlinj@gerlmo.ge.com
    for more information.

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
// .NAME vtkMarchingCubes - generate isosurface(s) from volume
// .SECTION Description
// vtkMarchingCubes is a filter that takes as input a volume (e.g., 3D
// structured point set) and generates on output one or more isosurfaces.
// One or more contour values must be specified to generate the isosurfaces.
// Alternatively, you can specify a min/max scalar range and the number of
// contours to generate a series of evenly spaced contour values.

// .SECTION Caveats
// This filter is specialized to volumes. If you are interested in 
// contouring other types of data, use the general vtkContourFilter. If you
// want to contour an image (i.e., a volume slice), use vtkMarchingSquares.
// .SECTION See Also
// vtkContourFilter vtkSliceCubes vtkMarchingSquares vtkDividingCubes

#ifndef __vtkMarchingCubes_h
#define __vtkMarchingCubes_h

#include "vtkStructuredPointsToPolyDataFilter.h"
#include "vtkContourValues.h"

class VTK_EXPORT vtkMarchingCubes : public vtkStructuredPointsToPolyDataFilter
{
public:
  static vtkMarchingCubes *New();
  vtkTypeMacro(vtkMarchingCubes,vtkStructuredPointsToPolyDataFilter);
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

  // Because we delegate to vtkContourValues
  unsigned long int GetMTime();

  // Description:
  // Set/Get the computation of normals. Normal computation is fairly
  // expensive in both time and storage. If the output data will be
  // processed by filters that modify topology or geometry, it may be
  // wise to turn Normals and Gradients off.
  vtkSetMacro(ComputeNormals,int);
  vtkGetMacro(ComputeNormals,int);
  vtkBooleanMacro(ComputeNormals,int);

  // Description:
  // Set/Get the computation of gradients. Gradient computation is
  // fairly expensive in both time and storage. Note that if
  // ComputeNormals is on, gradients will have to be calculated, but
  // will not be stored in the output dataset.  If the output data
  // will be processed by filters that modify topology or geometry, it
  // may be wise to turn Normals and Gradients off.
  vtkSetMacro(ComputeGradients,int);
  vtkGetMacro(ComputeGradients,int);
  vtkBooleanMacro(ComputeGradients,int);

  // Description:
  // Set/Get the computation of scalars.
  vtkSetMacro(ComputeScalars,int);
  vtkGetMacro(ComputeScalars,int);
  vtkBooleanMacro(ComputeScalars,int);

  void SetLocator(vtkPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Create default locator. Used to create one when none is
  // specified. The locator is used to merge coincident points.
  void CreateDefaultLocator();

protected:
  vtkMarchingCubes();
  ~vtkMarchingCubes();
  vtkMarchingCubes(const vtkMarchingCubes&) {};
  void operator=(const vtkMarchingCubes&) {};

  void Execute();

  vtkContourValues *ContourValues;
  int ComputeNormals;
  int ComputeGradients;
  int ComputeScalars;
  vtkPointLocator *Locator;
};

// Description:
// Set a particular contour value at contour number i. The index i ranges 
// between 0<=i<NumberOfContours.
inline void vtkMarchingCubes::SetValue(int i, float value)
{this->ContourValues->SetValue(i,value);}

// Description:
// Get the ith contour value.
inline float vtkMarchingCubes::GetValue(int i)
{return this->ContourValues->GetValue(i);}

// Description:
// Get a pointer to an array of contour values. There will be
// GetNumberOfContours() values in the list.
inline float *vtkMarchingCubes::GetValues()
{return this->ContourValues->GetValues();}

// Description:
// Fill a supplied list with contour values. There will be
// GetNumberOfContours() values in the list. Make sure you allocate
// enough memory to hold the list.
inline void vtkMarchingCubes::GetValues(float *contourValues)
{this->ContourValues->GetValues(contourValues);}

// Description:
// Set the number of contours to place into the list. You only really
// need to use this method to reduce list size. The method SetValue()
// will automatically increase list size as needed.
inline void vtkMarchingCubes::SetNumberOfContours(int number)
{this->ContourValues->SetNumberOfContours(number);}

// Description:
// Get the number of contours in the list of contour values.
inline int vtkMarchingCubes::GetNumberOfContours()
{return this->ContourValues->GetNumberOfContours();}

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
inline void vtkMarchingCubes::GenerateValues(int numContours, float range[2])
{this->ContourValues->GenerateValues(numContours, range);}

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
inline void vtkMarchingCubes::GenerateValues(int numContours, float
                                             rangeStart, float rangeEnd)
{this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);}

#endif


