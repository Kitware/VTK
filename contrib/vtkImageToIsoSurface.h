/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToIsoSurface.h
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
// .NAME vtkImageToIsoSurface - Streaming marching cubes.
// .SECTION Description
// vtkImageToIsoSurface  is an implementation of marching cubes
// that can take its input in chunks.


#ifndef __vtkImageToIsoSurface_h
#define __vtkImageToIsoSurface_h

#include "vtkImageSource.h"
#include "vtkPolySource.h"

#define VTK_MAX_CONTOURS 256

class VTK_EXPORT vtkImageToIsoSurface : public vtkPolySource
{
public:
  vtkImageToIsoSurface();
  static vtkImageToIsoSurface *New() {return new vtkImageToIsoSurface;};
  char *GetClassName() {return "vtkImageToIsoSurface";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set/Get the source of the vector field.
  vtkSetObjectMacro(Input, vtkImageSource);
  vtkGetObjectMacro(Input, vtkImageSource);

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
  
  void SetValue(int i, float value);
  float GetValue(int i) {return this->Values[i];};

  // Description:
  // Return array of contour values (size of numContours).
  vtkGetVectorMacro(Values,float,VTK_MAX_CONTOURS);

  // Description:
  // Set/get the number of contour values. The number of values set (using SetValue)
  // should match the NumberOfContours ivar value.
  vtkSetMacro(NumberOfContours,int);
  vtkGetMacro(NumberOfContours,int);

  void GenerateValues(int numContours, float range[2]);
  void GenerateValues(int numContours, float range1, float range2);
  
  // Description:
  // The InputMemoryLimit determines the chunk size (the number of slices
  // requested at each iteration).  The units of this limit is KiloBytes.
  // For now, only the Z axis is split.
  vtkSetMacro(InputMemoryLimit, int);
  vtkGetMacro(InputMemoryLimit, int);
  
protected:
  vtkImageSource *Input;
  int InputMemoryLimit;
  int NumberOfSlicesPerChunk;

  int ComputeScalars;
  int ComputeNormals;
  int ComputeGradients;
  int NeedGradients;
  
  float Values[VTK_MAX_CONTOURS];
  int NumberOfContours;
  float Range[2];
  
  vtkCellArray *Triangles;
  vtkFloatScalars *Scalars;
  vtkFloatPoints *Points;
  vtkFloatNormals *Normals;
  vtkFloatVectors *Gradients;
  
  int *LocatorPointIds;
  int LocatorDimX;
  int LocatorDimY;
  int LocatorMinX;
  int LocatorMinY;
  
  void Execute();

  void March(vtkImageRegion *inRegion, int chunkMin, int chunkMax);
  void HandleCube(int cellX, int cellY, int cellZ, vtkImageRegion *inRegion,
		  float *ptr);
  int MakeNewPoint(int idx0, int idx1, int idx2,
		   int inc0, int inc1, int inc2,
		   float *ptr, int edge, int *imageExtent,
		   float *aspectRatio, float *origin,
		   float value);
  void ComputePointGradient(float g[3], float *ptr,
			    int inc0, int inc1, int inc2, 
			    int b0, int b1, int b2);
  void InitializeLocator(int min0, int max0, int min1, int max1);
  void DeleteLocator();
  void IncrementLocatorZ();
  void AddLocatorPoint(int cellX, int cellY, int edge, int ptId);
  int GetLocatorPoint(int cellX, int cellY, int edge);
  int *GetLocatorPointer(int cellX, int cellY, int edge);
};

#endif



