/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeTextureMapper.h
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
// .NAME vtkVolumeTextureMapper - Abstract class for a volume mapper

// .SECTION Description
// vtkVolumeTextureMapper is the abstract definition of a volume mapper
// that uses a texture mapping approach.

// .SECTION see also
// vtkVolumeMapper

#ifndef __vtkVolumeTextureMapper_h
#define __vtkVolumeTextureMapper_h

#include "vtkVolumeMapper.h"
#include "vtkEncodedGradientShader.h"
#include "vtkEncodedGradientEstimator.h"

class vtkVolume;
class vtkRenderer;
class vtkRenderWindow;

class VTK_EXPORT vtkVolumeTextureMapper : public vtkVolumeMapper
{
public:
  vtkTypeMacro(vtkVolumeTextureMapper,vtkVolumeMapper);
  void PrintSelf( ostream& os, vtkIndent index );

  // Description:
  // Update the volume rendering pipeline by updating the scalar input
  virtual void Update();

  // Description:
  // Set / Get the gradient estimator used to estimate normals
  void SetGradientEstimator( vtkEncodedGradientEstimator *gradest );
  vtkGetObjectMacro( GradientEstimator, vtkEncodedGradientEstimator );

  // Description:
  // Get the gradient shader.
  vtkGetObjectMacro( GradientShader, vtkEncodedGradientShader );

//BTX

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  virtual int GetMapperType() {return VTK_FRAMEBUFFER_VOLUME_MAPPER;};

  // Description:
  // Allow access to the arrays / variables from the templated functions in the
  // subclasses.
  float *GetGradientOpacityArray(){return this->GradientOpacityArray;};
  unsigned char *GetRGBAArray(){return this->RGBAArray;};
  float *GetRedDiffuseShadingTable(){return this->RedDiffuseShadingTable;};
  float *GetGreenDiffuseShadingTable(){return this->GreenDiffuseShadingTable;};
  float *GetBlueDiffuseShadingTable(){return this->BlueDiffuseShadingTable;};
  float *GetRedSpecularShadingTable(){return this->RedSpecularShadingTable;};
  float *GetGreenSpecularShadingTable(){return this->GreenSpecularShadingTable;};
  float *GetBlueSpecularShadingTable(){return this->BlueSpecularShadingTable;};
  unsigned short *GetEncodedNormals(){return this->EncodedNormals;};
  unsigned char *GetGradientMagnitudes(){return this->GradientMagnitudes;};
  vtkGetMacro( Shade, int );
  vtkGetObjectMacro( RenderWindow, vtkRenderWindow );
  vtkGetVectorMacro( DataOrigin, float, 3 );
  vtkGetVectorMacro( DataSpacing, float, 3 );

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Render the volume
  virtual void Render(vtkRenderer *ren, vtkVolume *vol)=0;

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // Values needed by the volume
  virtual float GetGradientMagnitudeScale();
  virtual float GetGradientMagnitudeBias();
  
//ETX



protected:
  vtkVolumeTextureMapper();
  ~vtkVolumeTextureMapper();
  vtkVolumeTextureMapper(const vtkVolumeTextureMapper &);
  void operator=(const vtkVolumeTextureMapper &);

  void InitializeRender( vtkRenderer *ren, vtkVolume *vol );

  // Objects / variables  needed for shading / gradient magnitude opacity
  vtkEncodedGradientEstimator  *GradientEstimator;
  vtkEncodedGradientShader     *GradientShader;
  int                          Shade;

  float          *GradientOpacityArray;
  unsigned char  *RGBAArray;
  int            ArraySize;

  float          *RedDiffuseShadingTable;
  float          *GreenDiffuseShadingTable;
  float          *BlueDiffuseShadingTable;
  float          *RedSpecularShadingTable;
  float          *GreenSpecularShadingTable;
  float          *BlueSpecularShadingTable;

  float          DataOrigin[3];
  float          DataSpacing[3];

  unsigned short *EncodedNormals;
  unsigned char  *GradientMagnitudes;

  float          SampleDistance;
  
  vtkRenderWindow *RenderWindow;
};


#endif


