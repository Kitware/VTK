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

// Grab everything we need for rendering now. This procedure will be called
// during the initialization phase of ray casting. It is called once per 
// image. All Gets are done here for both performance and multithreading
// reentrant requirements reasons. At the end, the SpecificFunctionInitialize
// is called to give the subclass a chance to do its thing.
void vtkVolumeRayCastFunction::FunctionInitialize( 
				vtkRenderer *ren, 
				vtkVolume *vol,
				struct VolumeRayCastVolumeInfoStruct *volumeInfo,
				vtkVolumeRayCastMapper *mapper )
{
  // Is shading on?
  volumeInfo->Shading = vol->GetVolumeProperty()->GetShade();

  // How many color channels? Either 1 or 3. 1 means we have
  // to use the GrayTransferFunction, 3 means we use the
  // RGBTransferFunction
  volumeInfo->ColorChannels = vol->GetVolumeProperty()->GetColorChannels();

  // What is the interpolation type? Nearest or linear.
  volumeInfo->InterpolationType = vol->GetVolumeProperty()->GetInterpolationType();

  // What are the data increments? 
  // (One voxel, one row, and one slice offsets)
  mapper->GetDataIncrement( this->DataIncrement );

  // The size of the scalar input data
  mapper->GetScalarInput()->GetDimensions( this->DataSize );

  // Get the encoded normals from the normal encoder in the
  // volume ray cast mapper. We need to do this if shading is on
  // or if we are classifying scalar value into opacity based
  // on the magnitude of the gradient (since if we need to
  // calculate the magnitude we might as well just keep the
  // direction as well.
  if ( volumeInfo->Shading )
    {
    this->EncodedNormals = 
      mapper->GetGradientEstimator()->GetEncodedNormals();

    // Get the diffuse shading tables from the normal encoder
    // in the volume ray cast mapper
    volumeInfo->RedDiffuseShadingTable = 
      mapper->GetGradientShader()->GetRedDiffuseShadingTable(vol);
    volumeInfo->GreenDiffuseShadingTable = 
      mapper->GetGradientShader()->GetGreenDiffuseShadingTable(vol);
    volumeInfo->BlueDiffuseShadingTable = 
      mapper->GetGradientShader()->GetBlueDiffuseShadingTable(vol);
    
    // Get the specular shading tables from the normal encoder
    // in the volume ray cast mapper
    volumeInfo->RedSpecularShadingTable = 
      mapper->GetGradientShader()->GetRedSpecularShadingTable(vol);
    volumeInfo->GreenSpecularShadingTable = 
      mapper->GetGradientShader()->GetGreenSpecularShadingTable(vol);
    volumeInfo->BlueSpecularShadingTable = 
      mapper->GetGradientShader()->GetBlueSpecularShadingTable(vol);
    }
  else
    {
    this->EncodedNormals            = NULL;
    volumeInfo->RedDiffuseShadingTable    = NULL;
    volumeInfo->GreenDiffuseShadingTable  = NULL;
    volumeInfo->BlueDiffuseShadingTable   = NULL;
    volumeInfo->RedSpecularShadingTable   = NULL;
    volumeInfo->GreenSpecularShadingTable = NULL;
    volumeInfo->BlueSpecularShadingTable  = NULL;
    }

  // We need the gradient magnitudes only if we are classifying opacity
  // based on them. Otherwise we can just leave them NULL
  if ( vol->GetGradientOpacityArray() && 
       vol->GetGradientOpacityConstant() == -1.0 )
    {
    this->GradientMagnitudes = 
      mapper->GetGradientEstimator()->GetGradientMagnitudes();
    }
  else
    {
    this->GradientMagnitudes = NULL;
    }

  // Give the subclass a chance to do any initialization it needs
  // to do
  this->SpecificFunctionInitialize( ren, vol, volumeInfo, mapper );
}



