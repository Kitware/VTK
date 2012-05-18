/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGPUVolumeRayCastMapper.h"

#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkTimerLog.h"
#include "vtkImageResample.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include <assert.h>
#include "vtkCommand.h" // for VolumeMapperRender{Start|End|Progress}Event
#include "vtkCamera.h"
#include "vtkRendererCollection.h"
#include "vtkMultiThreader.h"
#include "vtkGPUInfoList.h"
#include "vtkGPUInfo.h"

// Return NULL if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkGPUVolumeRayCastMapper)
vtkCxxSetObjectMacro(vtkGPUVolumeRayCastMapper, MaskInput, vtkImageData);
vtkCxxSetObjectMacro(vtkGPUVolumeRayCastMapper, TransformedInput, vtkImageData);

vtkGPUVolumeRayCastMapper::vtkGPUVolumeRayCastMapper()
{
  this->AutoAdjustSampleDistances  = 1;
  this->ImageSampleDistance        = 1.0;
  this->MinimumImageSampleDistance = 1.0;
  this->MaximumImageSampleDistance = 10.0;
  this->SampleDistance             = 1.0;
  this->SmallVolumeRender          = 0;
  this->BigTimeToDraw              = 0.0;
  this->SmallTimeToDraw            = 0.0;
  this->FinalColorWindow           = 1.0;
  this->FinalColorLevel            = 0.5;
  this->GeneratingCanonicalView    = 0;
  this->CanonicalViewImageData     = NULL;

  this->MaskInput                  = NULL;
  this->MaskBlendFactor            = 1.0f;
  this->MaskType
    = vtkGPUVolumeRayCastMapper::LabelMapMaskType;


  this->AMRMode=0;
  this->ClippedCroppingRegionPlanes[0]=VTK_DOUBLE_MAX;
  this->ClippedCroppingRegionPlanes[1]=VTK_DOUBLE_MIN;
  this->ClippedCroppingRegionPlanes[2]=VTK_DOUBLE_MAX;
  this->ClippedCroppingRegionPlanes[3]=VTK_DOUBLE_MIN;
  this->ClippedCroppingRegionPlanes[4]=VTK_DOUBLE_MAX;
  this->ClippedCroppingRegionPlanes[5]=VTK_DOUBLE_MIN;

  this->MaxMemoryInBytes=0;
  vtkGPUInfoList *l=vtkGPUInfoList::New();
  l->Probe();
  if(l->GetNumberOfGPUs()>0)
    {
    vtkGPUInfo *info=l->GetGPUInfo(0);
    this->MaxMemoryInBytes=info->GetDedicatedVideoMemory();
    if(this->MaxMemoryInBytes==0)
      {
      this->MaxMemoryInBytes=info->GetDedicatedSystemMemory();
      }
    // we ignore info->GetSharedSystemMemory(); as this is very slow.
    }
  l->Delete();

  if(this->MaxMemoryInBytes==0) // use some default value: 128MB.
    {
    this->MaxMemoryInBytes=128*1024*1024;
    }

  this->MaxMemoryFraction = 0.75;

  this->ReportProgress=true;

  this->TransformedInput = NULL;
  this->LastInput = NULL;
}

// ----------------------------------------------------------------------------
vtkGPUVolumeRayCastMapper::~vtkGPUVolumeRayCastMapper()
{
  this->SetMaskInput(NULL);
  this->SetTransformedInput(NULL);
  this->LastInput = NULL;
}

// ----------------------------------------------------------------------------
// The render method that is called from the volume. If this is a canonical
// view render, a specialized version of this method will be called instead.
// Otherwise we will
//   - Invoke a start event
//   - Start timing
//   - Check that everything is OK for rendering
//   - Render
//   - Stop the timer and record results
//   - Invoke an end event
// ----------------------------------------------------------------------------
void vtkGPUVolumeRayCastMapper::Render( vtkRenderer *ren, vtkVolume *vol )
{
  // Catch renders that are happening due to a canonical view render and
  // handle them separately.
  if (this->GeneratingCanonicalView )
    {
    this->CanonicalViewRender(ren, vol);
    return;
    }

  // Invoke a VolumeMapperRenderStartEvent
  this->InvokeEvent(vtkCommand::VolumeMapperRenderStartEvent,0);

  // Start the timer to time the length of this render
  vtkTimerLog *timer = vtkTimerLog::New();
  timer->StartTimer();

  // Make sure everything about this render is OK.
  // This is where the input is updated.
  if ( this->ValidateRender(ren, vol ) )
    {
    // Everything is OK - so go ahead and really do the render
    this->GPURender( ren, vol);
    }

  // Stop the timer
  timer->StopTimer();
  double t = timer->GetElapsedTime();

//  cout << "Render Timer " << t << " seconds, " << 1.0/t << " frames per second" << endl;

  this->TimeToDraw = t;
  timer->Delete();

  if ( vol->GetAllocatedRenderTime() < 1.0 )
    {
    this->SmallTimeToDraw = t;
    }
  else
    {
    this->BigTimeToDraw = t;
    }

  // Invoke a VolumeMapperRenderEndEvent
  this->InvokeEvent(vtkCommand::VolumeMapperRenderEndEvent,0);
}

