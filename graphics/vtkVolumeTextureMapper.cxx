/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeTextureMapper.cxx
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
    this->GetInput()->Update();
    }

  if ( this->GetRGBTextureInput() )
    {
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

  size = vol->GetArraySize();

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
      this->RGBAArray[j++] = RGBArray[k++]*255.99;
      this->RGBAArray[j++] = RGBArray[k++]*255.99;
      this->RGBAArray[j++] = RGBArray[k++]*255.99;
      this->RGBAArray[j++] = AArray[i]*255.99*gradientOpacityConstant;
      }
    }
  else if ( colorChannels == 1 )
    {
    GArray = vol->GetGrayArray();
    for ( i=0, j=0; i < size; i++ )
      {
      this->RGBAArray[j++] = GArray[i]*255.99;
      this->RGBAArray[j++] = AArray[i]*255.99*gradientOpacityConstant;
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
