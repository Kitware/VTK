/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourGrid.h
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
// .NAME vtkContourGrid - generate isosurfaces/isolines from scalar values (specialized for unstructured grids)
// .SECTION Description
// vtkContourGrid is a filter that takes as input datasets of type
// vtkUnstructuredGrid and generates on output isosurfaces and/or
// isolines. The exact form of the output depends upon the dimensionality of
// the input data.  Data consisting of 3D cells will generate isosurfaces,
// data consisting of 2D cells will generate isolines, and data with 1D or 0D
// cells will generate isopoints. Combinations of output type are possible if
// the input dimension is mixed.
//
// To use this filter you must specify one or more contour values.
// You can either use the method SetValue() to specify each contour
// value, or use GenerateValues() to generate a series of evenly
// spaced contours. It is also possible to accelerate the operation of
// this filter (at the cost of extra memory) by using a
// vtkScalarTree. A scalar tree is used to quickly locate cells that
// contain a contour surface. This is especially effective if multiple
// contours are being extracted. If you want to use a scalar tree,
// invoke the method UseScalarTreeOn().
//
// If the input data is structured, consider using a filter that is
// optimized for structured data. These can be found in the patented
// classes of vtk.

// .SECTION Caveats
// For unstructured data or structured grids, normals and gradients
// are not computed. Use vtkPolyDataNormals to compute the surface
// normals of the resulting isosurface.

// .SECTION See Also
// vtkMarchingContourFilter vtkKitwareContourFilter
// vtkMarchingCubes vtkSliceCubes vtkDividingCubes vtkMarchingSquares
// vtkImageMarchingCubes

#ifndef __vtkContourGrid_h
#define __vtkContourGrid_h

#include "vtkUnstructuredGridToPolyDataFilter.h"
#include "vtkContourValues.h"

class vtkScalarTree;
class vtkEdgeTable;

class VTK_GRAPHICS_EXPORT vtkContourGrid : public vtkUnstructuredGridToPolyDataFilter
{
public:
  vtkTypeMacro(vtkContourGrid,vtkUnstructuredGridToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with initial range (0,1) and single contour value
  // of 0.0.
  static vtkContourGrid *New();

  // Description:
  // Methods to set / get contour values.
  void SetValue(int i, float value);
  float GetValue(int i);
  float *GetValues();
  void GetValues(float *contourValues);
  void SetNumberOfContours(int number);
  int GetNumberOfContours();
  void GenerateValues(int numContours, float range[2]);
  void GenerateValues(int numContours, float rangeStart, float rangeEnd);

  // Description:
  // Modified GetMTime Because we delegate to vtkContourValues
  unsigned long GetMTime();

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

  // Description:
  // Enable the use of a scalar tree to accelerate contour extraction.
  vtkSetMacro(UseScalarTree,int);
  vtkGetMacro(UseScalarTree,int);
  vtkBooleanMacro(UseScalarTree,int);

  // Description:
  // Set / get a spatial locator for merging points. By default, 
  // an instance of vtkMergePoints is used.
  void SetLocator(vtkPointLocator *locator);
  vtkGetObjectMacro(Locator,vtkPointLocator);

  // Description:
  // Create default locator. Used to create one when none is
  // specified. The locator is used to merge coincident points.
  void CreateDefaultLocator();

protected:
  vtkContourGrid();
  ~vtkContourGrid();

  void Execute();

  vtkContourValues *ContourValues;
  int ComputeNormals;
  int ComputeGradients;
  int ComputeScalars;
  vtkPointLocator *Locator;
  int UseScalarTree;
  vtkScalarTree *ScalarTree;
  vtkEdgeTable *EdgeTable;
  
private:
  vtkContourGrid(const vtkContourGrid&);  // Not implemented.
  void operator=(const vtkContourGrid&);  // Not implemented.
};

// Description:
// Set a particular contour value at contour number i. The index i ranges 
// between 0<=i<NumberOfContours.
inline void vtkContourGrid::SetValue(int i, float value)
{this->ContourValues->SetValue(i,value);}

// Description:
// Get the ith contour value.
inline float vtkContourGrid::GetValue(int i)
{return this->ContourValues->GetValue(i);}

// Description:
// Get a pointer to an array of contour values. There will be
// GetNumberOfContours() values in the list.
inline float *vtkContourGrid::GetValues()
{return this->ContourValues->GetValues();}

// Description:
// Fill a supplied list with contour values. There will be
// GetNumberOfContours() values in the list. Make sure you allocate
// enough memory to hold the list.
inline void vtkContourGrid::GetValues(float *contourValues)
{this->ContourValues->GetValues(contourValues);}

// Description:
// Set the number of contours to place into the list. You only really
// need to use this method to reduce list size. The method SetValue()
// will automatically increase list size as needed.
inline void vtkContourGrid::SetNumberOfContours(int number)
{this->ContourValues->SetNumberOfContours(number);}

// Description:
// Get the number of contours in the list of contour values.
inline int vtkContourGrid::GetNumberOfContours()
{return this->ContourValues->GetNumberOfContours();}

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
inline void vtkContourGrid::GenerateValues(int numContours, float range[2])
{this->ContourValues->GenerateValues(numContours, range);}

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
inline void vtkContourGrid::GenerateValues(int numContours, float
                                             rangeStart, float rangeEnd)
{this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);}


#endif


