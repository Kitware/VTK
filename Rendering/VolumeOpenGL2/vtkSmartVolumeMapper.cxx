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
#include <cassert>

#include "vtkSmartVolumeMapper.h"
#include "vtkObjectFactory.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkFixedPointVolumeRayCastMapper.h"
#include "vtkEventForwarderCommand.h"
#include "vtkImageData.h"
#include "vtkImageResample.h"
#include "vtkOSPRayVolumeInterface.h"
#include "vtkPiecewiseFunction.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageMagnitude.h"
#include "vtkPointData.h"


vtkStandardNewMacro( vtkSmartVolumeMapper );

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------
vtkSmartVolumeMapper::vtkSmartVolumeMapper()
: VectorMode(DISABLED)
{
  // Default for Window / Level - no adjustment
  this->FinalColorWindow  = 1.0;
  this->FinalColorLevel   = 0.5;

  // Our render mode is undefined at this point
  this->CurrentRenderMode = vtkSmartVolumeMapper::UndefinedRenderMode;

  // Nothing is initialized and we assume nothing is supported
  this->Initialized        = 0;
  this->GPUSupported       = 0;
  this->RayCastSupported   = 0;
  this->LowResGPUNecessary = 0;
  this->InterpolationMode=VTK_RESLICE_CUBIC;

  // If the render window has a desired update greater than or equal to the
  // interactive update rate, we apply certain optimizations to ensure that the
  // rendering is interactive.
  this->InteractiveUpdateRate = 1.0;
  // Enable checking whether the render is interactive and use the appropriate
  // sample distance for rendering
  this->InteractiveAdjustSampleDistances = 1;

  // Initial sample distance
  this->AutoAdjustSampleDistances  = 1;
  this->SampleDistance             = -1.0;

  // Create all the mappers we might need
  this->RayCastMapper   = vtkFixedPointVolumeRayCastMapper::New();

  this->GPUMapper       = vtkGPUVolumeRayCastMapper::New();
  this->MaxMemoryInBytes=this->GPUMapper->GetMaxMemoryInBytes();
  this->MaxMemoryFraction=this->GPUMapper->GetMaxMemoryFraction();

  this->GPULowResMapper = vtkGPUVolumeRayCastMapper::New();

  // This is the resample filter that may be used if we need
  // a lower resolution version of the input for GPU rendering
  this->GPUResampleFilter = vtkImageResample::New();

  // Compute the magnitude of a 3-component image for the SingleComponentMode
  this->ImageMagnitude = NULL;
  this->InputDataMagnitude = vtkImageData::New();

  // Turn this on by default - this means that the sample spacing will be
  // automatically computed from the spacing of the input data. This is
  // also true for the GPU ray cast mapper.
  this->RayCastMapper->LockSampleDistanceToInputSpacingOn();
  this->GPUMapper->LockSampleDistanceToInputSpacingOn();

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

  this->OSPRayMapper = NULL;
}

