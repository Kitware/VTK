/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFixedPointRayCastImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartVolumeMapper.h"

#include "vtkObjectFactory.h"

#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkFixedPointVolumeRayCastMapper.h"
#include "vtkEventForwarderCommand.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkImageResample.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeTextureMapper3D.h"
#include <cassert>

vtkStandardNewMacro( vtkSmartVolumeMapper );

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------
vtkSmartVolumeMapper::vtkSmartVolumeMapper()
{
  // Default for Window / Level - no adjustment
  this->FinalColorWindow  = 1.0;
  this->FinalColorLevel   = 0.5;

  // Our render mode is undefined at this point
  this->CurrentRenderMode = vtkSmartVolumeMapper::UndefinedRenderMode;

  // Nothing is initialized and we assume nothing is supported
  this->Initialized        = 0;
  this->TextureSupported   = 0;
  this->GPUSupported       = 0;
  this->RayCastSupported   = 0;
  this->LowResGPUNecessary = 0;
  this->InterpolationMode=VTK_RESLICE_CUBIC;

  // Create all the mappers we might need
  this->RayCastMapper   = vtkFixedPointVolumeRayCastMapper::New();
  this->GPUMapper       = vtkGPUVolumeRayCastMapper::New();
  this->MaxMemoryInBytes=this->GPUMapper->GetMaxMemoryInBytes();
  this->MaxMemoryFraction=this->GPUMapper->GetMaxMemoryFraction();

  this->TextureMapper   = vtkVolumeTextureMapper3D::New();
  this->GPULowResMapper = vtkGPUVolumeRayCastMapper::New();

  // If the render window has a desired update rate of at least 1 frame
  // per second or more, we'll consider this interactive
  this->InteractiveUpdateRate = 0.00001;

  // This is the resample filter that may be used if we need
  // a lower resolution version of the input for GPU rendering
  this->GPUResampleFilter = vtkImageResample::New();

  // Turn this on by default - this means that the sample spacing will be
  // automatically computed from the spacing of the input data. This is
  // also true for the GPU ray cast mapper.
  this->RayCastMapper->LockSampleDistanceToInputSpacingOn();

  // Default to the default mode - which will use the best option that
  // is supported by the hardware
  this->RequestedRenderMode = vtkSmartVolumeMapper::DefaultRenderMode;

  // Keep track of what blend mode we had when we initialized and
  // checked for hardware support - we need to recheck if the blend
  // mode changes
  this->InitializedBlendMode = -1;

  // Create the forwarding command
  vtkEventForwarderCommand *cb = vtkEventForwarderCommand::New();
  cb->SetTarget(this);

  // Now forward the ray caster's events
  this->RayCastMapper->AddObserver(vtkCommand::VolumeMapperRenderStartEvent, cb);
  this->RayCastMapper->AddObserver(vtkCommand::VolumeMapperRenderEndEvent, cb);
  this->RayCastMapper->AddObserver(vtkCommand::VolumeMapperRenderProgressEvent, cb);
  this->RayCastMapper->AddObserver(vtkCommand::VolumeMapperComputeGradientsStartEvent, cb);
  this->RayCastMapper->AddObserver(vtkCommand::VolumeMapperComputeGradientsEndEvent, cb);
  this->RayCastMapper->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, cb);

  // And the texture mapper's events
  this->TextureMapper->AddObserver(vtkCommand::StartEvent, cb);
  this->TextureMapper->AddObserver(vtkCommand::EndEvent, cb);
  this->TextureMapper->AddObserver(vtkCommand::ProgressEvent, cb);
  this->TextureMapper->AddObserver(vtkCommand::VolumeMapperRenderStartEvent, cb);
  this->TextureMapper->AddObserver(vtkCommand::VolumeMapperRenderEndEvent, cb);
  this->TextureMapper->AddObserver(vtkCommand::VolumeMapperRenderProgressEvent, cb);
  this->TextureMapper->AddObserver(vtkCommand::VolumeMapperComputeGradientsStartEvent, cb);
  this->TextureMapper->AddObserver(vtkCommand::VolumeMapperComputeGradientsEndEvent, cb);
  this->TextureMapper->AddObserver(vtkCommand::VolumeMapperComputeGradientsProgressEvent, cb);

  // And the GPU mapper's events
  // Commented out because too many events are being forwwarded
  // put back in after that is fixed
  /***
  this->GPUMapper->AddObserver(vtkCommand::VolumeMapperRenderStartEvent, cb);
  this->GPUMapper->AddObserver(vtkCommand::VolumeMapperRenderEndEvent, cb);
  this->GPUMapper->AddObserver(vtkCommand::VolumeMapperRenderProgressEvent, cb);
  ***/

  // And the low res GPU mapper's events
  // Commented out because too many events are being forwwarded
  // put back in after that is fixed
  /***
  this->GPULowResMapper->AddObserver(vtkCommand::VolumeMapperRenderStartEvent, cb);
  this->GPULowResMapper->AddObserver(vtkCommand::VolumeMapperRenderEndEvent, cb);
  this->GPULowResMapper->AddObserver(vtkCommand::VolumeMapperRenderProgressEvent, cb);
  ***/

  cb->Delete();
}