// ----------------------------------------------------------------------------
// Special version for rendering a canonical view - we don't do things like
// invoke start or end events, and we don't capture the render time.
// ----------------------------------------------------------------------------
void vtkGPUVolumeRayCastMapper::CanonicalViewRender(vtkRenderer *ren,
                                                    vtkVolume *vol )
{
  // Make sure everything about this render is OK
  if ( this->ValidateRender(ren, vol ) )
    {
    // Everything is OK - so go ahead and really do the render
    this->GPURender( ren, vol);
    }
}

// ----------------------------------------------------------------------------
// This method us used by the render method to validate everything before
// attempting to render. This method returns 0 if something is not right -
// such as missing input, a null renderer or a null volume, no scalars, etc.
// In some cases it will produce a vtkErrorMacro message, and in others
// (for example, in the case of cropping planes that define a region with
// a volume or 0 or less) it will fail silently. If everything is OK, it will
// return with a value of 1.
// ----------------------------------------------------------------------------
int vtkGPUVolumeRayCastMapper::ValidateRender(vtkRenderer *ren,
                                              vtkVolume *vol)
{
  // Check that we have everything we need to render.
  int goodSoFar = 1;

  // Check for a renderer - we MUST have one
  if ( !ren )
    {
    goodSoFar = 0;
    vtkErrorMacro("Renderer cannot be null.");
    }

  // Check for the volume - we MUST have one
  if ( goodSoFar && !vol )
    {
    goodSoFar = 0;
    vtkErrorMacro("Volume cannot be null.");
    }

  // Don't need to check if we have a volume property
  // since the volume will create one if we don't. Also
  // don't need to check for the scalar opacity function
  // or the RGB transfer function since the property will
  // create them if they do not yet exist.

  // However we must currently check that the number of
  // color channels is 3
  // TODO: lift this restriction - should work with
  // gray functions as well. Right now turning off test
  // because otherwise 4 component rendering isn't working.
  // Will revisit.
  if ( goodSoFar && vol->GetProperty()->GetColorChannels() != 3 )
    {
//    goodSoFar = 0;
//    vtkErrorMacro("Must have a color transfer function.");
    }

  // Check the cropping planes. If they are invalid, just silently
  // fail. This will happen when an interactive widget is dragged
  // such that it defines 0 or negative volume - this can happen
  // and should just not render the volume.
  // Check the cropping planes
  if( goodSoFar && this->Cropping &&
     (this->CroppingRegionPlanes[0]>=this->CroppingRegionPlanes[1] ||
      this->CroppingRegionPlanes[2]>=this->CroppingRegionPlanes[3] ||
      this->CroppingRegionPlanes[4]>=this->CroppingRegionPlanes[5] ))
    {
    // No error message here - we want to be silent
    goodSoFar = 0;
    }

  // Check that we have input data
  vtkImageData *input=this->GetInput();

  if(goodSoFar && input==0)
    {
    vtkErrorMacro("Input is NULL but is required");
    goodSoFar = 0;
    }

  if(goodSoFar)
    {
    this->GetInputAlgorithm()->Update();
    }

  // If we have a timestamp change or data change then create a new clone.
  if(goodSoFar && (input != this->LastInput ||
                   input->GetMTime() > this->TransformedInput->GetMTime()))
    {
    this->LastInput = input;

    vtkImageData* clone;
    if(!this->TransformedInput)
      {
      clone = vtkImageData::New();
      this->SetTransformedInput(clone);
      clone->Delete();
      }
    else
      {
      clone = this->TransformedInput;
      }

    clone->ShallowCopy(input);

    // @TODO: This is the workaround to deal with GPUVolumeRayCastMapper
    // not able to handle extents starting from non zero values.
    // There is not a easy fix in the GPU volume ray cast mapper hence
    // this fix has been introduced.

    // Get the current extents.
    int extents[6], real_extents[6];
    clone->GetExtent(extents);
    clone->GetExtent(real_extents);

    // Get the current origin and spacing.
    double origin[3], spacing[3];
    clone->GetOrigin(origin);
    clone->GetSpacing(spacing);

    for (int cc=0; cc < 3; cc++)
      {
      // Transform the origin and the extents.
      origin[cc] = origin[cc] + extents[2*cc]*spacing[cc];
      extents[2*cc+1] -= extents[2*cc];
      extents[2*cc] -= extents[2*cc];
      }

    clone->SetOrigin(origin);
    clone->SetExtent(extents);
    }

  // Update the date then make sure we have scalars. Note
  // that we must have point or cell scalars because field
  // scalars are not supported.
  vtkDataArray *scalars = NULL;
  if ( goodSoFar )
    {
    // Now make sure we can find scalars
    scalars=this->GetScalars(this->TransformedInput,this->ScalarMode,
                             this->ArrayAccessMode,
                             this->ArrayId,
                             this->ArrayName,
                             this->CellFlag);

    // We couldn't find scalars
    if ( !scalars )
      {
      vtkErrorMacro("No scalars found on input.");
      goodSoFar = 0;
      }
    // Even if we found scalars, if they are field data scalars that isn't good
    else if ( this->CellFlag == 2 )
      {
      vtkErrorMacro("Only point or cell scalar support - found field scalars instead.");
      goodSoFar = 0;
      }
    }

  // Make sure the scalar type is actually supported. This mappers supports
  // almost all standard scalar types.
  if ( goodSoFar )
    {
    switch(scalars->GetDataType())
      {
      case VTK_CHAR:
        vtkErrorMacro(<< "scalar of type VTK_CHAR is not supported "
                      << "because this type is platform dependent. "
                      << "Use VTK_SIGNED_CHAR or VTK_UNSIGNED_CHAR instead.");
        goodSoFar = 0;
        break;
      case VTK_BIT:
        vtkErrorMacro("scalar of type VTK_BIT is not supported by this mapper.");
        goodSoFar = 0;
        break;
      case VTK_ID_TYPE:
        vtkErrorMacro("scalar of type VTK_ID_TYPE is not supported by this mapper.");
        goodSoFar = 0;
        break;
      case VTK_STRING:
        vtkErrorMacro("scalar of type VTK_STRING is not supported by this mapper.");
        goodSoFar = 0;
        break;
      default:
        // Don't need to do anything here
        break;
      }
    }

  // Check on the blending type - we support composite and min / max intensity
  if ( goodSoFar )
    {
    if(this->BlendMode!=vtkVolumeMapper::COMPOSITE_BLEND &&
       this->BlendMode!=vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND &&
       this->BlendMode!=vtkVolumeMapper::MINIMUM_INTENSITY_BLEND &&
       this->BlendMode!=vtkVolumeMapper::ADDITIVE_BLEND)
      {
      goodSoFar = 0;
      vtkErrorMacro(<< "Selected blend mode not supported. "
                    << "Only Composite, MIP, MinIP and additive modes "
                    << "are supported by the current implementation.");
      }
    }

  // This mapper supports 1 component data, or 4 component if it is not independent
  // component (i.e. the four components define RGBA)
  int numberOfComponents = 0;
  if ( goodSoFar )
    {
    numberOfComponents=scalars->GetNumberOfComponents();
    if( !( numberOfComponents==1 ||
           (numberOfComponents==4 &&
            vol->GetProperty()->GetIndependentComponents()==0)))
      {
      goodSoFar = 0;
      vtkErrorMacro(<< "Only one component scalars, or four "
                    << "component with non-independent components, "
                    << "are supported by this mapper.");
      }
    }

  // If this is four component data, then it better be unsigned char (RGBA).
  if( goodSoFar &&
      numberOfComponents == 4 &&
      scalars->GetDataType() != VTK_UNSIGNED_CHAR)
    {
    goodSoFar = 0;
    vtkErrorMacro("Only unsigned char is supported for 4-component scalars!");
    }

  if(goodSoFar && numberOfComponents!=1 &&
     this->BlendMode==vtkVolumeMapper::ADDITIVE_BLEND)
    {
    goodSoFar=0;
    vtkErrorMacro("Additive mode only works with 1-component scalars!");
    }

  // return our status
  return goodSoFar;
}


