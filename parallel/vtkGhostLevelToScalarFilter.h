/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGhostLevelToScalarFilter.h
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
// .NAME vtkGhostLevelToScalarFilter - Move ghost levels to scalars.
// .SECTION Description
// vtkGhostLevelToScalarFilter was created to debug the ghost cells 
// feature of some sources.  It moves the ghost level values to
// scalars so that they can be visualized.

#ifndef __vtkGhostLevelToScalarFilter_h
#define __vtkGhostLevelToScalarFilter_h

#include "vtkDataSetToDataSetFilter.h"

class VTK_EXPORT vtkGhostLevelToScalarFilter : public vtkDataSetToDataSetFilter 
{
public:
  vtkTypeMacro(vtkGhostLevelToScalarFilter,vtkDataSetToDataSetFilter);
  static vtkGhostLevelToScalarFilter *New();

protected:
  vtkGhostLevelToScalarFilter() {};
  ~vtkGhostLevelToScalarFilter() {};
  vtkGhostLevelToScalarFilter(const vtkGhostLevelToScalarFilter&) {};
  void operator=(const vtkGhostLevelToScalarFilter&) {};

  void Execute();
  void CopyLevelsToScalars(vtkGhostLevels *levels, vtkScalars *scalars);
};

#endif


