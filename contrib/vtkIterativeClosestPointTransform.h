/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIterativeClosestPointTransform.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Sebastien Barre who developed this class. Thanks to
             Tim Hutton too for the idea.

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

// .NAME vtkIterativeClosestPointTransform - Implementation of the ICP algorithm.
// .SECTION Description
// Match two surfaces using the iterative closest point (ICP) algorithm.
// The core of the algorithm is to match each vertex in one surface with 
// the closest surface point on the other, then apply the rigid transformation
// that moves one surface to best match the other. This has to be iterated to
// get proper convergence of the surfaces.
// Use the transform in vtkTransformPolyDataFilter for example, to apply
// the resulting ICP transform to your data.


#ifndef __vtkIterativeClosestPointTransform_h
#define __vtkIterativeClosestPointTransform_h

#include "vtkLinearTransform.h"

class vtkCellLocator;
class vtkLandmarkTransform;
class vtkDataSet;

class VTK_EXPORT vtkIterativeClosestPointTransform : public vtkLinearTransform
{
public:
  static vtkIterativeClosestPointTransform *New();
  vtkTypeMacro(vtkIterativeClosestPointTransform,vtkLinearTransform);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the source and target data sets.
  void SetSource(vtkDataSet *source);
  void SetTarget(vtkDataSet *target);
  vtkGetObjectMacro(Source, vtkDataSet);
  vtkGetObjectMacro(Target, vtkDataSet);

  // Description:
  // Set/Get a spatial locator for speeding up the search process. 
  // An instance of vtkCellLocator is used by default.
  void SetLocator(vtkCellLocator *locator);
  vtkGetObjectMacro(Locator,vtkCellLocator);

  // Description: 
  // Set/Get the  maximum number of iterations
  vtkSetMacro(MaximumNumberOfIterations, int);
  vtkGetMacro(MaximumNumberOfIterations, int);

  // Description: 
  // Get the number of iterations since the last update
  vtkGetMacro(NumberOfIterations, int);

  // Description: 
  // Force the algorithm to check the mean distance between two iteration.
  vtkSetMacro(CheckMeanDistance, int);
  vtkGetMacro(CheckMeanDistance, int);
  vtkBooleanMacro(CheckMeanDistance, int);

  // Description: 
  // Set/Get the maximum mean distance between two iteration. If the mean
  // distance is lower than this, the convergence stops.
  vtkSetMacro(MaximumMeanDistance, float);
  vtkGetMacro(MaximumMeanDistance, float);
  
  // Description: 
  // Get the mean distance between the last two iterations.
  vtkGetMacro(MeanDistance, float);
  
  // Description: 
  // Set/Get the maximum number of landmarks sampled in your dataset.
  // If your dataset is dense, then you will typically not need all the 
  // points to compute the ICP transform. 
  vtkSetMacro(MaximumNumberOfLandmarks, int);
  vtkGetMacro(MaximumNumberOfLandmarks, int);

  // Description: 
  // Starts the process by translating source centroid to target centroid.
  vtkSetMacro(StartByMatchingCentroids, int);
  vtkGetMacro(StartByMatchingCentroids, int);
  vtkBooleanMacro(StartByMatchingCentroids, int);

  // Description: 
  // Get the landmark transform.
  vtkGetObjectMacro(LandmarkTransform,vtkLandmarkTransform);
  
  // Description:
  // Invert the transformation.  This is done by switching the
  // source and target.
  void Inverse();

  // Description:
  // Make another transform of the same type.
  vtkAbstractTransform *MakeTransform();

protected:

  // Description:
  // Release source and target
  void ReleaseSource(void);
  void ReleaseTarget(void);

  // Description:
  // Release locator
  void ReleaseLocator(void);

  // Description:
  // Create default locator. Used to create one when none is specified.
  void CreateDefaultLocator(void);

  // Description:
  // Get the MTime of this object also considering the locator.
  unsigned long int GetMTime();

  vtkIterativeClosestPointTransform();
  ~vtkIterativeClosestPointTransform();
  vtkIterativeClosestPointTransform(const vtkIterativeClosestPointTransform&) {};
  void operator=(const vtkIterativeClosestPointTransform&) {};

  void InternalUpdate();

  // Description:
  // This method does no type checking, use DeepCopy instead.
  void InternalDeepCopy(vtkAbstractTransform *transform);

  vtkDataSet* Source;
  vtkDataSet* Target;
  vtkCellLocator *Locator;
  int MaximumNumberOfIterations;
  int CheckMeanDistance;
  float MaximumMeanDistance;
  int MaximumNumberOfLandmarks;
  int StartByMatchingCentroids;

  int NumberOfIterations;
  float MeanDistance;
  vtkLandmarkTransform *LandmarkTransform;
};

#endif
