/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThinPlateSplineMeshWarp.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Tim Hutton (MINORI Project, Dental and Medical
             Informatics, Eastman Dental Institute, London, UK) who
             developed and contributed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkThinPlateSplineMeshWarp - warps polygonal meshes using landmarks
// .SECTION Description
// vtkThinPlateSplineMeshWarp warps a polygonal mesh into a different shape
// using two sets of landmarks (vtkPoints). Any point on the mesh close to a
// source landmark will be moved to a place close to the corresponding target
// landmark. The points in between are interpolated smoothly using
// Bookstein's Thin Plate Spline algorithm.
//
// The filter takes three inputs: the polygonal mesh to be warped (use
// SetInput), the source landmarks (SetSourceLandmarks) and the target
// Landmarks (SetTargetLandmarks).  There is one parameter (Sigma) that
// controls the 'stiffness' of the spline (default is 1.0).
//
// The topology of the mesh is not altered, only the geometry (the location
// of the points).

#ifndef __vtkThinPlateSplineMeshWarp_h
#define __vtkThinPlateSplineMeshWarp_h

#include "vtkPolyDataToPolyDataFilter.h"

class VTK_EXPORT vtkThinPlateSplineMeshWarp : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeMacro(vtkThinPlateSplineMeshWarp,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with Sigma=1.0
  static vtkThinPlateSplineMeshWarp* New();

  // Description: 
  // Specify the 'stiffness' of the spline. The default of 1.0
  // should usually be fine.
  vtkGetMacro(Sigma,float);
  vtkSetMacro(Sigma,float);

  // Description:
  // Turn on/off the generation of displacement vectors.
  vtkSetMacro(GenerateDisplacementVectors,int);
  vtkGetMacro(GenerateDisplacementVectors,int);
  vtkBooleanMacro(GenerateDisplacementVectors,int);

  // Description:
  // Set the source landmarks for the warp.
  vtkSetObjectMacro(SourceLandmarks,vtkPoints);
  vtkGetObjectMacro(SourceLandmarks,vtkPoints);

  // Description:
  // Set the target landmarks for the warp
  vtkSetObjectMacro(TargetLandmarks,vtkPoints);
  vtkGetObjectMacro(TargetLandmarks,vtkPoints);

protected:
  vtkThinPlateSplineMeshWarp();
  ~vtkThinPlateSplineMeshWarp();
  vtkThinPlateSplineMeshWarp(const vtkThinPlateSplineMeshWarp&) {};
  void operator=(const vtkThinPlateSplineMeshWarp&) {};

  void Execute();

  float Sigma;
  int GenerateDisplacementVectors;
  vtkPoints *SourceLandmarks;
  vtkPoints *TargetLandmarks;
};

#endif