// ----------------------------------------------------------------------------
// Destructor
// ----------------------------------------------------------------------------
vtkSmartVolumeMapper::~vtkSmartVolumeMapper()
{
  this->RayCastMapper->Delete();
  this->GPUMapper->Delete();
  this->GPULowResMapper->Delete();
  this->TextureMapper->Delete();
  this->GPUResampleFilter->Delete();
}


// ----------------------------------------------------------------------------
// The Render method will determine the render mode and then render using the
// appropriate mapper. If the render mode is invalid (the user explicitly
// chooses something that is not supported) the render will silently fail.
// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::Render( vtkRenderer *ren, vtkVolume *vol )
{
  // Compute the render mode based on the requested
  // render mode, available hardware, and render window's
  // desired update rate
  this->ComputeRenderMode(ren,vol);

  vtkGPUVolumeRayCastMapper *usedMapper=0;

  switch ( this->CurrentRenderMode )
    {
    case vtkSmartVolumeMapper::RayCastRenderMode:
      this->RayCastMapper->Render(ren,vol);
      break;
    case vtkSmartVolumeMapper::TextureRenderMode:
      this->TextureMapper->Render(ren,vol);
      break;
    case vtkSmartVolumeMapper::GPURenderMode:
      if(this->LowResGPUNecessary)
        {
        usedMapper=this->GPULowResMapper;
        }
      else
        {
        usedMapper=this->GPUMapper;
        }
      usedMapper->SetAutoAdjustSampleDistances(
        ren->GetRenderWindow()->GetDesiredUpdateRate()>=
        this->InteractiveUpdateRate);
      usedMapper->Render(ren, vol);
      break;
    case vtkSmartVolumeMapper::InvalidRenderMode:
      // Silently fail - a render mode that is not
      // valid was selected so we will render nothing
      break;
    default:
      vtkErrorMacro("Internal Error!");
      break;
    }
}


