/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVectorTopology.h
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
// .NAME vtkVectorTopology - mark points where the vector field vanishes (singularities exist).
// .SECTION Description
// vtkVectorTopology is a filter that marks points where the vector field 
// vanishes. At these points various important flow features are found, 
// including regions of circulation, separation, etc. The region around these
// areas are good places to start streamlines. (The vector field vanishes in 
// cells where the x-y-z vector components each pass through zero.)
//
// The output of this filter is a set of vertices. These vertices mark the 
// vector field singularities. You can use an object like vtkGlyph3D to place
// markers at these points, or use the vertices to initiate streamlines.
//
// The Distance instance variable controls the accuracy of placement of the
// vertices. Smaller values result in greater execution times.
//
// The input to this filter is any dataset type. The position of the 
// vertices is found by sampling the cell in parametric space. Sampling is
// repeated until the Distance criterion is satisfied.
// .SECTION See Also
// vtkGlyph3D vtkStreamLine

#ifndef __vtkVectorTopology_h
#define __vtkVectorTopology_h

#include "vtkDataSetToPolyDataFilter.h"

class VTK_EXPORT vtkVectorTopology : public vtkDataSetToPolyDataFilter
{
public:
  // Description:
  // Construct object with distance 0.1.
  static vtkVectorTopology *New();

  vtkTypeMacro(vtkVectorTopology,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify distance from singularity to generate point.
  vtkSetClampMacro(Distance,float,1.0e-06,VTK_LARGE_FLOAT);
  vtkGetMacro(Distance,float);

protected:
  vtkVectorTopology();
  ~vtkVectorTopology() {};
  vtkVectorTopology(const vtkVectorTopology&);
  void operator=(const vtkVectorTopology&);

  void Execute();
  float Distance;
};

#endif


