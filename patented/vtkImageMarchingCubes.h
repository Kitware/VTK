/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMarchingCubes.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
PARTICULAR PURPOSE, AND -INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkImageMarchingCubes - generate isosurface(s) from volume/images
// .SECTION Description
// vtkImageMarchingCubes is a filter that takes as input images (e.g., 3D
// image region) and generates on output one or more isosurfaces.
// One or more contour values must be specified to generate the isosurfaces.
// Alternatively, you can specify a min/max scalar range and the number of
// contours to generate a series of evenly spaced contour values.
// This filter can stream, so that the entire volume need not be loaded at
// once.  Streaming is controlled using the instance variable
// InputMemroyLimit, which has units KBytes.

// .SECTION Caveats
// This filter is specialized to volumes. If you are interested in 
// contouring other types of data, use the general vtkContourFilter. If you
// want to contour an image (i.e., a volume slice), use vtkMarchingSquares.
// .SECTION See Also
// vtkContourFilter vtkSliceCubes vtkMarchingSquares vtkDividingCubes


#ifndef __vtkImageMarchingCubes_h
#define __vtkImageMarchingCubes_h

#include "vtkImageCache.h"
#include "vtkPolyDataSource.h"
#include "vtkStructuredPointsToImage.h"
#include "vtkContourValues.h"

class VTK_EXPORT vtkImageMarchingCubes : public vtkPolyDataSource
{
public:
  vtkImageMarchingCubes();
  static vtkImageMarchingCubes *New() {return new vtkImageMarchingCubes;};
  ~vtkImageMarchingCubes();
  const char *GetClassName() {return "vtkImageMarchingCubes";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the source for the scalar data to contour.
  vtkSetObjectMacro(Input, vtkImageCache);
  vtkGetObjectMacro(Input, vtkImageCache);
  void SetInput(vtkStructuredPoints *spts)
    {this->SetInput(spts->GetStructuredPointsToImage()->GetOutput());}
  
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

  // Description:
  // Set/Get the computation of scalars.
  vtkSetMacro(ComputeScalars, int);
  vtkGetMacro(ComputeScalars, int);
  vtkBooleanMacro(ComputeScalars, int);
  
  // Description:
  // Set/Get the computation of normals. Normal computation is failrly expensive
  // in both time and storage. If the output data will be processed by filters
  // that modify topology or geometry, it may be wise to turn Normals and Gradients off.
  vtkSetMacro(ComputeNormals, int);
  vtkGetMacro(ComputeNormals, int);
  vtkBooleanMacro(ComputeNormals, int);
  
  // Description:
  // Set/Get the computation of gradients. Gradient computation is fairly expensive
  // in both time and storage. Note that if ComputeNormals is on, gradients will
  // have to be calculated, but will not be stored in the output dataset.
  // If the output data will be processed by filters that modify topology or
  // geometry, it may be wise to turn Normals and Gradients off.
  vtkSetMacro(ComputeGradients, int);
  vtkGetMacro(ComputeGradients, int);
  vtkBooleanMacro(ComputeGradients, int);
  
  // Description:
  // The InputMemoryLimit determines the chunk size (the number of slices
  // requested at each iteration).  The units of this limit is KiloBytes.
  // For now, only the Z axis is split.
  vtkSetMacro(InputMemoryLimit, int);
  vtkGetMacro(InputMemoryLimit, int);

  // Should be protected, but the templated functions need these
  int ComputeScalars;
  int ComputeNormals;
  int ComputeGradients;
  int NeedGradients;

  vtkCellArray *Triangles;
  vtkFloatScalars *Scalars;
  vtkFloatPoints *Points;
  vtkFloatNormals *Normals;
  vtkFloatVectors *Gradients;
  
  int GetLocatorPoint(int cellX, int cellY, int edge);
  void AddLocatorPoint(int cellX, int cellY, int edge, int ptId);
  void IncrementLocatorZ();
  
protected:
  vtkImageCache *Input;
  int InputMemoryLimit;
  int NumberOfSlicesPerChunk;

  vtkContourValues *ContourValues;
   
  int *LocatorPointIds;
  int LocatorDimX;
  int LocatorDimY;
  int LocatorMinX;
  int LocatorMinY;
  
  void Execute();

  void March(vtkImageRegion *inRegion, int chunkMin, int chunkMax,
             int numContours, float *values);
  void InitializeLocator(int min0, int max0, int min1, int max1);
  void DeleteLocator();
  int *GetLocatorPointer(int cellX, int cellY, int edge);
};

// Description:
// Set a particular contour value at contour number i. The index i ranges 
// between 0<=i<NumberOfContours.
inline void vtkImageMarchingCubes::SetValue(int i, float value)
{this->ContourValues->SetValue(i,value);}

// Description:
// Get the ith contour value.
inline float vtkImageMarchingCubes::GetValue(int i)
{return this->ContourValues->GetValue(i);};

// Description:
// Get a pointer to an array of contour values. There will be
// GetNumberOfContours() values in the list.
inline float *vtkImageMarchingCubes::GetValues()
{return this->ContourValues->GetValues();};

// Description:
// Fill a supplied list with contour values. There will be
// GetNumberOfContours() values in the list. Make sure you allocate
// enough memory to hold the list.
inline void vtkImageMarchingCubes::GetValues(float *contourValues)
{this->ContourValues->GetValues(contourValues);};

// Description:
// Set the number of contours to place into the list. You only really
// need to use this method to reduce list size. The method SetValue()
// will automatically increase list size as needed.
inline void vtkImageMarchingCubes::SetNumberOfContours(int number)
{this->ContourValues->SetNumberOfContours(number);};

// Description:
// Get the number of contours in the list of contour values.
inline int vtkImageMarchingCubes::GetNumberOfContours()
{return this->ContourValues->GetNumberOfContours();};

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
inline void vtkImageMarchingCubes::GenerateValues(int numContours, float range[2])
{this->ContourValues->GenerateValues(numContours, range);};

// Description:
// Generate numContours equally spaced contour values between specified
// range. Contour values will include min/max range values.
inline void vtkImageMarchingCubes::GenerateValues(int numContours, float
                                                 rangeStart, float rangeEnd)
{this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);};

#endif



