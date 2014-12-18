/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeTextureMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVolumeTextureMapper - Abstract class for a volume mapper

// .SECTION Description
// vtkVolumeTextureMapper is the abstract definition of a volume mapper
// that uses a texture mapping approach.

// .SECTION see also
// vtkVolumeMapper

#ifndef vtkVolumeTextureMapper_h
#define vtkVolumeTextureMapper_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkVolumeMapper.h"

class vtkEncodedGradientEstimator;
class vtkEncodedGradientShader;
class vtkRenderWindow;
class vtkRenderer;
class vtkVolume;

class VTKRENDERINGVOLUME_EXPORT vtkVolumeTextureMapper : public vtkVolumeMapper
{
public:
  vtkTypeMacro(vtkVolumeTextureMapper,vtkVolumeMapper);
  void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Update the volume rendering pipeline by updating the scalar input
  virtual void Update();
  virtual void Update(int port);

  // Description:
  // Set / Get the gradient estimator used to estimate normals
  virtual void SetGradientEstimator( vtkEncodedGradientEstimator *gradest );
  vtkGetObjectMacro( GradientEstimator, vtkEncodedGradientEstimator );

  // Description:
  // Get the gradient shader.
  vtkGetObjectMacro( GradientShader, vtkEncodedGradientShader );

//BTX
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
  vtkGetVectorMacro( DataOrigin, double, 3 );
  vtkGetVectorMacro( DataSpacing, double, 3 );

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
  virtual float GetGradientMagnitudeScale(int)
    { return this->GetGradientMagnitudeScale(); };
  virtual float GetGradientMagnitudeBias(int)
    { return this->GetGradientMagnitudeBias(); };

//ETX



protected:
  vtkVolumeTextureMapper();
  ~vtkVolumeTextureMapper();

  void InitializeRender( vtkRenderer *ren, vtkVolume *vol );

  virtual void ReportReferences(vtkGarbageCollector*);

  // Objects / variables  needed for shading / gradient magnitude opacity
  vtkEncodedGradientEstimator  *GradientEstimator;
  vtkEncodedGradientShader     *GradientShader;
  int                           Shade;

  float          *GradientOpacityArray;
  unsigned char  *RGBAArray;
  int             ArraySize;
  int             NumberOfComponents;

  float          *RedDiffuseShadingTable;
  float          *GreenDiffuseShadingTable;
  float          *BlueDiffuseShadingTable;
  float          *RedSpecularShadingTable;
  float          *GreenSpecularShadingTable;
  float          *BlueSpecularShadingTable;

  double          DataOrigin[3];
  double          DataSpacing[3];

  unsigned short *EncodedNormals;
  unsigned char  *GradientMagnitudes;

  float           SampleDistance;

  vtkRenderWindow *RenderWindow;
private:
  vtkVolumeTextureMapper(const vtkVolumeTextureMapper&);  // Not implemented.
  void operator=(const vtkVolumeTextureMapper&);  // Not implemented.
};


#endif


