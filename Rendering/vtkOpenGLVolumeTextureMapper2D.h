/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeTextureMapper2D.h
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
// .NAME vtkOpenGLVolumeTextureMapper2D - Abstract class for a volume mapper

// .SECTION Description
// vtkOpenGLVolumeTextureMapper2D renders a volume using 2D texture mapping.


// .SECTION see also
// vtkVolumeMapper

#ifndef __vtkOpenGLVolumeTextureMapper2D_h
#define __vtkOpenGLVolumeTextureMapper2D_h

#include "vtkVolumeTextureMapper2D.h"

class VTK_RENDERING_EXPORT vtkOpenGLVolumeTextureMapper2D : public vtkVolumeTextureMapper2D
{
public:
  vtkTypeMacro(vtkOpenGLVolumeTextureMapper2D,vtkVolumeTextureMapper2D);
  void PrintSelf( ostream& os, vtkIndent index );

  static vtkOpenGLVolumeTextureMapper2D *New();
  
//BTX

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Render the volume
  virtual void Render(vtkRenderer *ren, vtkVolume *vol);

  void RenderQuads( int count, float *v, float *t,
                    unsigned char *texture, int size[2], int reverseFlag);

//ETX

protected:
  vtkOpenGLVolumeTextureMapper2D();
  ~vtkOpenGLVolumeTextureMapper2D();

private:
  vtkOpenGLVolumeTextureMapper2D(const vtkOpenGLVolumeTextureMapper2D&);  // Not implemented.
  void operator=(const vtkOpenGLVolumeTextureMapper2D&);  // Not implemented.
};


#endif