// ----------------------------------------------------------------------------
// Destructor
// ----------------------------------------------------------------------------
vtkSmartVolumeMapper::~vtkSmartVolumeMapper()
{
  if (this->RayCastMapper)
  {
    this->RayCastMapper->Delete();
    this->RayCastMapper = NULL;
  }
  if (this->GPUMapper)
  {
    this->GPUMapper->Delete();
    this->GPUMapper = NULL;
  }
  if (this->GPULowResMapper)
  {
    this->GPULowResMapper->Delete();
    this->GPULowResMapper = NULL;
  }
  if (this->GPUResampleFilter)
  {
    this->GPUResampleFilter->Delete();
    this->GPUResampleFilter = NULL;
  }
  if (this->ImageMagnitude)
  {
    this->ImageMagnitude->Delete();
    this->ImageMagnitude = NULL;
  }
  if (this->InputDataMagnitude)
  {
    this->InputDataMagnitude->Delete();
    this->InputDataMagnitude = NULL;
  }
  if (this->OSPRayMapper)
  {
    this->OSPRayMapper->Delete();
    this->OSPRayMapper = NULL;
  }
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
      if (this->InteractiveAdjustSampleDistances)
      {
        this->RayCastMapper->SetAutoAdjustSampleDistances(
          ren->GetRenderWindow()->GetDesiredUpdateRate()>=
          this->InteractiveUpdateRate);
      }
      else
      {
        this->RayCastMapper->SetAutoAdjustSampleDistances(
                              this->AutoAdjustSampleDistances);
      }
      this->RayCastMapper->Render(ren,vol);
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
      if (this->InteractiveAdjustSampleDistances)
      {
        usedMapper->SetAutoAdjustSampleDistances(
          ren->GetRenderWindow()->GetDesiredUpdateRate()>=
          this->InteractiveUpdateRate);
      }
      else
      {
        usedMapper->SetAutoAdjustSampleDistances(
                      this->AutoAdjustSampleDistances);
      }
      usedMapper->Render(ren, vol);
      break;
    case vtkSmartVolumeMapper::OSPRayRenderMode:
      if (!this->OSPRayMapper)
      {
        this->OSPRayMapper = vtkOSPRayVolumeInterface::New();
      }
      this->OSPRayMapper->Render(ren, vol);
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
// Initialize the render
// We need to determine whether the GPU or CPU mapper are supported
// First we need to know what input scalar field we are working with to find
// out how many components it has. If it has more than one and we are considering
// them to be independent components, then only GPU Mapper will be supported.
// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::Initialize(vtkRenderer *ren, vtkVolume *vol)
{
  vtkImageData *input = this->GetInput();
  if (!input)
  {
    this->Initialized = 0;
    return;
  }

  int usingCellColors = 0;
  vtkDataArray* scalars = this->GetScalars(input, this->ScalarMode,
                                           this->ArrayAccessMode,
                                           this->ArrayId, this->ArrayName,
                                           usingCellColors);

  if (!scalars)
  {
    vtkErrorMacro("Could not find the requested vtkDataArray! " <<
    this->ScalarMode << ", " << this->ArrayAccessMode << ", " <<
    this->ArrayId << ", " << this->ArrayName);
    this->Initialized = 0;
    return;
  }

  int const numComp = scalars->GetNumberOfComponents();
  this->RayCastSupported = (usingCellColors || numComp > 1) ? 0 : 1;

  if (!this->RayCastSupported &&
    this->RequestedRenderMode == vtkSmartVolumeMapper::RayCastRenderMode)
  {
    vtkWarningMacro("Data array "<< this->ArrayName << " is not supported by"
      "FixedPointVolumeRCMapper (either cell data or multiple components).");
  }

  // Make the window current because we need the OpenGL context
  vtkRenderWindow* win = ren->GetRenderWindow();
  win->MakeCurrent();

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

  // Compute the sample distance based on dataset spacing.
  // It is assumed that a negative SampleDistance means the user would like to
  // compute volume mapper sample distance based on data spacing.
  if (this->SampleDistance < 0)
  {
    this->SampleDistance =
      static_cast<float>((spacing[0] + spacing[1] + spacing[2]) / 6.0);
  }

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

    // Requested GPU - OK as long as it is supported
    case vtkSmartVolumeMapper::GPURenderMode:
      if ( this->GPUSupported )
      {
        this->CurrentRenderMode = vtkSmartVolumeMapper::GPURenderMode;
      }
      break;

      // Requested default mode - select GPU if supported, otherwise RayCast
    case vtkSmartVolumeMapper::DefaultRenderMode:
      // Go with GPU rendering if it is supported
      if ( this->GPUSupported )
      {
        this->CurrentRenderMode = vtkSmartVolumeMapper::GPURenderMode;
      }
      else if ( this->RayCastSupported )
      {
        this->CurrentRenderMode = vtkSmartVolumeMapper::RayCastRenderMode;
      }
      break;

    case vtkSmartVolumeMapper::OSPRayRenderMode:
      this->CurrentRenderMode = vtkSmartVolumeMapper::OSPRayRenderMode;
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
      if (this->ArrayAccessMode == VTK_GET_ARRAY_BY_NAME)
      {
        this->RayCastMapper->SelectScalarArray(this->ArrayName);
      }
      else if (this->ArrayAccessMode == VTK_GET_ARRAY_BY_ID)
      {
        this->RayCastMapper->SelectScalarArray(this->ArrayId);
      }
      this->RayCastMapper->SetScalarMode(this->GetScalarMode());
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
      this->RayCastMapper->SetSampleDistance(this->SampleDistance);
      break;

    // We are rendering with the vtkGPUVolumeRayCastMapper
    case vtkSmartVolumeMapper::GPURenderMode:
      if (this->VectorMode == DISABLED)
      {
        // If the internal Magnitude data is not being used, then
        // set the array selection of the original input.
        if (this->ArrayAccessMode == VTK_GET_ARRAY_BY_NAME)
        {
          this->GPUMapper->SelectScalarArray(this->ArrayName);
        }
        else if (this->ArrayAccessMode == VTK_GET_ARRAY_BY_ID)
        {
          this->GPUMapper->SelectScalarArray(this->ArrayId);
        }
        this->GPUMapper->SetScalarMode(this->GetScalarMode());
        this->ConnectMapperInput(this->GPUMapper);
      }
      else
      {
        // Adjust the input or component weights depending on the
        // active mode.
        this->SetupVectorMode(vol);
      }

      this->GPUMapper->SetMaxMemoryInBytes(this->MaxMemoryInBytes);
      this->GPUMapper->SetMaxMemoryFraction(this->MaxMemoryFraction);
      this->GPUMapper->SetClippingPlanes(this->GetClippingPlanes());
      this->GPUMapper->SetCropping(this->GetCropping());
      this->GPUMapper->SetCroppingRegionPlanes(
        this->GetCroppingRegionPlanes());
      this->GPUMapper->SetCroppingRegionFlags(
        this->GetCroppingRegionFlags());
      this->GPUMapper->SetBlendMode( this->GetBlendMode() );
      this->GPUMapper->SetFinalColorWindow(this->FinalColorWindow);
      this->GPUMapper->SetFinalColorLevel(this->FinalColorLevel);
      this->GPUMapper->SetSampleDistance(this->SampleDistance);

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
        this->GPULowResMapper->SetSampleDistance(this->SampleDistance);
      }
      else
      {
        this->LowResGPUNecessary = 0;
      }

      break;

    case vtkSmartVolumeMapper::OSPRayRenderMode:
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
void vtkSmartVolumeMapper::SetupVectorMode(vtkVolume* vol)
{
  vtkImageData* input = this->GetInput();
  if (!input)
  {
    vtkErrorMacro("Failed to setup vector rendering mode! No input.");
  }

  int cellFlag = 0;
  vtkDataArray* dataArray  = this->GetScalars(input, this->ScalarMode,
    this->ArrayAccessMode, this->ArrayId, this->ArrayName, cellFlag);
  int const numComponents = dataArray->GetNumberOfComponents();

  switch (this->VectorMode)
  {
    case MAGNITUDE:
    {
      // ParaView sets mode as MAGNITUDE when there is a single component,
      // so check whether magnitude makes sense.
      if (numComponents > 1)
      {
        // Recompute magnitude if not up to date
        if (!this->ImageMagnitude ||
          input->GetMTime() > this->ImageMagnitude->GetOutput()->GetMTime())
        {
          if (!this->ImageMagnitude)
          {
            this->ImageMagnitude = vtkImageMagnitude::New();
          }

          // Proxy dataset (set the active attribute for the magnitude filter)
          vtkImageData* tempInput = vtkImageData::New();
          tempInput->ShallowCopy(input);
          int const id = tempInput->GetPointData()->SetActiveAttribute(
            dataArray->GetName(), vtkDataSetAttributes::SCALARS);

          if (id < 0)
          {
            vtkErrorMacro("Failed to set the active attribute in magnitude"
              " filter!");
          }

          this->ImageMagnitude->SetInputData(tempInput);
          this->ImageMagnitude->Update();
          this->InputDataMagnitude->ShallowCopy(this->ImageMagnitude->GetOutput());
          tempInput->Delete();
        }

        if (this->InputDataMagnitude->GetMTime() > this->MagnitudeUploadTime)
        {
          this->GPUMapper->SetInputDataObject(this->InputDataMagnitude);
          this->GPUMapper->SelectScalarArray("Magnitude");
          this->MagnitudeUploadTime.Modified();
        }

      }
      else
      {
        // Data is not multi-component so use the array itself.
        if (this->ArrayAccessMode == VTK_GET_ARRAY_BY_NAME)
        {
          this->GPUMapper->SelectScalarArray(this->ArrayName);
        }
        else if (this->ArrayAccessMode == VTK_GET_ARRAY_BY_ID)
        {
          this->GPUMapper->SelectScalarArray(this->ArrayId);
        }
        this->GPUMapper->SetArrayAccessMode(this->ArrayAccessMode);
        this->GPUMapper->SetScalarMode(this->GetScalarMode());
        this->ConnectMapperInput(this->GPUMapper);
      }
    }
    break;

    case COMPONENT:
    {
      if (this->ArrayAccessMode == VTK_GET_ARRAY_BY_NAME)
      {
        this->GPUMapper->SelectScalarArray(this->ArrayName);
      }
      else if (this->ArrayAccessMode == VTK_GET_ARRAY_BY_ID)
      {
        this->GPUMapper->SelectScalarArray(this->ArrayId);
      }
      this->GPUMapper->SetArrayAccessMode(this->ArrayAccessMode);
      this->GPUMapper->SetScalarMode(this->GetScalarMode());
      this->ConnectMapperInput(this->GPUMapper);

      // GPUMapper supports independent components (separate TFs per component).
      // To follow the current ParaView convention, the first TF is set on
      // the currently selected component. TODO: A more robust future
      // integration of independent components in ParaView should set these
      // TF's already per component.
      vtkVolumeProperty* volProp = vol->GetProperty();
      vtkColorTransferFunction* colortf = volProp->GetRGBTransferFunction(0);
      if (!colortf)
      {
        vtkErrorMacro("Internal Error: No RGBTransferFunction has been set!");
        return;
      }
      volProp->SetColor(this->VectorComponent, colortf);

      vtkPiecewiseFunction* opacitytf = volProp->GetScalarOpacity(0);
      if (!opacitytf)
      {
        vtkErrorMacro("Internal Error: No ScalarOpacity has been set!");
        return;
      }
      volProp->SetScalarOpacity(this->VectorComponent, opacitytf);

      for (int i = 0; i < numComponents; i++)
      {
        double const weight = i == this->VectorComponent ? 1.0 : 0.0;
        volProp->SetComponentWeight(i, weight);
      }
    }
    break;

    default:
      vtkErrorMacro("Unknown vector rendering mode!");
      break;
  }
}

// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::ConnectMapperInput(vtkVolumeMapper *m)
{
  assert("pre: m_exists" && m != NULL);

  bool needShallowCopy = false;
  vtkImageData* imData = m->GetInput();

  if (imData == NULL || imData == this->InputDataMagnitude)
  {
    imData = vtkImageData::New();
    m->SetInputDataObject(imData);
    needShallowCopy = true;
    imData->Delete();
  }
  else
  {
    needShallowCopy =
      imData->GetMTime() < this->GetInput()->GetMTime();

    m->SetInputDataObject(imData);
  }

  if (needShallowCopy)
  {
    imData->ShallowCopy(this->GetInput());
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
    f->SetInputDataObject(input2);
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

#if !defined(VTK_LEGACY_REMOVE)
  if (mode == RayCastAndTextureRenderMode || mode == TextureRenderMode)
  {
    vtkErrorMacro("RayCastAndTextureRenderMode and \
                  TextureRenderMode no longer supported");
    return;
  }
#endif // VTK_LEGACY_REMOVE

  // Make sure it is a valid mode
  if ( mode < vtkSmartVolumeMapper::DefaultRenderMode ||
       mode >= vtkSmartVolumeMapper::UndefinedRenderMode)
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
void vtkSmartVolumeMapper::SetRequestedRenderModeToRayCast()
{
  this->SetRequestedRenderMode(vtkSmartVolumeMapper::RayCastRenderMode);
}

// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::SetRequestedRenderModeToGPU()
{
  this->SetRequestedRenderMode(vtkSmartVolumeMapper::GPURenderMode);
}

// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::SetRequestedRenderModeToOSPRay()
{
  this->SetRequestedRenderMode(vtkSmartVolumeMapper::OSPRayRenderMode);
}


// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::ReleaseGraphicsResources(vtkWindow *w)
{
  this->RayCastMapper->ReleaseGraphicsResources(w);
  this->GPUMapper->ReleaseGraphicsResources(w);
  this->GPULowResMapper->ReleaseGraphicsResources(w);

  this->Initialized      = 0;
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
  os << "InteractiveAdjustSampleDistances: " <<
    this->InteractiveAdjustSampleDistances << endl;
  os << "InterpolationMode: " << this->InterpolationMode << endl;
  os << "MaxMemoryInBytes:" << this->MaxMemoryInBytes << endl;
  os << "MaxMemoryFraction:" << this->MaxMemoryFraction << endl;
  os << "AutoAdjustSampleDistances: "
     << this->AutoAdjustSampleDistances << endl;
  os << indent << "SampleDistance: " << this->SampleDistance << endl;
}

// ----------------------------------------------------------------------------
void vtkSmartVolumeMapper::SetVectorMode(int mode)
{
  int const clampedMode = mode < -1 ? -1 : (mode > 1 ? 1 : mode);
  if (clampedMode != this->VectorMode)
  {
    if (clampedMode == MAGNITUDE)
    {
      this->InputDataMagnitude->Modified();
    }

    this->VectorMode = clampedMode;
    this->Modified();
  }
}
