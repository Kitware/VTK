/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSurfaceReconstructionFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Tim Hutton (MINORI Project, Dental and Medical
             Informatics, Eastman Dental Institute, London, UK) who
             developed and contributed this class.

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
// .NAME vtkSurfaceReconstructionFilter - reconstructs a surface from unorganized points
// .SECTION Description
// 
// vtkSurfaceReconstructionFilter takes a list of points assumed to lie on
// the surface of a solid 3D object. A signed measure of the distance to the
// surface is computed and sampled on a regular grid. The grid can then be
// contoured at zero to extract the surface. The default values for
// neighborhood size and sample spacing should give reasonable results for
// most uses but can be set if desired. This procedure is based on the PhD
// work of Hugues Hoppe: http://www.research.microsoft.com/-hoppe

#ifndef __vtkSurfaceReconstructionFilter_h
#define __vtkSurfaceReconstructionFilter_h

#include "vtkDataSetToStructuredPointsFilter.h"

class VTK_EXPORT vtkSurfaceReconstructionFilter : public vtkDataSetToStructuredPointsFilter
{
public:
  vtkTypeMacro(vtkSurfaceReconstructionFilter,vtkDataSetToStructuredPointsFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct with NeighborhoodSize=20.
  static vtkSurfaceReconstructionFilter* New();

  // Description: 
  // Specify the number of neighbors each point has, used for estimating the
  // local surface orientation.  The default value of 20 should be OK for
  // most applications, higher values can be specified if the spread of
  // points is uneven. Values as low as 10 may yield adequate results for
  // some surfaces. Higher values cause the algorithm to take longer. Higher
  // values will cause errors on sharp boundaries.
  vtkGetMacro(NeighborhoodSize,int);
  vtkSetMacro(NeighborhoodSize,int);

  // Description: 
  // Specify the spacing of the 3D sampling grid. If not set, a
  // reasonable guess will be made.
  vtkGetMacro(SampleSpacing,float);
  vtkSetMacro(SampleSpacing,float);

protected:
  vtkSurfaceReconstructionFilter();
  ~vtkSurfaceReconstructionFilter() {};
  vtkSurfaceReconstructionFilter(const vtkSurfaceReconstructionFilter&);
  void operator=(const vtkSurfaceReconstructionFilter&);

  void Execute();

  int NeighborhoodSize;
  float SampleSpacing;
};

#endif