// ----------------------------------------------------------------------------
// Initialize the rende
// We need to determine whether the texture mapper or GPU mapper are supported
// First we need to know what input scalar field we are working with to find
// out how many components it has. If it has more than one, and we are considering
// them to be independent components, then we know that neither the texture mapper
// nor the GPU mapper will work.
// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::Initialize(vtkRenderer *ren, vtkVolume *vol)
{

  if ( !this->GetInput() )
    {
    this->Initialized = 0;
    return;
    }

  int usingCellColors=0;
  vtkDataArray *scalars  = this->GetScalars(this->GetInput(), this->ScalarMode,
                                            this->ArrayAccessMode,
                                            this->ArrayId, this->ArrayName,
                                            usingCellColors);

  if ( scalars->GetNumberOfComponents() != 1 )
    {
    if ( vol->GetProperty()->GetIndependentComponents() )
      {
      this->TextureSupported = 0;
      this->GPUSupported     = 0;
      if ( usingCellColors )
        {
        this->RayCastSupported = 0;
        }
      else
        {
        this->RayCastSupported = 1;
        }
      this->Initialized      = 1;
      this->SupportStatusCheckTime.Modified();
      return;
      }
    }

  if ( usingCellColors )
    {
    this->RayCastSupported = 0;
    }
  else
    {
    this->RayCastSupported = 1;
    }

  // Make the window current because we need the OpenGL context
  vtkRenderWindow *win=ren->GetRenderWindow();
  win->MakeCurrent();

  // Have to give the texture mapper its input or else it won't report that
  // it is supported. Texture mapper only supported for composite blend
  if ( this->GetBlendMode() !=  vtkVolumeMapper::COMPOSITE_BLEND )
    {
    this->TextureSupported = 0;
    }
  else
    {
    this->ConnectMapperInput(this->TextureMapper);
    this->TextureSupported = this->TextureMapper->IsRenderSupported(
      vol->GetProperty(),ren);
    }

  this->GPUSupported = this->GPUMapper->IsRenderSupported(win,
                                                          vol->GetProperty());
  this->Initialized = 1;
  this->InitializedBlendMode = this->GetBlendMode();
  this->SupportStatusCheckTime.Modified();
}

