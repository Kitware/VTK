/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastFunction.h
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

// .NAME vtkVolumeRayCastFunction - a superclass for ray casting functions

// .SECTION Description
// vtkVolumeRayCastFunction is a superclass for ray casting functions that 
// can be used within a vtkVolumeRayCastMapper. This includes for example,
// vtkVolumeRayCastCompositeFunction, vtkVolumeRayCastMIPFunction, and
// vtkVolumeRayCastIsosurfaceFunction.

// .SECTION See Also
// vtkVolumeRayCastCompositeFunction vtkVolumeRayCastMIPFunction
// vtkVolumeRayCastIsosurfaceFunction vtkVolumeRayCastMapper

#ifndef __vtkVolumeRayCastFunction_h
#define __vtkVolumeRayCastFunction_h

#include "vtkObject.h"

class vtkRenderer;
class vtkVolume;
class vtkVolumeRayCastMapper;

// Define a couple of structures we need to hold all the important information
// This first structure hold the dynamic information - stuff that changes per 
// ray
typedef struct 
{
  // These are the return values - RGBA
  float Color[4];
  float ScalarValue;

  // Ray information transformed into local coordinates
  float                        TransformedStart[4];
  float                        TransformedEnd[4];
  float                        TransformedDirection[4];
  float                        TransformedIncrement[3];
  
  // The number of steps we want to take if this is
  // a ray caster that takes steps
  int                          NumberOfStepsToTake;
  
  // The number of steps we actually take if this is
  // a ray caster that takes steps
  int                          NumberOfStepsTaken;

} VTKVRCDynamicInfo;

// This second structure hold the static information - things that don't
// change over the whole image
typedef struct 
{
  // A pointer to the volume
  vtkVolume                   *Volume;

  // A pointer to the renderer
  vtkRenderer                 *Renderer;
  
  // Matrices for switching from view to volume coordinate, and back
  float                        WorldToVoxelsMatrix[16];
  float                        VoxelsToWorldMatrix[16];
  float                        ViewToVoxelsMatrix[16];

  float                       *ClippingPlane;
  int                          NumberOfClippingPlanes;
  
  // The camera thickness (distance between near and far) is necessary 
  // for computing sampling distance
  float                        CameraThickness;

  // The type of the data and a pointer to it, and the information
  // about its size, spacing, origin and precomputed increment
  int                          ScalarDataType;
  void                        *ScalarDataPointer;
  int                          DataIncrement[3];
  int                          DataSize[3];
  float                        DataSpacing[3];
  float                        DataOrigin[3];

  // Information from the vtkVolumeProperty
  int                          Shading;
  int                          ColorChannels;
  float                        Color[3];
  int                          InterpolationType;
  float                        RGBTextureCoefficient;

  // The shading tables from the vtkEncodedGradientShader
  // that will be used for shading the volume.
  float                       *RedDiffuseShadingTable;
  float                       *GreenDiffuseShadingTable;
  float                       *BlueDiffuseShadingTable;
  float                       *RedSpecularShadingTable;
  float                       *GreenSpecularShadingTable;
  float                       *BlueSpecularShadingTable;

  // Info needed to do solid textures - a pointer to the texture
  // and info about its size, spacing, origin, and precomputed 
  // increments
  unsigned char               *RGBDataPointer;
  int                          RGBDataIncrement[3];
  int                          RGBDataSize[3];
  float                        RGBDataSpacing[3];
  float                        RGBDataOrigin[3];

  // Info needed from the gradient estimator
  unsigned short               *EncodedNormals;
  unsigned char                *GradientMagnitudes;

  // Image information
  int                          ImageInUseSize[2];
  int                          ImageMemorySize[2];
  int                          ImageViewportSize[2];
  int                          ImageOrigin[2];
  unsigned char               *Image;
  
  int                         *RowBounds;

  // Is a MIP ray cast function in use? This will control
  // how subsegments of the ray are combined when non-subvolume 
  // cropping is used. If maximize opacity is used, the color[3] 
  // value is used to find the max othersize the dynamicInfo->ScalarValue
  // value is used
  int                          MIPFunction;
  int                          MaximizeOpacity;
} VTKVRCStaticInfo;

class VTK_RENDERING_EXPORT vtkVolumeRayCastFunction : public vtkObject
{
public:
  vtkTypeMacro(vtkVolumeRayCastFunction,vtkObject);

  // Description:
  // Do the basic initialization. This includes saving the parameters
  // passed in into local variables, as well as grabbing some useful
  // info from the volume property and normal encoder. This initialize
  // routine is called once per render. It also calls the 
  // SpecificFunctionInitialize of the subclass function.
//BTX
  void FunctionInitialize( vtkRenderer *ren,
			   vtkVolume   *vol,
			   VTKVRCStaticInfo *staticInfo );

  virtual void CastRay( VTKVRCDynamicInfo *dynamicInfo,
			VTKVRCStaticInfo *staticInfo )=0;
//ETX

  // Description:
  // Get the value below which all scalar values are considered to
  // have 0 opacity.
  virtual float GetZeroOpacityThreshold( vtkVolume *vol )=0;

protected:
  vtkVolumeRayCastFunction() {};
  ~vtkVolumeRayCastFunction() {};

  // Description:
  // This method gives the subclass a chance to do any special
  // initialization that it may need to do
//BTX
  virtual void SpecificFunctionInitialize( vtkRenderer *ren,
					   vtkVolume   *vol,
					   VTKVRCStaticInfo *staticInfo,
					   vtkVolumeRayCastMapper *mapper )=0;
//ETX
private:
  vtkVolumeRayCastFunction(const vtkVolumeRayCastFunction&);  // Not implemented.
  void operator=(const vtkVolumeRayCastFunction&);  // Not implemented.
};

#endif
