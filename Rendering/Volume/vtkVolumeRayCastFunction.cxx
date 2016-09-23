/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolumeRayCastFunction.h"

#if !defined(VTK_LEGACY_REMOVE)

#include "vtkEncodedGradientEstimator.h"
#include "vtkEncodedGradientShader.h"
#include "vtkImageData.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeRayCastMapper.h"


// Grab everything we need for rendering now. This procedure will be called
// during the initialization phase of ray casting. It is called once per
// image. All Gets are done here for both performance and multithreading
// reentrant requirements reasons. At the end, the SpecificFunctionInitialize
// is called to give the subclass a chance to do its thing.
void vtkVolumeRayCastFunction::FunctionInitialize(
                                vtkRenderer *ren,
                                vtkVolume *vol,
                                vtkVolumeRayCastStaticInfo *staticInfo )
{
  vtkVolumeRayCastMapper *mapper =
    vtkVolumeRayCastMapper::SafeDownCast( vol->GetMapper() );

  if ( !mapper )
  {
    vtkErrorMacro(
    "Function initialized called with a volume that does not use ray casting");
    return;
  }

  // Is shading on?
  staticInfo->Shading = vol->GetProperty()->GetShade();

  // How many color channels? Either 1 or 3. 1 means we have
  // to use the GrayTransferFunction, 3 means we use the
  // RGBTransferFunction
  staticInfo->ColorChannels = vol->GetProperty()->GetColorChannels();

  // What is the interpolation type? Nearest or linear.
  staticInfo->InterpolationType = vol->GetProperty()->GetInterpolationType();

  // Get the size, spacing and origin of the scalar data
  mapper->GetInput()->GetDimensions( staticInfo->DataSize );
  mapper->GetInput()->GetSpacing( staticInfo->DataSpacing );
  mapper->GetInput()->GetOrigin( staticInfo->DataOrigin );

  // What are the data increments?
  // (One voxel, one row, and one slice offsets)
  staticInfo->DataIncrement[0] = 1;
  staticInfo->DataIncrement[1] = static_cast<vtkIdType>(staticInfo->DataSize[0]);
  staticInfo->DataIncrement[2] = static_cast<vtkIdType>(staticInfo->DataSize[0])*
                                 static_cast<vtkIdType>(staticInfo->DataSize[1]);


  // Get the encoded normals from the normal encoder in the
  // volume ray cast mapper. We need to do this if shading is on
  // or if we are classifying scalar value into opacity based
  // on the magnitude of the gradient (since if we need to
  // calculate the magnitude we might as well just keep the
  // direction as well.
  if ( staticInfo->Shading )
  {
    staticInfo->EncodedNormals =
      mapper->GetGradientEstimator()->GetEncodedNormals();

    // Get the diffuse shading tables from the normal encoder
    // in the volume ray cast mapper
    staticInfo->RedDiffuseShadingTable =
      mapper->GetGradientShader()->GetRedDiffuseShadingTable(vol);
    staticInfo->GreenDiffuseShadingTable =
      mapper->GetGradientShader()->GetGreenDiffuseShadingTable(vol);
    staticInfo->BlueDiffuseShadingTable =
      mapper->GetGradientShader()->GetBlueDiffuseShadingTable(vol);

    // Get the specular shading tables from the normal encoder
    // in the volume ray cast mapper
    staticInfo->RedSpecularShadingTable =
      mapper->GetGradientShader()->GetRedSpecularShadingTable(vol);
    staticInfo->GreenSpecularShadingTable =
      mapper->GetGradientShader()->GetGreenSpecularShadingTable(vol);
    staticInfo->BlueSpecularShadingTable =
      mapper->GetGradientShader()->GetBlueSpecularShadingTable(vol);
  }
  else
  {
    staticInfo->EncodedNormals            = NULL;
    staticInfo->RedDiffuseShadingTable    = NULL;
    staticInfo->GreenDiffuseShadingTable  = NULL;
    staticInfo->BlueDiffuseShadingTable   = NULL;
    staticInfo->RedSpecularShadingTable   = NULL;
    staticInfo->GreenSpecularShadingTable = NULL;
    staticInfo->BlueSpecularShadingTable  = NULL;
  }

  // We need the gradient magnitudes only if we are classifying opacity
  // based on them. Otherwise we can just leave them NULL
  if ( vol->GetGradientOpacityArray() &&
       vol->GetGradientOpacityConstant() == -1.0 )
  {
    staticInfo->GradientMagnitudes =
      mapper->GetGradientEstimator()->GetGradientMagnitudes();
  }
  else
  {
    staticInfo->GradientMagnitudes = NULL;
  }

  // By default the blending is not MIP - the MIP function will turn this
  // on
  staticInfo->MIPFunction = 0;

  // Give the subclass a chance to do any initialization it needs
  // to do
  this->SpecificFunctionInitialize( ren, vol, staticInfo, mapper );
}

//----------------------------------------------------------------------------
void vtkVolumeRayCastFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

#endif // VTK_LEGACY_REMOVE
