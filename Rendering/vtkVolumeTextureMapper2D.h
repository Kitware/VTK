/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeTextureMapper2D.h
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
// .NAME vtkVolumeTextureMapper2D - Abstract class for a volume mapper

// .SECTION Description
// vtkVolumeTextureMapper2D renders a volume using 2D texture mapping.


// .SECTION see also
// vtkVolumeMapper

#ifndef __vtkVolumeTextureMapper2D_h
#define __vtkVolumeTextureMapper2D_h

#include "vtkVolumeTextureMapper.h"

class VTK_EXPORT vtkVolumeTextureMapper2D : public vtkVolumeTextureMapper
{
public:
  vtkTypeMacro(vtkVolumeTextureMapper2D,vtkVolumeTextureMapper);
  void PrintSelf( ostream& os, vtkIndent index );

  static vtkVolumeTextureMapper2D *New();
  
  // Description:
  // Target size in pixels of each size of the texture for downloading. Default is
  // 512x512 - so a 512x512 texture will be tiled with as many slices of the volume
  // as possible, then all the quads will be rendered. This can be set to optimize
  // for a particular architecture. This must be set with numbers that are a power
  // of two.
  vtkSetVector2Macro( TargetTextureSize, int );
  vtkGetVector2Macro( TargetTextureSize, int );
  
  // Description:
  // This is the maximum number of planes that will be created for texture mapping
  // the volume. If the volume has more voxels than this along the viewing direction,
  // then planes of the volume will be skipped to ensure that this maximum is not
  // violated. A skip factor is used, and is incremented until the maximum condition
  // is satisfied.
  vtkSetMacro( MaximumNumberOfPlanes, int );
  vtkGetMacro( MaximumNumberOfPlanes, int );

  // Description:
  // This is the maximum size of saved textures in bytes. If this size is large
  // enough to hold the RGBA textures for all three directions (XxYxZx3x4 is
  // the approximate value - it is actually a bit larger due to wasted space in
  // the textures) then the textures will be saved.
  vtkSetMacro( MaximumStorageSize, int );
  vtkGetMacro( MaximumStorageSize, int );
  
//BTX

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Render the volume
  virtual void Render(vtkRenderer *, vtkVolume *) {};

  virtual void RenderQuads( int vtkNotUsed(count),
                            float *vtkNotUsed(v), float *vtkNotUsed(t),
                            unsigned char *vtkNotUsed(texture),
                            int vtkNotUsed(size)[2],
                            int vtkNotUsed(reverseFlag)) {};

  // Description:
  // Made public only for access from the templated method. Not a vtkGetMacro
  // to avoid the PrintSelf defect.
  int GetInternalSkipFactor() {return this->InternalSkipFactor;};
  
  int *GetAxisTextureSize() {return &(this->AxisTextureSize[0][0]);};

  int GetSaveTextures() {return this->SaveTextures;};

  unsigned char *GetTexture() {return this->Texture;};
  
//ETX


protected:
  vtkVolumeTextureMapper2D();
  ~vtkVolumeTextureMapper2D();
  vtkVolumeTextureMapper2D(const vtkVolumeTextureMapper2D&);
  void operator=(const vtkVolumeTextureMapper2D&);

  void InitializeRender( vtkRenderer *ren, vtkVolume *vol )
    {this->InitializeRender( ren, vol, -1 );}
  
  void InitializeRender( vtkRenderer *ren, vtkVolume *vol, int majorDirection );

  void GenerateTexturesAndRenderQuads( vtkRenderer *ren, vtkVolume *vol );

  int  MajorDirection;
  int  TargetTextureSize[2];

  int  MaximumNumberOfPlanes;
  int  InternalSkipFactor;
  int  MaximumStorageSize;
  
  unsigned char  *Texture;
  int             TextureSize;
  int             SaveTextures;
  vtkTimeStamp    TextureMTime;
  
  int             AxisTextureSize[3][3];
  void            ComputeAxisTextureSize( int axis, int *size );
  
  void           RenderSavedTexture();
  
};


#endif


