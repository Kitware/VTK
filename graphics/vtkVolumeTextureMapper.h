/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeTextureMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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
  const char *GetClassName() {return "vtkVolumeTextureMapper";};
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
  virtual void GetGradientMagnitudeRange( float range[2] );

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  virtual int GetMapperType() {return VTK_FRAMEBUFFER_VOLUME_MAPPER;};

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Render the volume
  virtual void Render(vtkRenderer *ren, vtkVolume *vol)=0;

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
//ETX


protected:
  vtkVolumeTextureMapper();
  ~vtkVolumeTextureMapper();
  vtkVolumeTextureMapper(const vtkVolumeTextureMapper &) {};
  void operator=(const vtkVolumeTextureMapper &)  {};

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


