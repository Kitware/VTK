/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastStructures.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

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
