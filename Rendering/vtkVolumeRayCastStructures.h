/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastStructures.h
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

// .NAME vtkVolumeRayCastStructures - structures for ray casting volumes

// .SECTION Description
// These are the structures required for the ray casting of volumes

// .SECTION See Also
// vtkRayCaster vtkRayCastStructures vtkVolumeRayCastMapper

#ifndef __vtkVolumeRayCastStructures_h
#define __vtkVolumeRayCastStructures_h

class vtkVolume;

typedef struct 
{
  // A pointer to the volume
  vtkVolume                    *Volume;

  // Matrices for switching from view to volume coordinate, and back
  float                        WorldToVolumeMatrix[16];
  float                        VolumeToWorldMatrix[16];
  float                        ViewToVolumeMatrix[16];

  // The distance from the camera to the center of the volume
  // To be used as a simple depth return value
  float                        CenterDistance;

  // The type of the data and a pointer to it, and the information
  // about its size, spacing, origin and precomputed increment
  int                          ScalarDataType;
  void                         *ScalarDataPointer;
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
  float                        *RedDiffuseShadingTable;
  float                        *GreenDiffuseShadingTable;
  float                        *BlueDiffuseShadingTable;
  float                        *RedSpecularShadingTable;
  float                        *GreenSpecularShadingTable;
  float                        *BlueSpecularShadingTable;

  // Info needed to do solid textures - a pointer to the texture
  // and info about its size, spacing, origin, and precomputed 
  // increments
  unsigned char                *RGBDataPointer;
  int                          RGBDataIncrement[3];
  int                          RGBDataSize[3];
  float                        RGBDataSpacing[3];
  float                        RGBDataOrigin[3];

  // Info needed from the gradient estimator
  unsigned short               *EncodedNormals;
  unsigned char                *GradientMagnitudes;

} VTKRayCastVolumeInfo;


#endif
