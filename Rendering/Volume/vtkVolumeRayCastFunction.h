/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastFunction.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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

#include "vtkRenderingVolumeModule.h" // For export macro
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

} vtkVolumeRayCastDynamicInfo;

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
  vtkIdType                    DataIncrement[3];
  int                          DataSize[3];
  double                       DataSpacing[3];
  double                       DataOrigin[3];

  // Information from the vtkVolumeProperty
  int                          Shading;
  int                          ColorChannels;
  float                        Color[3];
  int                          InterpolationType;

  // The shading tables from the vtkEncodedGradientShader
  // that will be used for shading the volume.
  float                       *RedDiffuseShadingTable;
  float                       *GreenDiffuseShadingTable;
  float                       *BlueDiffuseShadingTable;
  float                       *RedSpecularShadingTable;
  float                       *GreenSpecularShadingTable;
  float                       *BlueSpecularShadingTable;

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
} vtkVolumeRayCastStaticInfo;

class VTKRENDERINGVOLUME_EXPORT vtkVolumeRayCastFunction : public vtkObject
{
public:
  vtkTypeMacro(vtkVolumeRayCastFunction,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  // Description:
  // Do the basic initialization. This includes saving the parameters
  // passed in into local variables, as well as grabbing some useful
  // info from the volume property and normal encoder. This initialize
  // routine is called once per render. It also calls the
  // SpecificFunctionInitialize of the subclass function.
  void FunctionInitialize( vtkRenderer *ren,
                           vtkVolume   *vol,
                           vtkVolumeRayCastStaticInfo *staticInfo );

  virtual void CastRay( vtkVolumeRayCastDynamicInfo *dynamicInfo,
                        vtkVolumeRayCastStaticInfo *staticInfo )=0;
//ETX

  // Description:
  // Get the value below which all scalar values are considered to
  // have 0 opacity.
  virtual float GetZeroOpacityThreshold( vtkVolume *vol )=0;

protected:
  vtkVolumeRayCastFunction() {}
  ~vtkVolumeRayCastFunction() {}

//BTX
  // Description:
  // This method gives the subclass a chance to do any special
  // initialization that it may need to do
  virtual void SpecificFunctionInitialize( vtkRenderer *ren,
                                           vtkVolume   *vol,
                                           vtkVolumeRayCastStaticInfo *staticInfo,
                                           vtkVolumeRayCastMapper *mapper )=0;
//ETX
private:
  vtkVolumeRayCastFunction(const vtkVolumeRayCastFunction&);  // Not implemented.
  void operator=(const vtkVolumeRayCastFunction&);  // Not implemented.
};

#endif
