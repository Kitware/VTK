/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastFunction.cxx
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

#include "vtkVolumeRayCastFunction.h"
#include "vtkVolumeRayCastMapper.h"
#include "vtkVolume.h"

// Description:
// Grab everything we need for rendering now. This procedure will be called
// during the initialization phase of ray casting. It is called once per 
// image. All Gets are done here for both performance and multithreading
// reentrant requirements reasons. At the end, the SpecificFunctionInitialize
// is called to give the subclass a chance to do its thing.
void vtkVolumeRayCastFunction::FunctionInitialize( vtkRenderer *ren, 
						   vtkVolume *vol,
						   vtkVolumeRayCastMapper *mapper,
						   float *opacity_tf_array,
						   float *rgb_tf_array,
						   float *gray_tf_array,
						   int tf_array_size )
{
  // First, just save the stuff that was passed in
  this->OpacityTFArray = opacity_tf_array;
  this->RGBTFArray     = rgb_tf_array;
  this->GrayTFArray    = gray_tf_array;
  this->TFArraySize    = tf_array_size;

  // Is shading on?
  this->Shading = vol->GetVolumeProperty()->GetShade();

  // How many color channels? Either 1 or 3. 1 means we have
  // to use the GrayTransferFunction, 3 means we use the
  // RGBTransferFunction
  this->ColorChannels = vol->GetVolumeProperty()->GetColorChannels();

  // What is the interpolation type? Nearest or linear.
  this->InterpolationType = vol->GetVolumeProperty()->GetInterpolationType();

  // What are the data increments? 
  // (One voxel, one row, and one slice offsets)
  mapper->GetDataIncrement( this->DataIncrement );

  // The size of the scalar input data
  mapper->GetScalarInput()->GetDimensions( this->DataSize );

  // Get the diffuse shading tables from the normal encoder
  // in the volume ray cast mapper
  this->RedDiffuseShadingTable = 
    mapper->GetNormalEncoder()->GetRedDiffuseShadingTable();
  this->GreenDiffuseShadingTable = 
    mapper->GetNormalEncoder()->GetGreenDiffuseShadingTable();
  this->BlueDiffuseShadingTable = 
    mapper->GetNormalEncoder()->GetBlueDiffuseShadingTable();

  // Get the specular shading tables from the normal encoder
  // in the volume ray cast mapper
  this->RedSpecularShadingTable = 
    mapper->GetNormalEncoder()->GetRedSpecularShadingTable();
  this->GreenSpecularShadingTable = 
    mapper->GetNormalEncoder()->GetGreenSpecularShadingTable();
  this->BlueSpecularShadingTable = 
    mapper->GetNormalEncoder()->GetBlueSpecularShadingTable();

  // Get the encoded normals from the normal encoder in the
  // volume ray cast mapper
  this->EncodedNormals = 
    mapper->GetNormalEncoder()->GetEncodedNormals();

  // Give the subclass a chance to do any initialization it needs
  // to do
  this->SpecificFunctionInitialize( ren, vol, mapper );
}

