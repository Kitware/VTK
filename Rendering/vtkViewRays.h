/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkViewRays.h
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

// .NAME vtkViewRays - obsolete class
// .SECTION Description

#ifndef __vtkViewRays_h
#define __vtkViewRays_h
#include "vtkObject.h"
#include "vtkMatrix4x4.h"
class vtkRenderer;

class VTK_RENDERING_EXPORT vtkViewRays :public vtkObject
{
public:
  static vtkViewRays *New() {return new vtkViewRays;};
  vtkTypeMacro(vtkViewRays,vtkObject);

  void SetRenderer(vtkRenderer *ren){VTK_LEGACY_METHOD(SetRenderer,"4.0");};
  vtkRenderer *GetRenderer()
    {VTK_LEGACY_METHOD(GetRenderer,"4.0"); return NULL;};
  
  void SetSize( int size[2] ){VTK_LEGACY_METHOD(SetSize,"4.0");};
  void SetSize(int x, int y){VTK_LEGACY_METHOD(SetSize,"4.0");};
  int *GetSize(){VTK_LEGACY_METHOD(GetSize,"4.0"); return NULL;};
  void GetSize(int size[2]){VTK_LEGACY_METHOD(GetSize,"4.0");};

  float *GetPerspectiveViewRays(void)
    {VTK_LEGACY_METHOD(GetPerspectiveViewRays,"4.0"); return NULL;};
      

  float *GetParallelStartPosition(void)
    {VTK_LEGACY_METHOD(GetParallelStartPosition,"4.0"); return NULL;};

  float *GetParallelIncrements(void)
    {VTK_LEGACY_METHOD(GetParallelIncrements,"4.0"); return NULL;};

protected:
  vtkViewRays(void) {};
  ~vtkViewRays(void) {};
  vtkViewRays(const vtkViewRays&);
  void operator=(const vtkViewRays&);
  };
#endif



