/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastFunction.cxx
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
				VTKRayCastVolumeInfo *volumeInfo,
				vtkVolumeRayCastMapper *mapper )
{
  // Is shading on?
  volumeInfo->Shading = vol->GetProperty()->GetShade();

  // How many color channels? Either 1 or 3. 1 means we have
  // to use the GrayTransferFunction, 3 means we use the
  // RGBTransferFunction
  volumeInfo->ColorChannels = vol->GetProperty()->GetColorChannels();

  // What is the interpolation type? Nearest or linear.
  volumeInfo->InterpolationType = vol->GetProperty()->GetInterpolationType();

  // Get the size, spacing and origin of the scalar data
  mapper->GetInput()->GetDimensions( volumeInfo->DataSize );
  mapper->GetInput()->GetSpacing( volumeInfo->DataSpacing );
  mapper->GetInput()->GetOrigin( volumeInfo->DataOrigin );

  // What are the data increments? 
  // (One voxel, one row, and one slice offsets)
  volumeInfo->DataIncrement[0] = 1;
  volumeInfo->DataIncrement[1] = volumeInfo->DataSize[0];
  volumeInfo->DataIncrement[2] = volumeInfo->DataSize[0] * volumeInfo->DataSize[1];


  // If there is rgbTexture, then get the info about this
  if ( mapper->GetRGBTextureInput() )
    {
    mapper->GetRGBTextureInput()->GetDimensions( volumeInfo->RGBDataSize );
    mapper->GetRGBTextureInput()->GetSpacing( volumeInfo->RGBDataSpacing );
    mapper->GetRGBTextureInput()->GetOrigin( volumeInfo->RGBDataOrigin );
    volumeInfo->RGBDataIncrement[0] = 3;
    volumeInfo->RGBDataIncrement[1] = 3*volumeInfo->RGBDataSize[0];
    volumeInfo->RGBDataIncrement[2] = 3*( volumeInfo->RGBDataSize[0] * 
					  volumeInfo->RGBDataSize[1] );

    volumeInfo->RGBDataPointer = (unsigned char *)
      mapper->GetRGBTextureInput()->GetPointData()->GetScalars()->GetVoidPointer(0);

    volumeInfo->RGBTextureCoefficient = vol->GetProperty()->GetRGBTextureCoefficient();
    }
  else
    {
    volumeInfo->RGBDataPointer = NULL;
    }
  // Get the encoded normals from the normal encoder in the
  // volume ray cast mapper. We need to do this if shading is on
  // or if we are classifying scalar value into opacity based
  // on the magnitude of the gradient (since if we need to
  // calculate the magnitude we might as well just keep the
  // direction as well.
  if ( volumeInfo->Shading )
    {
    volumeInfo->EncodedNormals = 
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
    volumeInfo->EncodedNormals            = NULL;
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
    volumeInfo->GradientMagnitudes = 
      mapper->GetGradientEstimator()->GetGradientMagnitudes();
    }
  else
    {
    volumeInfo->GradientMagnitudes = NULL;
    }

  // Give the subclass a chance to do any initialization it needs
  // to do
  this->SpecificFunctionInitialize( ren, vol, volumeInfo, mapper );
}



