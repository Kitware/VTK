/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToMaximumSurface.h
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
// .NAME vtkImageToMaximumSurface - Creates polygon surface from vector field.
// .SECTION Description
// vtkImageToMaximumSurface takes a vector field from a surface detection
// filter (i.e. Gradient) and creates a polygonal surface at the maximum
// surfaces of the vector field.  The surface will be orthogonal to
// the intersecting vectors.  The vectors must be above "MagnitudeThreshold"
// In order to be included in the surface.



#ifndef __vtkImageToMaximumSurface_h
#define __vtkImageToMaximumSurface_h

#include "vtkImageSource.h"
#include "vtkPolySource.h"

class vtkImageToMaximumSurface : public vtkPolySource
{
public:
  vtkImageToMaximumSurface();
  char *GetClassName() {return "vtkImageToMaximumSurface";};
  
  // Description:
  // Set/Get the source of the vector field.
  vtkSetObjectMacro(Input, vtkImageSource);
  vtkGetObjectMacro(Input, vtkImageSource);

  // Description:
  // Determine whether to compute scalars or not.
  vtkSetMacro(ComputeScalars, int);
  vtkGetMacro(ComputeScalars, int);
  vtkBooleanMacro(ComputeScalars, int);
  
  // Description:
  // Determine whether to compute normals or not.
  vtkSetMacro(ComputeNormals, int);
  vtkGetMacro(ComputeNormals, int);
  vtkBooleanMacro(ComputeNormals, int);
  
  // Description:
  // Set the lower magnitude threshold for generating a surface.
  // If this is zero, this filter will produce artifacts.
  vtkSetMacro(Threshold, float);
  vtkGetMacro(Threshold, float);

  
protected:
  vtkImageSource *Input;
  float Threshold;

  int ComputeScalars;
  int ComputeNormals;
  
  vtkCellArray *Triangles;
  vtkFloatScalars *Scalars;
  vtkFloatPoints *Points;
  vtkFloatNormals *Normals;
  
  int *LocatorPointIds;
  int LocatorDimX;
  int LocatorDimY;
  int LocatorMinX;
  int LocatorMinY;

  
  void Execute();

  void ComputeMagnitudes(vtkImageRegion *vector, vtkImageRegion *magnitude);
  void ComputeDerivatives(vtkImageRegion *vector, vtkImageRegion *magnitude,
			  vtkImageRegion *derivative);
  void March(vtkImageRegion *derivatives, vtkImageRegion *magnitudes,
	     vtkImageRegion *vectors, float *origin, float *ratio);
  void HandleCube(int cellX, int cellY, int cellZ, float *origin, float *ratio,
		  float *derivatives, float *magnitude, float **vectors, 
		  int vInc3);
  int MakeNewPoint(int cellX, int cellY, int cellZ, 
		   float *origin, float *ratio,
		   float *derivatives, float *magnitudes, float **vectors,
		   int vInc3, int edge);

  void InitializeLocator(int min0, int max0, int min1, int max1);
  void DeleteLocator();
  void IncrementLocatorZ();
  void AddLocatorPoint(int cellX, int cellY, int edge, int ptId);
  int GetLocatorPoint(int cellX, int cellY, int edge);
  int *GetLocatorPointer(int cellX, int cellY, int edge);
};

#endif



