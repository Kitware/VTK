/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastStructures.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen.

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
