/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProbeFilter.h
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
// .NAME vtkProbeFilter - sample data values at specified point locations
// .SECTION Description
// vtkProbeFilter is a filter that computes point attributes (e.g., scalars,
// vectors, etc.) at specified point positions. The filter has two inputs:
// the Input and Source. The Input geometric structure is passed through the
// filter. The point attributes are computed at the Input point positions
// by interpolating into the source data. For example, we can compute data
// values on a plane (plane specified as Input) from a volume (Source).

#ifndef __vtkProbeFilter_h
#define __vtkProbeFilter_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_EXPORT vtkProbeFilter : public vtkDataSetToDataSetFilter
{
public:
  static vtkProbeFilter *New();
  vtkTypeMacro(vtkProbeFilter,vtkDataSetToDataSetFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the point locations used to probe input. Any geometry
  // can be used.
  void SetSource(vtkDataSet *source);
  vtkDataSet *GetSource();

  // Description:
  // This flag is used only when a piece is requested to update.  By default
  // the flag is off.  Because no spatial correspondence between input pieces
  // and source pieces is known, all of the source has to be requested no
  // matter what piece of the output is requested.  When there is a spatial 
  // correspondence, the user/application can set this flag.  This hint allows
  // the breakup of the probe operation to be much more efficient.  When piece
  // m of n is requested for update by the user, then only n of m needs to
  // be requested of the source. 
  vtkSetMacro(SpatialMatch, int);
  vtkGetMacro(SpatialMatch, int);
  vtkBooleanMacro(SpatialMatch, int);

protected:
  vtkProbeFilter();
  ~vtkProbeFilter();
  vtkProbeFilter(const vtkProbeFilter&);
  void operator=(const vtkProbeFilter&);

  int SpatialMatch;

  void Execute();
  void ExecuteInformation();
  void ComputeInputUpdateExtents(vtkDataObject *output);

};

#endif


