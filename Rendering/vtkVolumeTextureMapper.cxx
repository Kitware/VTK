/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeTextureMapper.cxx
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
#include "vtkVolumeTextureMapper.h"
#include "vtkVolume.h"
#include "vtkRenderer.h"
#include "vtkFiniteDifferenceGradientEstimator.h"

vtkVolumeTextureMapper::vtkVolumeTextureMapper()
{
  this->GradientOpacityArray    = NULL;
  this->RGBAArray               = NULL;
  this->ArraySize               = -1;
  this->SampleDistance          = 1.0;
  this->GradientEstimator       = vtkFiniteDifferenceGradientEstimator::New();
  this->GradientShader          = vtkEncodedGradientShader::New();
}

vtkVolumeTextureMapper::~vtkVolumeTextureMapper()
{
  this->SetGradientEstimator( NULL );
  this->GradientShader->Delete();
  
  if ( this->RGBAArray )
    {
    delete [] this->RGBAArray;
    }
}

void vtkVolumeTextureMapper::SetGradientEstimator( 
				      vtkEncodedGradientEstimator *gradest )
{

  // If we are setting it to its current value, don't do anything
  if ( this->GradientEstimator == gradest )
    {
    return;
    }

  // If we already have a gradient estimator, unregister it.
  if ( this->GradientEstimator )
    {
    this->GradientEstimator->UnRegister(this);
    this->GradientEstimator = NULL;
    }

  // If we are passing in a non-NULL estimator, register it
  if ( gradest )
    {
    gradest->Register( this );
    }

  // Actually set the estimator, and consider the object Modified
  this->GradientEstimator = gradest;
  this->Modified();
}

void vtkVolumeTextureMapper::Update()
{
  if ( this->GetInput() )
    {
    this->GetInput()->UpdateInformation();
    this->GetInput()->SetUpdateExtentToWholeExtent();
    this->GetInput()->Update();
    }

  if ( this->GetRGBTextureInput() )
    {
    this->GetRGBTextureInput()->UpdateInformation();
    this->GetRGBTextureInput()->SetUpdateExtentToWholeExtent();
    this->GetRGBTextureInput()->Update();
    }
}

