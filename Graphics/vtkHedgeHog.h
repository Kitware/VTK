/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHedgeHog.h
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
// .NAME vtkHedgeHog - create oriented lines from vector data
// .SECTION Description
// vtkHedgeHog creates oriented lines from the input data set. Line
// length is controlled by vector (or normal) magnitude times scale
// factor. If VectorMode is UseNormal, normals determine the orientation
// of the lines. Lines are colored by scalar data, if available.

#ifndef __vtkHedgeHog_h
#define __vtkHedgeHog_h

#include "vtkDataSetToPolyDataFilter.h"

#define VTK_USE_VECTOR 0
#define VTK_USE_NORMAL 1

class VTK_GRAPHICS_EXPORT vtkHedgeHog : public vtkDataSetToPolyDataFilter
{
public:
  static vtkHedgeHog *New();
  vtkTypeMacro(vtkHedgeHog,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set scale factor to control size of oriented lines.
  vtkSetMacro(ScaleFactor,float);
  vtkGetMacro(ScaleFactor,float);

  // Description:
  // Specify whether to use vector or normal to perform vector operations.
  vtkSetMacro(VectorMode,int);
  vtkGetMacro(VectorMode,int);
  void SetVectorModeToUseVector() {this->SetVectorMode(VTK_USE_VECTOR);};
  void SetVectorModeToUseNormal() {this->SetVectorMode(VTK_USE_NORMAL);};
  const char *GetVectorModeAsString();

protected:
  vtkHedgeHog();
  ~vtkHedgeHog() {};

  void Execute();
  float ScaleFactor;
  int VectorMode; // Orient/scale via normal or via vector data

private:
  vtkHedgeHog(const vtkHedgeHog&);  // Not implemented.
  void operator=(const vtkHedgeHog&);  // Not implemented.
};

// Description:
// Return the vector mode as a character string.
inline const char *vtkHedgeHog::GetVectorModeAsString(void)
{
  if ( this->VectorMode == VTK_USE_VECTOR) 
    {
    return "UseVector";
    }
  else if ( this->VectorMode == VTK_USE_NORMAL) 
    {
    return "UseNormal";
    }
  else 
    {
    return "Unknown";
    }
}
#endif


