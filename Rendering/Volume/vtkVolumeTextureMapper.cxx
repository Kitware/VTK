/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeTextureMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolumeTextureMapper.h"

#include "vtkDataArray.h"
#include "vtkEncodedGradientShader.h"
#include "vtkFiniteDifferenceGradientEstimator.h"
#include "vtkGarbageCollector.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"

vtkVolumeTextureMapper::vtkVolumeTextureMapper()
{
  this->GradientOpacityArray    = NULL;
  this->RGBAArray               = NULL;
  this->ArraySize               = -1;
  this->SampleDistance          = 1.0;
  this->GradientEstimator       = vtkFiniteDifferenceGradientEstimator::New();
  this->GradientShader          = vtkEncodedGradientShader::New();
  this->NumberOfComponents      = 1;

  this->RenderWindow            = NULL;
  this->DataOrigin[0]           = 0.0;
  this->DataOrigin[1]           = 0.0;
  this->DataOrigin[2]           = 0.0;
  this->DataSpacing[0]          = 1.0;
  this->DataSpacing[1]          = 1.0;
  this->DataSpacing[2]          = 1.0;
}

vtkVolumeTextureMapper::~vtkVolumeTextureMapper()
{
  this->SetGradientEstimator( NULL );
  this->GradientShader->Delete();

  delete [] this->RGBAArray;
  delete [] this->GradientOpacityArray;
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

int vtkVolumeTextureMapper::ProcessRequest(vtkInformation* request,
                                           vtkInformationVector** inputVector,
                                           vtkInformationVector*)
{
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    int* wholeExt = inInfo->Get(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), wholeExt, 6);
  }
  return 1;
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

  size = static_cast<int>(vol->GetArraySize());

  int numComponents = this->GetInput()->
    GetPointData()->GetScalars()->GetNumberOfComponents();

  if ( this->ArraySize != size ||
       this->NumberOfComponents != numComponents )
  {
    delete [] this->RGBAArray;
    delete [] this->GradientOpacityArray;

    this->RGBAArray            = new unsigned char [4*size*numComponents];
    this->GradientOpacityArray = new float [256*numComponents];
    this->ArraySize            = size;
    this->NumberOfComponents   = numComponents;
  }

  float *goPtr;
  float *goArray;

  for ( int c = 0; c < numComponents; c++ )
  {
    goPtr = vol->GetGradientOpacityArray(c);
    goArray = this->GradientOpacityArray + c;

    for ( i = 0; i < 256; i++ )
    {
      *(goArray) = *(goPtr++);
      goArray += numComponents;
    }

    AArray = vol->GetCorrectedScalarOpacityArray(c);
    colorChannels = vol->GetProperty()->GetColorChannels(c);


    // Being less than 0.0 implies a transfer function, so just multiply by
    // 1.0 here since the transfer function will supply the true opacity
    // modulation value
    gradientOpacityConstant = vol->GetGradientOpacityConstant(c);
    if ( gradientOpacityConstant <= 0.0 )
    {
      gradientOpacityConstant = 1.0;
    }

    if ( colorChannels == 3 )
    {
      RGBArray = vol->GetRGBArray(c);
      for ( i=0, j=(c*4), k=0; i < size; i++ )
      {
        this->RGBAArray[j++] = static_cast<unsigned char>(
          0.5 + (RGBArray[k++]*255.0));
        this->RGBAArray[j++] = static_cast<unsigned char>(
          0.5 + (RGBArray[k++]*255.0));
        this->RGBAArray[j++] = static_cast<unsigned char>(
          0.5 + (RGBArray[k++]*255.0));
        this->RGBAArray[j++] = static_cast<unsigned char>(
          0.5 + AArray[i]*255.0*gradientOpacityConstant);

        j += 4*(numComponents-1);
      }
    }
    else if ( colorChannels == 1 )
    {
      GArray = vol->GetGrayArray(c);
      for ( i=0, j=(c*4); i < size; i++ )
      {
        this->RGBAArray[j++] = static_cast<unsigned char>(
          0.5 + (GArray[i]*255.0));
        this->RGBAArray[j++] = static_cast<unsigned char>(
          0.5 + (GArray[i]*255.0));
        this->RGBAArray[j++] = static_cast<unsigned char>(
          0.5 + (GArray[i]*255.0));
        this->RGBAArray[j++] = static_cast<unsigned char>(
          0.5 + AArray[i]*255.0*gradientOpacityConstant);

        j += 4*(numComponents-1);
      }
    }
  }

  this->Shade =  vol->GetProperty()->GetShade();

  this->GradientEstimator->SetInputData( this->GetInput() );

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

  this->ConvertCroppingRegionPlanesToVoxels();
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
  this->Superclass::PrintSelf(os,indent);

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

//----------------------------------------------------------------------------
void vtkVolumeTextureMapper::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // These filters share our input and are therefore involved in a
  // reference loop.
  vtkGarbageCollectorReport(collector, this->GradientEstimator,
                            "GradientEstimator");
}
