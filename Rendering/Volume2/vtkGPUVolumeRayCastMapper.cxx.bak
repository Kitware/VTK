/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProjectedTetrahedraMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGPUVolumeRayCastMapper.h"

/// Include compiled shader code
#include <raycasterfs.h>
#include <raycastervs.h>

/// VTK includes
#include <vtkBoundingBox.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkColorTransferFunction.h>
#include <vtkCommand.h>
#include <vtkDensifyPolyData.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPerlinNoise.h>
#include <vtkPlaneCollection.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkTessellatedBoxSource.h>
#include <vtkTimerLog.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedIntArray.h>
#include <vtkVolumeProperty.h>

/// GL includes
#include <GL/glew.h>
#include <vtkgl.h>

/// C/C++ includes
#include <cassert>
#include <sstream>

vtkAbstractObjectFactoryNewMacro(vtkGPUVolumeRayCastMapper);

///----------------------------------------------------------------------------
vtkGPUVolumeRayCastMapper::vtkGPUVolumeRayCastMapper() : vtkVolumeMapper()
{
  this->CellFlag = 0;
  this->AutoAdjustSampleDistancesOff();
  this->SampleDistance = 1.0;
  this->Timer = vtkTimerLog::New();
}

///----------------------------------------------------------------------------
vtkGPUVolumeRayCastMapper::~vtkGPUVolumeRayCastMapper()
{
  if (this->Timer)
    {
    this->Timer->Delete();
    this->Timer = NULL;
    }
}

///----------------------------------------------------------------------------
void vtkGPUVolumeRayCastMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AutoAdjustSampleDistances: "
     << this->AutoAdjustSampleDistances << endl;
  os << indent << "SampleDistance: " << this->SampleDistance << endl;
}

///----------------------------------------------------------------------------
int vtkGPUVolumeRayCastMapper::ValidateRender(vtkRenderer* ren, vtkVolume* vol)
{
  /// Check that we have everything we need to render.
  int goodSoFar = 1;

  /// Check for a renderer - we MUST have one
  if (!ren)
    {
    goodSoFar = 0;
    vtkErrorMacro("Renderer cannot be null.");
    }

  /// Check for the m_volume - we MUST have one
  if (goodSoFar && !vol)
    {
    goodSoFar = 0;
    vtkErrorMacro("Volume cannot be null.");
    }

  /// Don't need to check if we have a m_volume property
  /// since the m_volume will create one if we don't. Also
  /// don't need to check for the scalar opacity function
  /// or the RGB transfer function since the property will
  /// create them if they do not yet exist.

  /// TODO: Enable cropping planes
  /// \see vtkGPUVolumeRayCastMapper

  /// Check that we have input data
  vtkImageData* input = this->GetInput();
  if (goodSoFar && input == 0)
    {
    vtkErrorMacro("Input is NULL but is required");
    goodSoFar = 0;
    }

  if (goodSoFar)
    {
    this->GetInputAlgorithm()->Update();
    }

  /// TODO:
  /// Check if we need to do workaround to handle extents starting from non-zero
  /// values.
  /// \see vtkGPUVolumeRayCastMapper

  /// Update the date then make sure we have scalars. Note
  /// that we must have point or cell scalars because field
  /// scalars are not supported.
  vtkDataArray* scalars = NULL;
  if (goodSoFar)
    {
    /// Now make sure we can find scalars
    scalars=this->GetScalars(input,this->ScalarMode,
                             this->ArrayAccessMode,
                             this->ArrayId,
                             this->ArrayName,
                             this->CellFlag);

    /// We couldn't find scalars
    if (!scalars)
      {
      vtkErrorMacro("No scalars found on input.");
      goodSoFar = 0;
      }
    /// Even if we found scalars, if they are field data scalars that isn't good
    else if (this->CellFlag == 2)
      {
      vtkErrorMacro("Only point or cell scalar support - found field scalars instead.");
      goodSoFar = 0;
      }
    }

  /// Make sure the scalar type is actually supported. This mappers supports
  /// almost all standard scalar types.
  if (goodSoFar)
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
        /// Don't need to do anything here
        break;
      }
    }

  /// Check on the blending type - we support composite and min / max intensity
  if (goodSoFar)
    {
    if (this->BlendMode != vtkVolumeMapper::COMPOSITE_BLEND &&
        this->BlendMode != vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND &&
        this->BlendMode != vtkVolumeMapper::MINIMUM_INTENSITY_BLEND &&
        this->BlendMode != vtkVolumeMapper::ADDITIVE_BLEND)
      {
      goodSoFar = 0;
      vtkErrorMacro(<< "Selected blend mode not supported. "
                    << "Only Composite, MIP, MinIP and additive modes "
                    << "are supported by the current implementation.");
      }
    }

  /// This mapper supports 1 component data, or 4 component if it is not independent
  /// component (i.e. the four components define RGBA)
  int numberOfComponents = 0;
  if (goodSoFar)
    {
    numberOfComponents=scalars->GetNumberOfComponents();
    if (!(numberOfComponents==1 ||
          (numberOfComponents==4 &&
           vol->GetProperty()->GetIndependentComponents()==0)))
      {
      goodSoFar = 0;
      vtkErrorMacro(<< "Only one component scalars, or four "
                    << "component with non-independent components, "
                    << "are supported by this mapper.");
      }
    }

  /// If this is four component data, then it better be unsigned char (RGBA).
  /// TODO: Check on this condition
  if (goodSoFar &&
      numberOfComponents == 4 &&
      scalars->GetDataType() != VTK_UNSIGNED_CHAR)
    {
    goodSoFar = 0;
    vtkErrorMacro("Only unsigned char is supported for 4-component scalars!");
    }

  if (goodSoFar && numberOfComponents!=1 &&
     this->BlendMode==vtkVolumeMapper::ADDITIVE_BLEND)
    {
    goodSoFar=0;
    vtkErrorMacro("Additive mode only works with 1-component scalars!");
    }

  /// return our status
  return goodSoFar;
}

///----------------------------------------------------------------------------
void vtkGPUVolumeRayCastMapper::Render(vtkRenderer* ren, vtkVolume* vol)
{
  /// Invoke a VolumeMapperRenderStartEvent
  this->InvokeEvent(vtkCommand::VolumeMapperRenderStartEvent,0);

  /// Start the timer to time the length of this render
  this->Timer->StartTimer();

  /// Make sure everything about this render is OK.
  /// This is where the input is updated.
  if (this->ValidateRender(ren, vol ))
    {
    /// Everything is OK - so go ahead and really do the render
    this->GPURender(ren, vol);
    }

  /// Stop the timer
  this->Timer->StopTimer();
  this->ElapsedDrawTime = this->Timer->GetElapsedTime();

  // Invoke a VolumeMapperRenderEndEvent
  this->InvokeEvent(vtkCommand::VolumeMapperRenderEndEvent,0);
}