// ----------------------------------------------------------------------------
// Compute the render mode based on what hardware is available, what the user
// requested as a render mode, and the desired update rate of the render window
// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::ComputeRenderMode(vtkRenderer *ren, vtkVolume *vol)
{
  // If we are already initialized, and the volume,
  // volume's input, and volume's property have not
  // changed since last time we computed the render mode,
  // then we don't need to initialize again
  if (!( this->Initialized &&
         this->SupportStatusCheckTime.GetMTime() > this->GetMTime() &&
         this->SupportStatusCheckTime.GetMTime() > vol->GetProperty()->GetMTime() &&
         this->SupportStatusCheckTime.GetMTime() > this->GetInput()->GetMTime() &&
         this->InitializedBlendMode == this->GetBlendMode() ) )
    {
    this->Initialize(ren,vol);
    }


  // Use this as the initial state to simplify the code below
  this->CurrentRenderMode = vtkSmartVolumeMapper::InvalidRenderMode;

  if ( !this->GetInput() )
    {
    return;
    }

  double scale[3];
  double spacing[3];
  this->GetInput()->GetSpacing(spacing);

  vtkRenderWindow *win=ren->GetRenderWindow();
  
  switch ( this->RequestedRenderMode )
    {
    // Requested ray casting - OK as long as it is supported
    // This ray caster is a software mapper so it is supported as
    // we aren't attempting to render cell scalars
    case vtkSmartVolumeMapper::RayCastRenderMode:
      if ( this->RayCastSupported )
        {
        this->CurrentRenderMode = vtkSmartVolumeMapper::RayCastRenderMode;
        }
      break;

    // Requested 3D texture - OK as long as it is supported
    case vtkSmartVolumeMapper::TextureRenderMode:
      if ( this->TextureSupported )
        {
        this->CurrentRenderMode = vtkSmartVolumeMapper::TextureRenderMode;
        }
      break;

    // Requested GPU - OK as long as it is supported
    case vtkSmartVolumeMapper::GPURenderMode:
      if ( this->GPUSupported )
        {
        this->CurrentRenderMode = vtkSmartVolumeMapper::GPURenderMode;
        }
      break;

      // Requested default mode - select GPU if supported, otherwise
      // select texture mapping for interactive rendering (if supported)
      // and ray casting for still rendering. Make determination of
      // still vs. interactive based on whether the desired update rate
      // is at or above this->InteractiveUpdateRate
    case vtkSmartVolumeMapper::DefaultRenderMode:
      // Go with GPU rendering if it is supported
      if ( this->GPUSupported )
        {
        this->CurrentRenderMode = vtkSmartVolumeMapper::GPURenderMode;
        }
      // If this is interactive, try for texture mapping
      else if ( win->GetDesiredUpdateRate() >= this->InteractiveUpdateRate &&
                this->TextureSupported )
        {
        this->CurrentRenderMode = vtkSmartVolumeMapper::TextureRenderMode;
        }
      else if ( this->RayCastSupported )
        {
        this->CurrentRenderMode = vtkSmartVolumeMapper::RayCastRenderMode;
        }
      break;

      // Requested the texture mapping / ray cast combo. If texture
      // mapping is supported and this is an interactive render, then
      // use it. Otherwise use ray casting.
    case vtkSmartVolumeMapper::RayCastAndTextureRenderMode:
      if ( win->GetDesiredUpdateRate() >= this->InteractiveUpdateRate &&
           this->TextureSupported )
        {
        this->CurrentRenderMode = vtkSmartVolumeMapper::TextureRenderMode;
        }
      else if ( this->RayCastSupported )
        {
        this->CurrentRenderMode = vtkSmartVolumeMapper::RayCastRenderMode;
        }
      break;

      // This should never happen since the SetRequestedRenderMode
      // protects against invalid states
    default:
      vtkErrorMacro("Internal Error: Invalid RequestedRenderMode");
      break;
    }

  switch ( this->CurrentRenderMode )
    {
    // We are rendering with the vtkFixedPointVolumeRayCastMapper
    case vtkSmartVolumeMapper::RayCastRenderMode:
      this->ConnectMapperInput(this->RayCastMapper);
      this->RayCastMapper->SetClippingPlanes(this->GetClippingPlanes());
      this->RayCastMapper->SetCropping(this->GetCropping());
      this->RayCastMapper->SetCroppingRegionPlanes(
        this->GetCroppingRegionPlanes());
      this->RayCastMapper->SetCroppingRegionFlags(
        this->GetCroppingRegionFlags());
      this->RayCastMapper->SetBlendMode( this->GetBlendMode() );
      this->RayCastMapper->SetFinalColorWindow(this->FinalColorWindow);
      this->RayCastMapper->SetFinalColorLevel(this->FinalColorLevel);
      break;

      // We are rendering with the vtkVolumeTextureMapper3D
    case vtkSmartVolumeMapper::TextureRenderMode:
      this->ConnectMapperInput(this->TextureMapper);
      if ( this->RequestedRenderMode == vtkSmartVolumeMapper::DefaultRenderMode ||
           this->RequestedRenderMode == vtkSmartVolumeMapper::RayCastAndTextureRenderMode )
        {
        this->TextureMapper->SetSampleDistance( static_cast<float>((spacing[0] + spacing[1] + spacing[2] ) / 2.0) );
        }
      else
        {
        this->TextureMapper->SetSampleDistance( static_cast<float>((spacing[0] + spacing[1] + spacing[2] ) / 6.0) );
        }
      this->TextureMapper->SetClippingPlanes(this->GetClippingPlanes());
      this->TextureMapper->SetCropping(this->GetCropping());
      this->TextureMapper->SetCroppingRegionPlanes(
        this->GetCroppingRegionPlanes());
      this->TextureMapper->SetCroppingRegionFlags(
        this->GetCroppingRegionFlags());
      // TextureMapper does not support FinalColor Window/Level.
      break;

      // We are rendering with the vtkGPUVolumeRayCastMapper
    case vtkSmartVolumeMapper::GPURenderMode:
      this->GPUMapper->SetMaxMemoryInBytes(this->MaxMemoryInBytes);
      this->GPUMapper->SetMaxMemoryFraction(this->MaxMemoryFraction);
      this->GPUMapper->SetSampleDistance(
        static_cast<float>((spacing[0] + spacing[1] + spacing[2] ) / 6.0) );
      this->ConnectMapperInput(this->GPUMapper);
      this->GPUMapper->SetClippingPlanes(this->GetClippingPlanes());
      this->GPUMapper->SetCropping(this->GetCropping());
      this->GPUMapper->SetCroppingRegionPlanes(
        this->GetCroppingRegionPlanes());
      this->GPUMapper->SetCroppingRegionFlags(
        this->GetCroppingRegionFlags());
      this->GPUMapper->SetBlendMode( this->GetBlendMode() );
      this->GPUMapper->SetFinalColorWindow(this->FinalColorWindow);
      this->GPUMapper->SetFinalColorLevel(this->FinalColorLevel);

      // Make the window current because we need the OpenGL context
      win->MakeCurrent();

      // Now we need to find out if we need to use a low resolution
      // version of the mapper for interactive rendering. This is true
      // if the GPU mapper cannot hand the size of the volume.
      this->GPUMapper->GetReductionRatio(scale);

      // if any of the scale factors is not 1.0, then we do need
      // to use the low res mapper for interactive rendering
      if ( scale[0] != 1.0 || scale[1] != 1.0 || scale[2] != 1.0 )
        {
        this->LowResGPUNecessary = 1;
        this->ConnectFilterInput(this->GPUResampleFilter);
        this->GPUResampleFilter->SetInterpolationMode(this->InterpolationMode);
        this->GPUResampleFilter->SetAxisMagnificationFactor( 0, scale[0]/2.0 );
        this->GPUResampleFilter->SetAxisMagnificationFactor( 1, scale[1]/2.0 );
        this->GPUResampleFilter->SetAxisMagnificationFactor( 2, scale[2]/2.0 );

        this->GPULowResMapper->SetMaxMemoryInBytes(this->MaxMemoryInBytes);
        this->GPULowResMapper->SetMaxMemoryFraction(this->MaxMemoryFraction);
        this->GPULowResMapper->SetSampleDistance(
        static_cast<float>((spacing[0] + spacing[1] + spacing[2] ) / 6.0) );

        this->GPULowResMapper->SetInputConnection(
          this->GPUResampleFilter->GetOutputPort());
        this->GPULowResMapper->SetClippingPlanes(this->GetClippingPlanes());
        this->GPULowResMapper->SetCropping(this->GetCropping());
        this->GPULowResMapper->SetCroppingRegionPlanes(
          this->GetCroppingRegionPlanes());
        this->GPULowResMapper->SetCroppingRegionFlags(
        this->GetCroppingRegionFlags());
        this->GPULowResMapper->SetBlendMode( this->GetBlendMode() );
        this->GPULowResMapper->SetFinalColorWindow(this->FinalColorWindow);
        this->GPULowResMapper->SetFinalColorLevel(this->FinalColorLevel);
        }
      else
        {
        this->LowResGPUNecessary = 0;
        }

      break;

      // The user selected a RequestedRenderMode that is
      // not supported. In this case the mapper will just
      // silently fail.
    case vtkSmartVolumeMapper::InvalidRenderMode:
      break;

      // This should never happen since we don't set the CurrentRenderMode
      // to anything other than the above handled options
    default:
      vtkErrorMacro("Internal Error: Invalid CurrentRenderMode");
      break;
    }
}

// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::ConnectMapperInput(vtkVolumeMapper *m)
{
  assert("pre: m_exists" && m!=0);

  vtkImageData *input2=m->GetInput();
  bool needShallowCopy=false;
  if(input2==0)
    {
    // make sure we not create a shallow copy each time to avoid
    // performance penalty.
    input2=vtkImageData::New();
    m->SetInputConnection(input2->GetProducerPort());
    input2->Delete();
    needShallowCopy=true;
    }
  else
    {
    needShallowCopy=input2->GetMTime()<this->GetInput()->GetMTime();
    }
  if(needShallowCopy)
    {
    input2->ShallowCopy(this->GetInput());
    }
}

// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::ConnectFilterInput(vtkImageResample *f)
{
  assert("pre: f_exists" && f!=0);

  vtkImageData *input2=static_cast<vtkImageData *>(f->GetInput());
  bool needShallowCopy=false;
  if(input2==0)
    {
    // make sure we not create a shallow copy each time to avoid
    // performance penalty.
    input2=vtkImageData::New();
    f->SetInputConnection(input2->GetProducerPort());
    input2->Delete();
    needShallowCopy=true;
    }
  else
    {
    needShallowCopy=input2->GetMTime()<this->GetInput()->GetMTime();
    }
  if(needShallowCopy)
    {
    input2->ShallowCopy(this->GetInput());
    }
}

// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::SetRequestedRenderMode(int mode)
{
  // If we aren't actually changing it, just return
  if ( this->RequestedRenderMode == mode )
    {
    return;
    }

  // Make sure it is a valid mode
  if ( mode < vtkSmartVolumeMapper::DefaultRenderMode ||
       mode > vtkSmartVolumeMapper::GPURenderMode )
    {
    vtkErrorMacro("Invalid Render Mode.");
    return;
    }

  this->RequestedRenderMode = mode;
  this->Modified();

}

// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::SetRequestedRenderModeToDefault()
{
  this->SetRequestedRenderMode(vtkSmartVolumeMapper::DefaultRenderMode);
}

// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::SetRequestedRenderModeToRayCastAndTexture()
{
  this->SetRequestedRenderMode(
    vtkSmartVolumeMapper::RayCastAndTextureRenderMode );
}

// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::SetRequestedRenderModeToRayCast()
{
  this->SetRequestedRenderMode(vtkSmartVolumeMapper::RayCastRenderMode);
}

// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::ReleaseGraphicsResources(vtkWindow *w)
{
  this->RayCastMapper->ReleaseGraphicsResources(w);
  this->TextureMapper->ReleaseGraphicsResources(w);
  this->GPUMapper->ReleaseGraphicsResources(w);
  this->GPULowResMapper->ReleaseGraphicsResources(w);

  this->Initialized      = 0;
  this->TextureSupported = 0;
  this->GPUSupported     = 0;
  this->RayCastSupported = 0;
}

// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::SetInterpolationModeToNearestNeighbor()
{
  this->SetInterpolationMode(VTK_RESLICE_NEAREST);
}

// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::SetInterpolationModeToLinear()
{
  this->SetInterpolationMode(VTK_RESLICE_LINEAR);
}

// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::SetInterpolationModeToCubic()
{
  this->SetInterpolationMode(VTK_RESLICE_CUBIC);
}

// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::CreateCanonicalView(
  vtkRenderer *ren,
  vtkVolume *volume,
  vtkVolume *volume2,
  vtkImageData *image,
  int blend_mode,
  double viewDirection[3],
  double viewUp[3])
{
  this->ComputeRenderMode(ren, volume);

  if ( this->CurrentRenderMode == vtkSmartVolumeMapper::GPURenderMode )
    {
    vtkVolumeProperty *savedProperty = volume->GetProperty();
    volume->SetProperty(volume2->GetProperty());
    volume->GetProperty()->Modified();
    volume->GetProperty()->GetScalarOpacity()->Modified();
    volume->GetProperty()->GetRGBTransferFunction()->Modified();
    this->GPUMapper->CreateCanonicalView(ren, volume,
                                         image, blend_mode,
                                         viewDirection, viewUp);
    volume->SetProperty(savedProperty);
    volume->GetProperty()->Modified();
    volume->GetProperty()->GetScalarOpacity()->Modified();
    volume->GetProperty()->GetRGBTransferFunction()->Modified();
    }
  else if ( this->RayCastSupported )
    {
    this->RayCastMapper->CreateCanonicalView(volume2,
                                             image, blend_mode,
                                             viewDirection, viewUp);
    }
  else
    {
    vtkErrorMacro("Could not create image - no available mapper");
    }
}

// ----------------------------------------------------------------------------
int vtkSmartVolumeMapper::GetLastUsedRenderMode()
{
  return this->CurrentRenderMode;
}

// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << "FinalColorWindow: " << this->FinalColorWindow << endl;
  os << "FinalColorLevel: " << this->FinalColorLevel << endl;
  os << "RequestedRenderMode: " << this->RequestedRenderMode << endl;
  os << "InteractiveUpdateRate: " << this->InteractiveUpdateRate << endl;
  os << "InterpolationMode: " << this->InterpolationMode << endl;
  os << "MaxMemoryInBytes:" << this->MaxMemoryInBytes << endl;
  os << "MaxMemoryFraction:" << this->MaxMemoryFraction << endl;
}
