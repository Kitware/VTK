/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastFunction.h
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

struct VolumeRayCastRayInfoStruct 
{
  // These are the input values
  float RayOrigin[3];
  float RayDirection[3];
  int   RayPixel[2];
  int   ImageWidth;

  // These are input values but may be changed
  // along the way
  float RayNearClip;
  float RayFarClip;

  // These are computed along the way
  float VolumeRayStart[4];
  float VolumeRayEnd[4];
  float VolumeRayDirection[4];
  float VolumeRayIncrement[3];
  int   VolumeRayNumberOfSamples;

  // These are the return values
  float RayColor[4];
  float RayDepth;
  int   VolumeRayStepsTaken;

};

struct VolumeRayCastVolumeInfoStruct 
{
  float WorldToVolumeMatrix[16];
  float ViewToVolumeMatrix[16];
  vtkVolume *Volume;
  int   ScalarDataType;
  void  *ScalarDataPointer;

  // Description:
  // These are some variables set during FunctionInitialize. They
  // are either passed into that function, or acquired using Get 
  // methods and saved locally for performance reasons. They are
  // public because they need to be accessed by a templated method
  // which is not a member method.
  int                          Shading;
  int                          ColorChannels;
  float                        Color[3];
  int                          InterpolationType;
  float                        *RedDiffuseShadingTable;
  float                        *GreenDiffuseShadingTable;
  float                        *BlueDiffuseShadingTable;
  float                        *RedSpecularShadingTable;
  float                        *GreenSpecularShadingTable;
  float                        *BlueSpecularShadingTable;
};


class VTK_EXPORT vtkVolumeRayCastFunction : public vtkObject
{
public:
  const char *GetClassName() {return "vtkVolumeRayCastFunction";};

  // Description:
  // Do the basic initialization. This includes saving the parameters
  // passed in into local variables, as well as grabbing some useful
  // info from the volume property and normal encoder. This initialize
  // routine is called once per render. It also calls the 
  // SpecificFunctionInitialize of the subclass function.
//BTX
  void FunctionInitialize( vtkRenderer *ren,
			   vtkVolume   *vol,
			   struct VolumeRayCastVolumeInfoStruct *volumeInfo,
			   vtkVolumeRayCastMapper *mapper );

  virtual void CastRay( struct VolumeRayCastRayInfoStruct *rayInfo,
			struct VolumeRayCastVolumeInfoStruct *volumeInfo )=0;
//ETX

  // Description:
  // Get the value below which all scalar values are considered to
  // have 0 opacity.
  virtual float GetZeroOpacityThreshold( vtkVolume *vol )=0;

  int                          DataIncrement[3];
  int                          DataSize[3];
  unsigned short               *EncodedNormals;
  unsigned char                *GradientMagnitudes;

protected:

  // Description:
  // This method gives the subclass a chance to do any special
  // initialization that it may need to do
//BTX
  virtual void SpecificFunctionInitialize( vtkRenderer *ren,
					   vtkVolume   *vol,
					   struct VolumeRayCastVolumeInfoStruct *volumeInfo,
					   vtkVolumeRayCastMapper *mapper )=0;
//ETX
};

#endif