void vtkVolumeTextureMapper::InitializeRender( vtkRenderer *ren,
					       vtkVolume *vol )
{
  int   size, i, j, k;
  float *AArray;
  float *RGBArray;
  float *GArray;
  int   colorChannels;
  float gradientOpacityConstant;
  
  // Hang on to the render window - we'll need it to test for abort
  this->RenderWindow = ren->GetRenderWindow();

  vol->UpdateTransferFunctions( ren );

  vol->UpdateScalarOpacityforSampleSize( ren, this->SampleDistance );

  colorChannels = vol->GetProperty()->GetColorChannels();

  size = (int) vol->GetArraySize();

  if ( this->ArraySize != size )
    {
    if ( this->RGBAArray )
      {
      delete [] this->RGBAArray;
      }

    this->RGBAArray             = new unsigned char[4*size];    
    this->ArraySize = size;
    }

  this->GradientOpacityArray = vol->GetGradientOpacityArray();

  AArray = vol->GetCorrectedScalarOpacityArray();

  // Being less than 0.0 implies a transfer function, so just multiply by
  // 1.0 here since the transfer function will supply the true opacity
  // modulation value
  gradientOpacityConstant = vol->GetGradientOpacityConstant();
  if ( gradientOpacityConstant <= 0.0 )
    {
    gradientOpacityConstant = 1.0;
    }

  if ( colorChannels == 3 )
    {
    RGBArray = vol->GetRGBArray();    
    for ( i=0, j=0, k=0; i < size; i++ )
      {
      this->RGBAArray[j++] = (unsigned char) (0.5 + (RGBArray[k++]*255.0));
      this->RGBAArray[j++] = (unsigned char) (0.5 + (RGBArray[k++]*255.0));
      this->RGBAArray[j++] = (unsigned char) (0.5 + (RGBArray[k++]*255.0));
      this->RGBAArray[j++] = (unsigned char) (0.5 + AArray[i]*255.0*gradientOpacityConstant);
      }
    }
  else if ( colorChannels == 1 )
    {
    GArray = vol->GetGrayArray();
    for ( i=0, j=0; i < size; i++ )
      {
      this->RGBAArray[j++] = (unsigned char) (0.5 + (GArray[i]*255.0));
      this->RGBAArray[j++] = (unsigned char) (0.5 + (GArray[i]*255.0));
      this->RGBAArray[j++] = (unsigned char) (0.5 + (GArray[i]*255.0));
      this->RGBAArray[j++] = (unsigned char) (0.5 + AArray[i]*255.0*gradientOpacityConstant);
      }
    }

  this->Shade =  vol->GetProperty()->GetShade();  

  this->GradientEstimator->SetInput( this->GetInput() );

  if ( this->Shade )
    {
    this->GradientShader->UpdateShadingTable( ren, vol, 
					      this->GradientEstimator );
    this->EncodedNormals = 
      this->GradientEstimator->GetEncodedNormals();

    this->RedDiffuseShadingTable = 
      this->GradientShader->GetRedDiffuseShadingTable(vol);
    this->GreenDiffuseShadingTable = 
      this->GradientShader->GetGreenDiffuseShadingTable(vol);
    this->BlueDiffuseShadingTable = 
      this->GradientShader->GetBlueDiffuseShadingTable(vol);
    
    this->RedSpecularShadingTable = 
      this->GradientShader->GetRedSpecularShadingTable(vol);
    this->GreenSpecularShadingTable = 
      this->GradientShader->GetGreenSpecularShadingTable(vol);
    this->BlueSpecularShadingTable = 
      this->GradientShader->GetBlueSpecularShadingTable(vol);    
    }
  else
    {
    this->EncodedNormals = NULL;
    this->RedDiffuseShadingTable = NULL;
    this->GreenDiffuseShadingTable = NULL;
    this->BlueDiffuseShadingTable = NULL;
    this->RedSpecularShadingTable = NULL;
    this->GreenSpecularShadingTable = NULL;
    this->BlueSpecularShadingTable = NULL;
    }

  // If we have non-constant opacity on the gradient magnitudes,
  // we need to use the gradient magnitudes to look up the opacity
  if ( vol->GetGradientOpacityConstant() == -1.0 )
    {
    this->GradientMagnitudes = 
      this->GradientEstimator->GetGradientMagnitudes();
    }
  else
    {
    this->GradientMagnitudes = NULL;
    }

  this->GetInput()->GetOrigin( this->DataOrigin );
  this->GetInput()->GetSpacing( this->DataSpacing );
}

float vtkVolumeTextureMapper::GetGradientMagnitudeScale()
{
  if ( !this->GradientEstimator ) 
    {
    vtkErrorMacro( "You must have a gradient estimator set to get the scale" );
    return 1.0;
    }
  
  return this->GradientEstimator->GetGradientMagnitudeScale();  
}

float vtkVolumeTextureMapper::GetGradientMagnitudeBias()
{
  if ( !this->GradientEstimator ) 
    {
    vtkErrorMacro( "You must have a gradient estimator set to get the bias" );
    return 1.0;
    }
  
  return this->GradientEstimator->GetGradientMagnitudeBias();  
}

// Print the vtkVolumeTextureMapper
void vtkVolumeTextureMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkVolumeMapper::PrintSelf(os,indent);

  if ( this->GradientEstimator )
    {
      os << indent << "Gradient Estimator: " << (this->GradientEstimator) <<
	endl;
    }
  else
    {
      os << indent << "Gradient Estimator: (none)" << endl;
    }

  if ( this->GradientShader )
    {
      os << indent << "Gradient Shader: " << (this->GradientShader) << endl;
    }
  else
    {
      os << indent << "Gradient Shader: (none)" << endl;
    }

  // this->Shade is a temporary variable that should not be printed
  // this->RenderWindow is a temporary variable that should not be printed
  // this->DataSpacing is a temporary variable that should not be printed
  // this->DataOrigin is a temporary variable that should not be printed
}