// ----------------------------------------------------------------------------
// Description:
// Called by the AMR Volume Mapper.
// Set the flag that tells if the scalars are on point data (0) or
// cell data (1).
void vtkGPUVolumeRayCastMapper::SetCellFlag(int cellFlag)
{
  this->CellFlag=cellFlag;
}

// ----------------------------------------------------------------------------
void vtkGPUVolumeRayCastMapper::CreateCanonicalView(
  vtkRenderer *ren,
  vtkVolume *volume,
  vtkImageData *image,
  int vtkNotUsed(blend_mode),
  double viewDirection[3],
  double viewUp[3])
{
  this->GeneratingCanonicalView = 1;
  int oldSwap = ren->GetRenderWindow()->GetSwapBuffers();
  ren->GetRenderWindow()->SwapBuffersOff();


  int dim[3];
  image->GetDimensions(dim);
  int *size = ren->GetRenderWindow()->GetSize();

  vtkImageData *bigImage = vtkImageData::New();
  bigImage->SetDimensions(size[0], size[1], 1);
  bigImage->AllocateScalars(VTK_UNSIGNED_CHAR, 3);

  this->CanonicalViewImageData = bigImage;


  double scale[2];
  scale[0] = dim[0] / static_cast<double>(size[0]);
  scale[1] = dim[1] / static_cast<double>(size[1]);


  // Save the visibility flags of the renderers and set all to false except
  // for the ren.
  vtkRendererCollection *renderers=ren->GetRenderWindow()->GetRenderers();
  int numberOfRenderers=renderers->GetNumberOfItems();

  bool *rendererVisibilities=new bool[numberOfRenderers];
  renderers->InitTraversal();
  int i=0;
  while(i<numberOfRenderers)
    {
    vtkRenderer *r=renderers->GetNextItem();
    rendererVisibilities[i]=r->GetDraw()==1;
    if(r!=ren)
      {
      r->SetDraw(false);
      }
    ++i;
    }

  // Save the visibility flags of the props and set all to false except
  // for the volume.

  vtkPropCollection *props=ren->GetViewProps();
  int numberOfProps=props->GetNumberOfItems();

  bool *propVisibilities=new bool[numberOfProps];
  props->InitTraversal();
  i=0;
  while(i<numberOfProps)
    {
    vtkProp *p=props->GetNextProp();
    propVisibilities[i]=p->GetVisibility()==1;
    if(p!=volume)
      {
      p->SetVisibility(false);
      }
    ++i;
    }

  vtkCamera *savedCamera=ren->GetActiveCamera();
  savedCamera->Modified();
  vtkCamera *canonicalViewCamera=vtkCamera::New();

  // Code from vtkFixedPointVolumeRayCastMapper:
  double *center=volume->GetCenter();
  double bounds[6];
  volume->GetBounds(bounds);
  double d=sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

  // For now use x distance - need to change this
  d=bounds[1]-bounds[0];

  // Set up the camera in parallel
  canonicalViewCamera->SetFocalPoint(center);
  canonicalViewCamera->ParallelProjectionOn();
  canonicalViewCamera->SetPosition(center[0] - d*viewDirection[0],
                                   center[1] - d*viewDirection[1],
                                   center[2] - d*viewDirection[2]);
  canonicalViewCamera->SetViewUp(viewUp);
  canonicalViewCamera->SetParallelScale(d/2);

  ren->SetActiveCamera(canonicalViewCamera);
  ren->GetRenderWindow()->Render();


  ren->SetActiveCamera(savedCamera);
  canonicalViewCamera->Delete();


  // Shrink to image to the desired size
  vtkImageResample *resample = vtkImageResample::New();
  resample->SetInputData( bigImage );
  resample->SetAxisMagnificationFactor(0,scale[0]);
  resample->SetAxisMagnificationFactor(1,scale[1]);
  resample->SetAxisMagnificationFactor(2,1);
  resample->UpdateWholeExtent();

  // Copy the pixels over
  image->DeepCopy(resample->GetOutput());

  bigImage->Delete();
  resample->Delete();

  // Restore the visibility flags of the props
  props->InitTraversal();
  i=0;
  while(i<numberOfProps)
    {
    vtkProp *p=props->GetNextProp();
    p->SetVisibility(propVisibilities[i]);
    ++i;
    }

  delete[] propVisibilities;

  // Restore the visibility flags of the renderers
  renderers->InitTraversal();
  i=0;
  while(i<numberOfRenderers)
    {
    vtkRenderer *r=renderers->GetNextItem();
    r->SetDraw(rendererVisibilities[i]);
    ++i;
    }

  delete[] rendererVisibilities;

  ren->GetRenderWindow()->SetSwapBuffers(oldSwap);
  this->CanonicalViewImageData = NULL;
  this->GeneratingCanonicalView = 0;
}

// ----------------------------------------------------------------------------
// Print method for vtkGPUVolumeRayCastMapper
void vtkGPUVolumeRayCastMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AutoAdjustSampleDistances: "
     << this->AutoAdjustSampleDistances << endl;
  os << indent << "MinimumImageSampleDistance: "
     << this->MinimumImageSampleDistance << endl;
  os << indent << "MaximumImageSampleDistance: "
     << this->MaximumImageSampleDistance << endl;
  os << indent << "ImageSampleDistance: " << this->ImageSampleDistance << endl;
  os << indent << "SampleDistance: " << this->SampleDistance << endl;
  os << indent << "FinalColorWindow: " << this->FinalColorWindow << endl;
  os << indent << "FinalColorLevel: " << this->FinalColorLevel << endl;
  os << indent << "MaskInput: " << this->MaskInput << endl;
  os << indent << "MaskType: " << this->MaskType << endl;
  os << indent << "MaskBlendFactor: " << this->MaskBlendFactor << endl;
  os << indent << "MaxMemoryInBytes: " << this->MaxMemoryInBytes << endl;
  os << indent << "MaxMemoryFraction: " << this->MaxMemoryFraction << endl;
  os << indent << "ReportProgress: " << this->ReportProgress << endl;
}

// ----------------------------------------------------------------------------
// Description:
// Compute the cropping planes clipped by the bounds of the volume.
// The result is put into this->ClippedCroppingRegionPlanes.
// NOTE: IT WILL BE MOVED UP TO vtkVolumeMapper after bullet proof usage
// in this mapper. Other subclasses will use the ClippedCroppingRegionsPlanes
// members instead of CroppingRegionPlanes.
// \pre volume_exists: this->GetInput()!=0
// \pre valid_cropping: this->Cropping &&
//             this->CroppingRegionPlanes[0]<this->CroppingRegionPlanes[1] &&
//             this->CroppingRegionPlanes[2]<this->CroppingRegionPlanes[3] &&
//             this->CroppingRegionPlanes[4]<this->CroppingRegionPlanes[5])
void vtkGPUVolumeRayCastMapper::ClipCroppingRegionPlanes()
{
  assert("pre: volume_exists" && this->GetInput()!=0);
  assert("pre: valid_cropping" && this->Cropping &&
         this->CroppingRegionPlanes[0]<this->CroppingRegionPlanes[1] &&
         this->CroppingRegionPlanes[2]<this->CroppingRegionPlanes[3] &&
         this->CroppingRegionPlanes[4]<this->CroppingRegionPlanes[5]);

  // vtkVolumeMapper::Render() will have something like:
//  if(this->Cropping && (this->CroppingRegionPlanes[0]>=this->CroppingRegionPlanes[1] ||
//                        this->CroppingRegionPlanes[2]>=this->CroppingRegionPlanes[3] ||
//                        this->CroppingRegionPlanes[4]>=this->CroppingRegionPlanes[5]))
//    {
//    // silentely  stop because the cropping is not valid.
//    return;
//    }

  double volBounds[6];
  this->GetInput()->GetBounds(volBounds);

  int i=0;
  while(i<6)
    {
    // max of the mins
    if(this->CroppingRegionPlanes[i]<volBounds[i])
      {
      this->ClippedCroppingRegionPlanes[i]=volBounds[i];
      }
    else
      {
      this->ClippedCroppingRegionPlanes[i]=this->CroppingRegionPlanes[i];
      }
    ++i;
    // min of the maxs
    if(this->CroppingRegionPlanes[i]>volBounds[i])
      {
      this->ClippedCroppingRegionPlanes[i]=volBounds[i];
      }
    else
      {
      this->ClippedCroppingRegionPlanes[i]=this->CroppingRegionPlanes[i];
      }
    ++i;
    }
}

//----------------------------------------------------------------------------
void vtkGPUVolumeRayCastMapper::SetMaskTypeToBinary()
{
  this->MaskType = vtkGPUVolumeRayCastMapper::BinaryMaskType;
}

//----------------------------------------------------------------------------
void vtkGPUVolumeRayCastMapper::SetMaskTypeToLabelMap()
{
  this->MaskType = vtkGPUVolumeRayCastMapper::LabelMapMaskType;
}
