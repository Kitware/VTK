/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeProVP1000Mapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolumeProVP1000Mapper.h"

#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkDebugLeaks.h"
#include "vtkGraphicsFactory.h"
#include "vtkImageData.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLVolumeProVP1000Mapper.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"
#include "vtkToolkits.h"
#include "vtkTransform.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include <stdio.h>
#include <math.h>


//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkVolumeProVP1000Mapper);
//----------------------------------------------------------------------------

vtkVolumeProVP1000Mapper::vtkVolumeProVP1000Mapper()
{
  VLIStatus         status;
  VLIConfiguration  *config;

  this->ImageBuffer = NULL;
  this->DepthBuffer = NULL;

  // Establish a connection with vli
  status = VLIOpen();
  
  if ( status != kVLIOK )
    {
    vtkDebugMacro( << "VLIOpen failed!" );
    this->Context = NULL;
    this->LookupTable = NULL;

    if ( status == kVLIErrNoHardware )
      {
      this->NoHardware = 1;
      }
    else if ( status == kVLIErrVersion )
      {
      this->WrongVLIVersion = 1;
      }
    return;
    }

  // Gather some useful information
  config = new VLIConfiguration;
  this->NumberOfBoards = config->GetNumberOfBoards();
  this->MajorBoardVersion = config->GetBoardMajorVersion();
  this->MinorBoardVersion = config->GetBoardMinorVersion();
  this->GradientTableSize = config->GetGradientTableLength();
  delete config;

  // Create the context
  this->Context = VLIContext::Create();
  if (!this->Context)
    {
    vtkErrorMacro( << "Context could not be created!" );
    return;
    }

  this->LookupTable = VLILookupTable::Create(VLILookupTable::kSize4096);

  if ( !this->LookupTable )
    {
    vtkErrorMacro( << "Lookup table could not be created!" );
    return;    
    }

  this->Context->GetClassifier().SetLookupTable(kVLITable0, this->LookupTable);

  this->Cut = VLICutPlane::Create( 1.0, 0.0, 0.0, 0.0, 0.0, 0.0 );
  
  if ( !this->Cut )
    {
    vtkErrorMacro( << "Cut plane could not be created!" );
    return;    
    }
  
  this->DrawBoundingBox = 0;

  VLISetParameter("MipmapMinVolumeSize", 20);
}



vtkVolumeProVP1000Mapper::~vtkVolumeProVP1000Mapper()
{
  int i;

  // free the lights
  if (this->NumberOfLights > 0)
    {      
    for ( i = 0; i < this->NumberOfLights; i++ )
      {
      this->Context->RemoveLight( this->Lights[i] );
      this->Lights[i]->Release();
      }
    if ( this->Lights )
      {
      delete [] this->Lights;
      }
    }
  
  if (this->Cut)
    {
    this->Cut->Release();
    }

  // Free the lookup table if it was created
  if ( this->LookupTable )
    {
    this->LookupTable->Release();
    }

  // Free the volume if necessary
  if ( this->Volume )
    {
    if (this->Volume->IsLocked() == VLItrue)
      {
      this->Volume->UnlockVolume();
      }
    this->Volume->Release();
    }

  if (this->ImageBuffer)
    {
    this->ImageBuffer->Release();
    this->ImageBuffer = NULL;
    }
  
  if (this->DepthBuffer)
    {
    this->DepthBuffer->Release();
    this->DepthBuffer = NULL;
    }

  // Free the context if necessary
  if (this->Context)
    {
    this->Context->Release();
    }
  
  // Terminate connection to the hardware
  VLIClose();
}

vtkVolumeProVP1000Mapper *vtkVolumeProVP1000Mapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkVolumeProVP1000Mapper");
  if(ret)
    {
    return (vtkVolumeProVP1000Mapper*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  const char *temp = vtkGraphicsFactory::GetRenderLibrary();
  
#if defined(VTK_USE_OGLR) || defined(VTK_USE_OSMESA)
  if (!strcmp("OpenGL",temp))
    {
#ifdef VTK_DEBUG_LEAKS
    vtkDebugLeaks::DestructClass("vtkVolumeProVP1000Mapper");
#endif
    return vtkOpenGLVolumeProVP1000Mapper::New();
    }
#endif
#ifdef _WIN32
  if (!strcmp("Win32OpenGL",temp))
    {
#ifdef VTK_DEBUG_LEAKS
    vtkDebugLeaks::DestructClass("vtkVolumeProVP1000Mapper");
#endif
    return vtkOpenGLVolumeProVP1000Mapper::New();
    }
#endif
  
  return new vtkVolumeProVP1000Mapper;
}



void vtkVolumeProVP1000Mapper::UpdateCamera( vtkRenderer *ren, vtkVolume * vtkNotUsed(vol) )
{
  double                    positionVTK[3];
  double                    focalPointVTK[3];
  double                    viewUpVTK[3];
  VLIStatus                 status;

  // Get the necessary information from the vtk camera
  ren->GetActiveCamera()->GetPosition( positionVTK );
  ren->GetActiveCamera()->GetFocalPoint( focalPointVTK );
  ren->GetActiveCamera()->GetViewUp( viewUpVTK );

  // make sure we are in parallel mode
  if (!ren->GetActiveCamera()->GetParallelProjection())
    {
    vtkWarningMacro("The Volume Pro VP1000 does not support perspective projection and the camera is currently not in ParallelProjection mode.");
    }

  // Create the three vectors we need to do the lookat
  VLIVector3D positionVLI ( positionVTK );
  VLIVector3D focalPointVLI ( focalPointVTK );
  VLIVector3D viewUpVLI ( viewUpVTK );

  // Create a camera from this matrix
  VLIMatrix viewMatrixVLI = VLIMatrix::LookAt(positionVLI, focalPointVLI,
                                              viewUpVLI );
  status = this->Context->GetCamera().SetViewMatrix( viewMatrixVLI );

  double clippingRange[2], parallelScale;
  double aspect[2];
  ren->GetActiveCamera()->GetClippingRange(clippingRange);
  ren->GetAspect(aspect);
  parallelScale = ren->GetActiveCamera()->GetParallelScale();

  VLIMatrix projectionMatrixVLI = VLIMatrix::Ortho(-parallelScale*aspect[0],
                                                   parallelScale*aspect[0],
                                                   -parallelScale,
                                                   parallelScale,
                                                   clippingRange[0],
                                                   clippingRange[1]);
  
  status = this->Context->GetCamera().SetProjectionMatrix( projectionMatrixVLI );

  if ( status != kVLIOK )
    {
    vtkErrorMacro( << "Camera matrix not set!" );
    }

  if ( this->SuperSampling )
    {
    if (this->SuperSamplingFactor[2] == 0.0)
      {
      status = this->Context->SetSamplingFactor(1.0);
      }
    else
      {
      status =
        this->Context->SetSamplingFactor(1/this->SuperSamplingFactor[2]);
      }
    if ( status != kVLIOK )
      {
      vtkErrorMacro( << "Could not set the sampling factor!" );
      }
    }
  else
    {
    this->Context->SetSamplingFactor( 1.0 );
    }
}



void vtkVolumeProVP1000Mapper::UpdateLights( vtkRenderer *ren, vtkVolume *vol )
{
  vtkLight     *light;
  float        status;
  int          count;
  int          index;
  double       position[3], focalPoint[3];
  float        intensity;
  VLIVector3D  direction;
  int          i;

  // How many lights do we have?
  count = 0;
  for( ren->GetLights()->InitTraversal(); 
      (light = ren->GetLights()->GetNextItem()); )
    {
    status = light->GetSwitch();
    if ( status > 0.0 )
      {
      count++;
      }
    }

  if ( count > this->NumberOfLights )
    {      
    for ( i = 0; i < this->NumberOfLights; i++ )
      {
      this->Context->RemoveLight( this->Lights[i] );
      this->Lights[i]->Release();
      }
    if ( this->Lights )
      {
      delete [] this->Lights;
      }

    this->NumberOfLights = count;
    this->Lights = new VLILight* [count];

    for ( i = 0; i < this->NumberOfLights; i++ )
      {
      this->Lights[i] =  VLILight::CreateDirectional( );
      this->Context->AddLight( this->Lights[i] );
      }
    }

  index = 0;
  if ( vol->GetProperty()->GetShade() )
    {
    for(ren->GetLights()->InitTraversal(); 
        (light = ren->GetLights()->GetNextItem()); )
      {
      status = light->GetSwitch();
      if ( status > 0.0 )
        {
        light->GetPosition( position );
        light->GetFocalPoint( focalPoint );
        intensity = light->GetIntensity();
        direction.Assign( (focalPoint[0] - position[0]),
                          (focalPoint[1] - position[1]),
                          (focalPoint[2] - position[2]) );
        direction.Normalize();
        this->Lights[index]->SetDirection( direction );
        this->Lights[index]->SetIntensity( intensity );
        index++;
        }
      }
    }
  
  for ( i = index; i < this->NumberOfLights; i++ )
    {
    this->Lights[i]->SetIntensity( 0.0 );
    }
}

void vtkVolumeProVP1000Mapper::UpdateProperties( vtkRenderer *vtkNotUsed(ren), 
                                                vtkVolume *vol )
{
  vtkPiecewiseFunction      *grayFunc;
  vtkPiecewiseFunction      *goFunc;
  vtkPiecewiseFunction      *soFunc;
  vtkColorTransferFunction  *rgbFunc;
  VLIuint8                  rgbTable[4096][3];
  VLIuint16                 aTable[4096];
  int                       i;
  float                     scale = 1.0;
  double                    *gradientTable;
  float                     val;

  switch ( this->VolumeDataType )
    {
    case VTK_VOLUME_8BIT:
      scale = 255.0 / 4095.0;
      break;
    case VTK_VOLUME_12BIT_LOWER:
      scale = 1.0;
      break;
    case VTK_VOLUME_16BIT:
      scale = 65535.0 / 4095.0;
      break;
    }

  soFunc = vol->GetProperty()->GetScalarOpacity();

  switch ( vol->GetProperty()->GetColorChannels() )
    {
    case 1:
      grayFunc = vol->GetProperty()->GetGrayTransferFunction();
      for ( i= 0; i< 4096; i++)
        {
        val = 0.5 + grayFunc->GetValue(static_cast<float>(i)*scale)*255.0;
        val = (val < 0)?(0):(val);
        val = (val > 255)?(255):(val);
        rgbTable[i][0] = rgbTable[i][1] = rgbTable[i][2] 
          = static_cast<unsigned char>( val );
        
        val = 0.5 + 4095.0 * soFunc->GetValue(static_cast<float>(i)*scale);
        val = (val < 0)?(0):(val);
        val = (val > 4095)?(4095):(val);
        aTable[i] = static_cast<unsigned short>( val );
        }
      break;
    case 3:
      rgbFunc = vol->GetProperty()->GetRGBTransferFunction();
      for ( i= 0; i< 4096; i++)
        {
        val = 0.5 + rgbFunc->GetRedValue(static_cast<float>(i)*scale)*255.0;
        val = (val < 0)?(0):(val);
        val = (val > 255)?(255):(val);
        rgbTable[i][0] = static_cast<unsigned char>( val );

        val = 0.5 + rgbFunc->GetGreenValue(static_cast<float>(i)*scale)*255.0;
        val = (val < 0)?(0):(val);
        val = (val > 255)?(255):(val);
        rgbTable[i][1] = static_cast<unsigned char>( val );

        val = 0.5 + rgbFunc->GetBlueValue(static_cast<float>(i)*scale)*255.0;
        val = (val < 0)?(0):(val);
        val = (val > 255)?(255):(val);
        rgbTable[i][2] = static_cast<unsigned char>( val );

        val = 0.5 + 4095.0 * soFunc->GetValue(static_cast<float>(i)*scale);
        val = (val < 0)?(0):(val);
        val = (val > 4095)?(4095):(val);
        aTable[i] = static_cast<unsigned short>( val );
        }
      break;
    }

  this->LookupTable->SetColorEntries( 0, 4096, rgbTable );
  this->LookupTable->SetAlphaEntries( 0, 4096, aTable );

  // Set up the gradient magnitude opacity modulation
  goFunc = vol->GetProperty()->GetGradientOpacity();

  if ( !this->GradientOpacityModulation || !goFunc ||
       ( !strcmp(goFunc->GetType(), "Constant") && 
         goFunc->GetValue(0) == 1.0 ))
    {
    this->Context->SetGradientOpacityModulation( VLIfalse );
    }
  else
    {
    switch ( this->VolumeDataType )
      {
      case VTK_VOLUME_8BIT:
        scale = sqrt(3.0)*256.0;
        break;
      case VTK_VOLUME_12BIT_LOWER:
        scale = sqrt(3.0)*4096;
        break;
      case VTK_VOLUME_16BIT:
        scale = sqrt(3.0)*65536;
        break;
      }

    gradientTable = new double [this->GradientTableSize];
    double *spacing = this->GetInput()->GetSpacing();
    double avgSpacing = 0.333*(spacing[0] + spacing[1] + spacing[2]);
    scale = scale/(avgSpacing*(this->GradientTableSize-1));
    
    for ( i = 0; i < this->GradientTableSize; i++ )
      {
      // Take an average of five values in the region
      gradientTable[i] = 0.2 * ( 
        goFunc->GetValue(scale*(static_cast<float>(i - 0.4)))  +
        goFunc->GetValue(scale*(static_cast<float>(i-0.2))) +
        goFunc->GetValue(scale*(static_cast<float>(i))) +
        goFunc->GetValue(scale*(static_cast<float>(i+0.2))) +
        goFunc->GetValue(scale*(static_cast<float>(i+0.4))));
      }
    
    this->Context->SetGradientOpacityModulation( VLItrue );
    this->Context->SetGradientTable( gradientTable );
    delete [] gradientTable;
    }

  if ( vol->GetProperty()->GetShade() )
    {
    this->Context->
      SetReflectionProperties( vol->GetProperty()->GetDiffuse(),
                               vol->GetProperty()->GetSpecular(),
                               vol->GetProperty()->GetAmbient(),
                               vol->GetProperty()->GetSpecularPower() );
    }
  else
    {
    this->Context->SetReflectionProperties( 0.0, 0.0, 1.0, 1.0 );
    }

  this->Context->GetClassifier().SetLookupTable(kVLITable0, this->LookupTable);
}

void vtkVolumeProVP1000Mapper::UpdateCropping( vtkRenderer * vtkNotUsed(ren), vtkVolume * vtkNotUsed(vol) )
{
  VLICrop  *crop;

  crop = new VLICrop;

  crop->SetSlabs( this->VoxelCroppingRegionPlanes[0], 
                  this->VoxelCroppingRegionPlanes[1],
                  this->VoxelCroppingRegionPlanes[2], 
                  this->VoxelCroppingRegionPlanes[3],
                  this->VoxelCroppingRegionPlanes[4], 
                  this->VoxelCroppingRegionPlanes[5] );

  if ( !this->Cropping )
    {
    crop->SetFlags( VLICrop::kDisable );
    }
  else
    {
    switch ( this->CroppingRegionFlags )
      {
      case VTK_CROP_SUBVOLUME:
        crop->SetFlags( VLICrop::kSubVolume );
        break;
      case VTK_CROP_FENCE:
        crop->SetFlags( VLICrop::k3DFence );
        break;
      case VTK_CROP_INVERTED_FENCE:
        crop->SetFlags( VLICrop::k3DFenceInvert );
        break;
      case VTK_CROP_CROSS:
        crop->SetFlags( VLICrop::k3DCross );
        break;
      case VTK_CROP_INVERTED_CROSS:
        crop->SetFlags( VLICrop::k3DCrossInvert );
        break;
      default:
        crop->SetFlags( VLICrop::kDisable );
        vtkErrorMacro( << "Unsupported crop option!" );
        break;
      }
    }

  this->Context->SetCrop( *crop );

  delete crop;
}

void vtkVolumeProVP1000Mapper::UpdateCutPlane( vtkRenderer * vtkNotUsed(ren), vtkVolume *vtkNotUsed(vol) )
{
  VLIStatus   status;

  // If the cut plane is turned off, but the context has a cut plane,
  // then we need to remove it
  if ( !this->CutPlane )
    {
    // Remove it if necessary
    if ( this->Context->GetCutPlaneCount() > 0 )
      {
      status = this->Context->RemoveCutPlane( this->Cut );
      if ( status != kVLIOK )
        {
        vtkErrorMacro( << "Could not remove cut plane from context" );
        }
      }
    }
  // If the cut plane is turned on, and the context does not have a cut
  // plane, then we need to add it. Also, update the position/orientation
  // and thickness of the plane
  else
    {
    // Update the position/orientation
    status = this->Cut->SetPlane( this->CutPlaneEquation[0],
                                  this->CutPlaneEquation[1],
                                  this->CutPlaneEquation[2],
                                  this->CutPlaneEquation[3] );
    if ( status != kVLIOK )
      {
      vtkErrorMacro( << "Could not set cut plane equation" );
      }

    // Update the thickness
    status = this->Cut->SetThickness( this->CutPlaneThickness );
    if ( status != kVLIOK )
      {
      vtkErrorMacro( << "Could not set cut plane thickness" );
      }

    // Update the falloff distance
    status = this->Cut->SetFallOff( this->CutPlaneFallOffDistance );
    if ( status != kVLIOK )
      {
      vtkErrorMacro( << "Could not set cut plane fall off distance" );
      }

    // Add it if necessary
    if ( this->Context->GetCutPlaneCount() == 0 )
      {
      status = this->Context->AddCutPlane( this->Cut );
      if ( status != kVLIOK )
        {
        vtkErrorMacro( << "Could not remove cut plane from context" );
        }
      }
    }
}

void vtkVolumeProVP1000Mapper::UpdateCursor( vtkRenderer *vtkNotUsed(ren), vtkVolume *vtkNotUsed(vol) )
{
}

void vtkVolumeProVP1000Mapper::UpdateVolume( vtkRenderer * vtkNotUsed(ren), vtkVolume * vol )
{
  int                       dataSize[3];
  int                       dataType;
  unsigned char             *uc_data_ptr;
  unsigned short            *us_data_ptr;
  void                      *data_ptr;
  vtkImageData              *input = this->GetInput();
  vtkTransform              *correctionTransform;
  vtkTransform              *modelTransform;
  int                       i, j;
  double                    dataOrigin[3];
  double                    dataSpacing[3];
  VLIStatus                 status;
  double                    range[2];

  // We need the size to create the volume and check the subvolume
  input->GetDimensions( dataSize );
  VLIVolumeRange volumeRange (dataSize[0], dataSize[1], dataSize[2]);
  
  // If we have a volume, the size still matches, but our data has
  // been modified, call UpdateVolume() to change the content
  if ( this->Volume &&
       input == this->VolumeInput &&
       input->GetMTime() >= this->VolumeBuildTime->GetMTime() &&
       this->LoadedDataSize[0] == dataSize[0] && 
       this->LoadedDataSize[1] == dataSize[1] &&
       this->LoadedDataSize[2] == dataSize[2] )
    {
    int volumeUpdated = 0;
    
    // Get the data type and a void * pointer to the data
    dataType = input->GetPointData()->GetScalars()->GetDataType();
    data_ptr = input->GetPointData()->GetScalars()->GetVoidPointer(0);
    
    // Switch on data type and update the volume
    switch ( dataType )
      {
      case VTK_UNSIGNED_CHAR:
        if ( this->VolumeDataType == VTK_VOLUME_8BIT )
          {
          uc_data_ptr = static_cast<unsigned char *>(data_ptr);
          this->Volume->Update( uc_data_ptr, volumeRange );
          volumeUpdated = 1;
          }
        
        break;
        
      case VTK_UNSIGNED_SHORT:
        if ( this->VolumeDataType == VTK_VOLUME_16BIT ||
             this->VolumeDataType == VTK_VOLUME_12BIT_LOWER)
          {
          us_data_ptr = static_cast<unsigned short *>(data_ptr);
          this->Volume->Update(us_data_ptr, volumeRange);
          volumeUpdated = 1;
          }
        break;
        
      default:
        vtkErrorMacro( "You must convert your data to unsigned char or " <<
                       "unsigned short for a VolumePro mapper" );
        break;
      }
    
    if ( volumeUpdated )
      {
      this->VolumeBuildTime->Modified();
      }
    }
  
  // If we have a volume, it is the one we last built with, and it
  // has not been modified since then, then we don't need to rebuilt
  if ( !this->Volume || 
       input != this->VolumeInput ||
       input->GetMTime() >= this->VolumeBuildTime->GetMTime() )
    {
    // Otherwise, we need to build the volume
    this->VolumeInput = input;
    this->VolumeBuildTime->Modified();
    
    // If we already have one, get rid of it
    if ( this->Volume )
      {
      this->Volume->Release();
      this->Volume = NULL;
      }
    
    // Get the data type and a void * pointer to the data
    dataType = input->GetPointData()->GetScalars()->GetDataType();
    data_ptr = input->GetPointData()->GetScalars()->GetVoidPointer(0);

    // Switch on data type and create the volume
    switch ( dataType )
      {
      case VTK_UNSIGNED_CHAR:
        uc_data_ptr = static_cast<unsigned char *>(data_ptr);
        this->Volume = VLIVolume::Create( 8, dataSize[0], dataSize[1],
                                          dataSize[2], 0, 0, uc_data_ptr );
        this->Volume->SetFieldDescriptor(kVLIField0,
                                         VLIFieldDescriptor(0, 8, kVLIUnsignedFraction));
        this->Volume->SetMipmapAutoGenerate(1);

        this->VolumeDataType = VTK_VOLUME_8BIT;
        
        break;
        
      case VTK_UNSIGNED_SHORT:
        us_data_ptr = static_cast<unsigned short *>(data_ptr);
        this->Volume = VLIVolume::Create( 16, dataSize[0], dataSize[1],
                                          dataSize[2], 0, 0, us_data_ptr );
        
        this->Volume->SetMipmapAutoGenerate(1);
        input->GetPointData()->GetScalars()->GetRange( range );
        if ( range[1] > 4095 )
          {
          this->Volume->SetFieldDescriptor(kVLIField0,
                                           VLIFieldDescriptor(0, 16, kVLIUnsignedFraction));
          this->VolumeDataType = VTK_VOLUME_16BIT;
          }
        else
          {
          this->Volume->SetFieldDescriptor(kVLIField0,
                                           VLIFieldDescriptor(0, 12, kVLIUnsignedFraction));

          this->VolumeDataType = VTK_VOLUME_12BIT_LOWER;
          }
        
        break;
        
      default:
        vtkErrorMacro( << "You must convert your data to unsigned char or "
                       << "unsigned short for a VolumePro mapper" );
        break;
      }
    }
  
  // Keep the data size for our check next time
  this->LoadedDataSize[0] = dataSize[0];
  this->LoadedDataSize[1] = dataSize[1];
  this->LoadedDataSize[2] = dataSize[2];

  // Store the matrix of the volume in a temporary transformation matrix
  modelTransform = vtkTransform::New();
  modelTransform->SetMatrix( vol->vtkProp3D::GetMatrix() );

  // Get the origin of the data.  This translation is not accounted for in
  // the volume's matrix, so we must add it in.
  input->GetOrigin( dataOrigin );

  // Get the data spacing.  This scaling is not accounted for in
  // the volume's matrix, so we must add it in.
  input->GetSpacing( dataSpacing );

  // Create a transform that will account for the scaling and translation of
  // the scalar data
  correctionTransform = vtkTransform::New();
  correctionTransform->Identity();
  correctionTransform->Translate(dataOrigin[0], dataOrigin[1], dataOrigin[2]);
  correctionTransform->Scale( dataSpacing[0], dataSpacing[1], dataSpacing[2] );

  VLIMatrix     correctionMatrixVLI;
  VLIMatrix modelMatrixVLI;

  // Now copy the matrix out (inverted) into an array of doubles
  for ( j = 0; j < 4; j++ )
    for ( i = 0; i < 4; i++ )
      {
      modelMatrixVLI[i][j] = modelTransform->GetMatrix()->GetElement( i, j );
      correctionMatrixVLI[i][j] = correctionTransform->GetMatrix()->GetElement( i, j );
      }

  if( this->Volume )
    {
    status = this->Volume->SetCorrectionMatrix( correctionMatrixVLI );
    if ( status != kVLIOK )
      {
      vtkErrorMacro( << "Error setting the correction matrix: " << status );
      }
    }

  status = this->Context->GetCamera().SetModelMatrix( modelMatrixVLI );
  if ( status != kVLIOK )
    {
    vtkErrorMacro( << "Error setting the model matrix: " << status );
    }
  
  // Delete the objects we created
  correctionTransform->Delete();
  modelTransform->Delete();

  // Update the subvolume if it is reasonable
  if ( this->Volume &&
       this->SubVolume[0] >= 0 && 
       this->SubVolume[2] >= 0 &&
       this->SubVolume[4] >= 0 &&
       this->SubVolume[0] < dataSize[0] &&
       this->SubVolume[2] < dataSize[1] &&
       this->SubVolume[4] < dataSize[2] &&
       this->SubVolume[1] >= this->SubVolume[0] && 
       this->SubVolume[3] >= this->SubVolume[2] &&
       this->SubVolume[5] >= this->SubVolume[4] &&
       this->SubVolume[1] < dataSize[0] &&
       this->SubVolume[3] < dataSize[1] &&
       this->SubVolume[5] < dataSize[2] )
    {
    VLIVolumeRange volRange ((this->SubVolume[1]-this->SubVolume[0]) + 1,
                             (this->SubVolume[3]-this->SubVolume[2]) + 1,
                             (this->SubVolume[5]-this->SubVolume[4]) + 1,
                             this->SubVolume[0], this->SubVolume[2],
                             this->SubVolume[4] );
    status = 
      this->Volume->SetActiveSubVolume( volRange );
    if ( status != kVLIOK )
      {
      vtkErrorMacro( << "Could not set the active subvolume" );
      }
    }
}

int vtkVolumeProVP1000Mapper::GetAvailableBoardMemory()
{
  int               memory;
  VLIConfiguration  *config;

  config = new VLIConfiguration;
  memory = config->GetAvailableMemory( 0 );
  delete config;
  
  return memory;
}

void vtkVolumeProVP1000Mapper::GetLockSizesForBoardMemory( unsigned int type,
                                                     unsigned int *xSize,
                                                     unsigned int *ySize,
                                                     unsigned int *zSize )
{
  VLIConfiguration  *config;

  config = new VLIConfiguration;
  config->GetMaxLockedSize( type, *xSize, *ySize, *zSize );
  delete config;
}

void vtkVolumeProVP1000Mapper::Render( vtkRenderer *ren, vtkVolume *vol )
{
  int                       size[2];
  VLIStatus                 status;
  //return;
  
  if ( !this->StatusOK() )
    {
    return;
    }

  // make sure that we have scalar input and update the scalar input
  if ( this->GetInput() == NULL ) 
    {
    vtkErrorMacro(<< "No Input!");
    return;
    }
  else
    {    
    this->GetInput()->UpdateInformation();
    this->GetInput()->SetUpdateExtentToWholeExtent();
    this->GetInput()->Update();
    } 

  this->RenderTimer->StartTimer();

  this->ConvertCroppingRegionPlanesToVoxels();
  
  this->UpdateCamera( ren, vol );

  this->UpdateLights( ren, vol);

  this->UpdateVolume( ren, vol );

  this->UpdateProperties( ren, vol );  

  if ( !this->Volume )
    {
    this->RenderTimer->StopTimer();
    return;
    }

  this->UpdateCropping( ren, vol );

  this->UpdateCutPlane( ren, vol );

  this->UpdateCursor( ren, vol );

  this->Context->SetCorrectGradient (VLItrue);

  switch ( this->BlendMode )
    {
    case VTK_BLEND_MODE_COMPOSITE:
      this->Context->SetBlendMode( kVLIBlendFTB );
      break;
    case VTK_BLEND_MODE_MAX_INTENSITY:
      this->Context->SetBlendMode( kVLIBlendMIP );
      break;
    case VTK_BLEND_MODE_MIN_INTENSITY:
      this->Context->SetBlendMode( kVLIBlendMINIP );
      break;
    default:
      vtkErrorMacro( << "Unknown blending mode: " << this->BlendMode );
      break;
    }

  int *windowSize;
  windowSize = ren->GetRenderWindow()->GetSize();
  
  status = this->Volume->LockVolume();

  if ( this->ImageBuffer )
    {
    unsigned int width, height;
    this->ImageBuffer->GetSize(width, height);
    if (static_cast<int>(width) != windowSize[0] || 
        static_cast<int>(height) != windowSize[1])
      {
      this->ImageBuffer->Release();
      this->ImageBuffer = NULL;
      }
    }
  if ( ! this->ImageBuffer )
    {
    static VLIFieldDescriptor sImageBufferFields[4] =
    {
      VLIFieldDescriptor(0, 8, kVLIUnsignedFraction),
      VLIFieldDescriptor(8, 8, kVLIUnsignedFraction),
      VLIFieldDescriptor(16, 8, kVLIUnsignedFraction),
      VLIFieldDescriptor(24, 8, kVLIUnsignedFraction)
    };
    
    this->ImageBuffer = VLIImageBuffer::Create(kVLIBoard0, windowSize[0],
                                               windowSize[1], 32, 4,
                                               sImageBufferFields);
    this->ImageBuffer->SetBorderValue(0, 0, 0, 0);
    }
  
  this->Context->SetRayTermination(1.0, VLIfalse);


  float allocatedTime = vol->GetAllocatedRenderTime();

  if (this->AutoAdjustMipmapLevels)
    {
    if (allocatedTime < 10)
      {
      float oldTime = this->RetrieveRenderTime(ren, vol);
      float offset = allocatedTime * 0.2;
      if ( oldTime > (allocatedTime+offset) )
        {
        this->MipmapLevel++;
        }
      else if ( oldTime < (allocatedTime-offset) )
        {
        this->MipmapLevel--;
        }
      this->MipmapLevel =
        (this->MipmapLevel > this->MaximumMipmapLevel) ?
        (this->MaximumMipmapLevel) : (this->MipmapLevel);
      this->MipmapLevel =
        (this->MipmapLevel < this->MinimumMipmapLevel) ?
        (this->MinimumMipmapLevel) : (this->MipmapLevel);
      }
    else
      {
      this->MipmapLevel = this->MinimumMipmapLevel;
      }
    }

  if (allocatedTime < 10)
    {
    this->Volume->SetMipmapRange(this->MipmapLevel, this->MipmapLevel);
    }
  else
    {
    this->Volume->SetMipmapRange(this->MinimumMipmapLevel,
                                 this->MinimumMipmapLevel);
    }


  int width = 0, height = 0;
  
  this->CheckSubSampling(this->Volume, this->Context, width, height);
  
  int imageWidth, imageHeight;
  imageWidth = this->ImageBuffer->GetWidth();
  imageHeight = this->ImageBuffer->GetHeight();
  
  this->DrawBoundingBox = 0;
  
  if (width > imageWidth || height > imageHeight)
    {
    if (width < 2000 && height < 2000)
      {
      int widthDiff, heightDiff, newWidth, newHeight;
      float increase;
      
      widthDiff = width - imageWidth;
      heightDiff = height - imageHeight;
      if (widthDiff > heightDiff)
        {
        increase = (float)width / (float)imageWidth;
        newWidth = width;
        newHeight = ceil(imageHeight*increase);
        }
      else
        {
        increase = (float)height / (float)imageHeight;
        newWidth = ceil(imageWidth*increase);
        newHeight = height;
        }
      this->ImageBuffer->Release();
      static VLIFieldDescriptor sImageBufferFields[4] =
      {
        VLIFieldDescriptor(0, 8, kVLIUnsignedFraction),
        VLIFieldDescriptor(8, 8, kVLIUnsignedFraction),
        VLIFieldDescriptor(16, 8, kVLIUnsignedFraction),
        VLIFieldDescriptor(24, 8, kVLIUnsignedFraction)
      };
      
      this->ImageBuffer = VLIImageBuffer::Create(kVLIBoard0, newWidth,
                                                 newHeight, 32, 4,
                                                 sImageBufferFields);
      this->ImageBuffer->SetBorderValue(0, 0, 0, 0);
      }
    else
      {
      this->DrawBoundingBox = 1;
      }
    }

  if (this->BlendMode == VTK_BLEND_MODE_MIN_INTENSITY)
    {
    this->ImageBuffer->SetBorderValue(1, 1, 1, 1);
    }
  else
    {
    this->ImageBuffer->SetBorderValue(0, 0, 0, 0);
    }

  if ( ! this->DrawBoundingBox)
    {
    if ( ! this->IntermixIntersectingGeometry )
      {
      status = this->Volume->Render(this->Context, this->ImageBuffer);
      }
    else
      {
      VLIImageRange iRange = VLIImageRange(windowSize[0], windowSize[1]);
      if ( this->DepthBuffer )
        {
        unsigned int width, height;
        this->DepthBuffer->GetSize(width, height);
        if (static_cast<int>(width) != windowSize[0] || 
            static_cast<int>(height) != windowSize[1])
          {
          this->DepthBuffer->Release();
          this->DepthBuffer = NULL;
          }
        }
      if ( ! this->DepthBuffer )
        {
        this->DepthBuffer = VLIDepthBuffer::Create(kVLIBoard0, windowSize[0],
                                                   windowSize[1]);
        this->DepthBuffer->SetBorderValue(0);
        this->DepthBuffer->SetInputLimits(iRange);
        status = this->Context->SetDepthTest(VLIContext::kDepthBuffer1,
                                             VLIContext::kDepthTestLess);
        }
      unsigned int *depthData = new unsigned int[windowSize[0]*windowSize[1]];
      this->GetDepthBufferValues(ren, windowSize, depthData);
      
      status = this->DepthBuffer->Update(depthData,
                                         VLIImageRange(windowSize[0],
                                                       windowSize[1]));
      if ( status != kVLIOK )
        {
        switch ( status )
          {
          case kVLIErrArgument:
            vtkErrorMacro( << "Invalid argument for updating depth buffer!" );
            break;
          case kVLIErrAlloc:
            vtkErrorMacro( << "Not enough resources to update depth buffer!" );
            break;
          default:
            // Don't know what the error is, but can't update the depth buffer.
            // Shouldn't get to this error message.
            vtkErrorMacro( << "Unknown error updating depth buffer!" );
            break;
          }
        this->RenderTimer->StopTimer();
        return;
        }
      this->ImageBuffer->Clear(iRange, 0);
      status = this->Volume->Render(this->Context, this->ImageBuffer, 0, 0,    
                                    this->DepthBuffer);
      
      delete [] depthData;
      }
    
    if ( status != kVLIOK )
      {
      switch ( status )
        {
        case kVLIErrArgument:
          vtkErrorMacro( << "Volume could not be rendered - bad argument!" );
          break;
        case kVLIErrCantSubsample:
          vtkErrorMacro( << "Volume could not be rendered - volume too large for viewport!");
          break;
        case kVLIErrClassifier:
          vtkErrorMacro( << "Volume could not be rendered - invalid classifier!");
          break;
        case kVLIErrTransform:
          vtkErrorMacro( << "Volume could not be rendered - invalid transform state!");
          break;
        case kVLIErrAccess:
          vtkErrorMacro( << "Volume could not be rendered - could not access volume!" );
          break;
        case kVLIErrPermission:
          vtkErrorMacro( << "Volume could not be rendered - do not have permission to perform render!");
          break;
        case kVLIErrVolume:
          vtkErrorMacro( << "Volume could not be rendered - no attached buffer!");
          break;
        case kVLIErrAlloc:
          vtkErrorMacro( << "Volume could not be rendered - not enough resources!" );
          break;
        default:
          // Don't report the error - this volume just won't render
          vtkErrorMacro( << "Volume could not be rendered - unkown error!" );
          break;    
        }

      this->RenderTimer->StopTimer();
      return;
      }
    
    size[0] = this->ImageBuffer->GetWidth();
    size[1] = this->ImageBuffer->GetHeight();
    
    unsigned int *outData = new unsigned int[size[0]*size[1]];
    
    status = this->ImageBuffer->Unload(outData,
                                       this->ImageBuffer->GetOutputLimits());
    
    if ( status != kVLIOK )
      {
      switch (status)
        {
        case kVLIErrArgument:
          vtkErrorMacro("Image buffer could not be unloaded - invalid argument!");
          break;
        case kVLIErrAlloc:
          vtkErrorMacro("Image buffer could not be unloaded - not enough resources!");
          break;
        case kVLIErrInternal:
          vtkErrorMacro("Image buffer could not be unloaded - internal VLI error!");
          break;
        default:
          vtkErrorMacro("Image buffer could not be unloaded - unknown error!");
        }
      }
    
    // Render the image buffer we've been returned.
    this->RenderImageBuffer(ren, vol, size, outData);
    
    delete [] outData;
    }
  else
    {
    this->RenderBoundingBox(ren, vol);
    }

  this->RenderTimer->StopTimer();
  this->StoreRenderTime(ren, vol, this->RenderTimer->GetElapsedTime());
}

#if ((VTK_MAJOR_VERSION == 3)&&(VTK_MINOR_VERSION == 2))
void vtkVolumeProVP1000Mapper::ConvertCroppingRegionPlanesToVoxels()
{
  memcpy( this->VoxelCroppingRegionPlanes, this->CroppingRegionPlanes,
          sizeof ( this->VoxelCroppingRegionPlanes ) );
}
#endif

VLIStatus vtkVolumeProVP1000Mapper::CheckSubSampling(const VLIVolume *inVolume,
                                                     const VLIContext *inContext,
                                                     int &outMinImageWidth,
                                                     int &outMinImageHeight)
{
  enum VGAxis
  {
    kU = 0,
    kV = 1,
    kW = 2
  };
  
  enum VGNeg
  {
    kNotNeg = 0,
    kIsNeg = 1
  };

  const double kEpsilonSubSample = 1.999;
  
  
  //////////////////////////////////////////////////////////////////////////
  //
  // 1) Initialize and calculate matrices: 
  //
  //////////////////////////////////////////////////////////////////////////
  
  VLIStatus status = kVLIOK;
  
  double depthNear, depthFar;
  int viewportMinX, viewportMinY, viewportWidth, viewportHeight;
  inContext->GetCamera().GetViewport(viewportMinX, viewportMinY,
                                     viewportWidth, viewportHeight);
  inContext->GetCamera().GetDepthRange(depthNear, depthFar);
  
  if (viewportWidth <=0 || viewportHeight <=0)
    {
    viewportWidth  = 3;
    viewportHeight = 3;
    viewportMinX = 0;
    viewportMinY = 0;
    status = kVLIErrCantSubsample;
    }
  
  //------------------------------------------------------
  // Calculate viewport matrix from viewport paramteres
  //------------------------------------------------------
  
  VLIMatrix viewportMatrix;
  viewportMatrix.Assign(
    (viewportWidth -1)/2.0, 0, 0, (viewportWidth -1)/2.0  + viewportMinX,
    0, (viewportHeight-1)/2.0, 0, (viewportHeight-1)/2.0  + viewportMinY,
    0, 0, (depthFar-depthNear)/2.0, (depthFar+depthNear)/2.0,
    0, 0, 0, 1.0);
  
  //------------------------------------------------------
  // Calculate viewport-viewmapping(projection)-CorrectedModelView matrix 
  //-------------------------------------------------------
  
  VLIMatrix projection = inContext->GetCamera().GetProjectionMatrix();
  VLIMatrix viewMatrix = inContext->GetCamera().GetViewMatrix();
  VLIMatrix model = inContext->GetCamera().GetModelMatrix();
  VLIMatrix correction = inVolume->GetCorrectionMatrix();
  
  VLIMatrix VP_VM_CRMVMatrix = 
    viewportMatrix * 
    inContext->GetCamera().GetProjectionMatrix() *
    inContext->GetCamera().GetViewMatrix() *
    inContext->GetCamera().GetModelMatrix() *
    inVolume->GetCorrectionMatrix();
  
  if (VP_VM_CRMVMatrix.IsSingular())
    {
    return kVLIErrTransform;
    }
  
  
  //////////////////////////////////////////////////////////////////////////
  //
  // 2) Calculate permutation matrix
  //   a) Choose primary axis to permuted space
  //   b) Decide the select and neg values in transform
  //   c) Construct permutation matrix considering min block
  //////////////////////////////////////////////////////////////////////////
  
  //------------------------------------------------------
  //   a) Choose primary axis to permuted space,
  //
  //      The Z axis of the permuted space  
  //      is one axis of object space that is 
  //      closest to the casting ray direction
  //------------------------------------------------------
  
  
  VLIVector4D view(0.0, 0.0, 1.0, 0.0);  // view vector in image space
  
  VLIMatrix VP_VM_CRMVInverse = VP_VM_CRMVMatrix.Inverse();
  
  VLIVector4D viewVectorInObjectSpace = (VP_VM_CRMVInverse * view).Normalize();
  
  double max = fabs (viewVectorInObjectSpace[0]);
  VGAxis primaryAxis = kU;
  
  if (fabs (viewVectorInObjectSpace[1]) > max) 
    {
    primaryAxis = kV;
    max = fabs(viewVectorInObjectSpace[1]);
    }
  
  if (fabs (viewVectorInObjectSpace[2]) > max) 
    {
    primaryAxis = kW;
    max = fabs(viewVectorInObjectSpace[2]);
    }
  
  //------------------------------------------------------
  //    b) Decide the select and neg values in transform
  //
  //       selectZ is the primary axis, negZ as its direction
  //
  //       selectX, selectY is chosen
  //       to keep the coordinate system order
  //------------------------------------------------------
  
  int dirSignOfViewVector[3]; // sign of du, dv, dw direction
  int i;
  for ( i = 0; i < 3 ; i++)
    {
    dirSignOfViewVector[i] =(viewVectorInObjectSpace[i] < 0 )? -1:1;
    }
  
  // even: 1, odd -1
  int even = dirSignOfViewVector[0] * dirSignOfViewVector[1]* 
    dirSignOfViewVector[2];
  
  
  int axisObj = primaryAxis; // axis of Object Space
  int negSign[3];
  
  // SelectZ, negZ
  VGAxis select[3];
  VGNeg neg[3];
  select[2] = primaryAxis; 
  neg[2] = (dirSignOfViewVector[axisObj] == 1)?kNotNeg:kIsNeg;
  negSign[2] = dirSignOfViewVector[axisObj];
  
  // SelectX, SelectY, negX, negY
  int axisP; // Axis of Permuted Space
  
  // The permuted axes are chosen to keep the same coordinate order
  // (right or left) as object space
  for ( i = 1; i < 3; i++)
    {
    axisP = (2+ i* even)%3;
    axisObj = (primaryAxis +i) %3;
    
    select[axisP] = (VGAxis)(axisObj);
    neg[axisP] = (dirSignOfViewVector[axisObj] == 1)?kNotNeg:kIsNeg;
    negSign[axisP] = dirSignOfViewVector[axisObj];
    }
  
  //-----------------------------------------------------------
  //    c) Construct permutation matrix considering mini block
  //------------------------------------------------------------
  
#define SELECT_AXIS(select, axisObj) ((select == axisObj)?1:0) 
#define PERMUTE(axisP,axisObj) negSign[axisP] * SELECT_AXIS(select[axisP], axisObj)
  
  // shift -1 if du, dv, dw < 0
#define SHIFT(index) ((negSign[index]==1)? 0:-1)
  VLIMatrix permutation;
  permutation.Assign(
    PERMUTE(0,kU), PERMUTE(0,kV),PERMUTE(0,kW), SHIFT(0),
    PERMUTE(1,kU), PERMUTE(1,kV),PERMUTE(1,kW), SHIFT(1),
    PERMUTE(2,kU), PERMUTE(2,kV),PERMUTE(2,kW), SHIFT(2),
    0, 0, 0, 1);
  
  //////////////////////////////////////////////////////////////////////////
  //  3) Calculate sample space increment registers using matrices
  //////////////////////////////////////////////////////////////////////////
  
  VLIMatrix VP_VM_CRMVPermuted = VP_VM_CRMVMatrix * permutation.Inverse();
  
  // 3.a) The 2x2 upper part of dI_DV Matrix is same in  dS_dV Matrix
  
  VLIMatrix dS_dVsubMatrix = VLIMatrix::Identity();
  
  dS_dVsubMatrix[0][0] = VP_VM_CRMVPermuted[0][0];
  dS_dVsubMatrix[0][1] = VP_VM_CRMVPermuted[0][1];
  dS_dVsubMatrix[1][0] = VP_VM_CRMVPermuted[1][0];
  dS_dVsubMatrix[1][1] = VP_VM_CRMVPermuted[1][1];
  
  VLIMatrix dV_dSsubMatrix  = dS_dVsubMatrix.Inverse();
  
  dV_dSsubMatrix[0][0] = fabs(dV_dSsubMatrix[0][0]);
  dV_dSsubMatrix[0][1] = fabs(dV_dSsubMatrix[0][1]);
  dV_dSsubMatrix[1][0] = fabs(dV_dSsubMatrix[1][0]);
  dV_dSsubMatrix[1][1] = fabs(dV_dSsubMatrix[1][1]);
  
  //////////////////////////////////////////////////////////
  //  A Scaling method that will
  //
  //  1) Change only the image size, (viewport)
  //
  //  2) keep the same permutation matrix
  //  
  //  3) keep valid DepthWarp Matrix
  //
  //  4) avoid sub sampling, so that
  // 
  //     VLIAbs(dXv_dXs) + VLIAbs(dXv_dYs) <=2
  //     VLIAbs(dYv_dXs) + VLIAbs(dYv_dYs) <=2
  //
  //     To conclude, get a scaleX, scaleY in imageSize
  // so that
  //     VLIAbs(dXv_dXs)/ScaleX+ VLIAbs(dXv_dYs)/ScaleY <=2
  //     VLIAbs(dXv_dYs)/ScaleX+ VLIAbs(dYv_dYs)/ScaleY <=2
  //
  //  Different ways to choose ScaleX, ScaleY to avoid sub Sampling
  //  We choose here when ScaleX == ScaleY, and most close to 
  //  ScaleX =1 , ScaleY =1
  //
  
  double dXvSampleMax= dV_dSsubMatrix[0][0] + dV_dSsubMatrix[0][1];
  double dYvSampleMax= dV_dSsubMatrix[1][0] + dV_dSsubMatrix[1][1];
  
  if (dXvSampleMax > 2.0 || dYvSampleMax > 2.0)
    {
    status = kVLIErrCantSubsample;
    }
  
  double viewportScale = dXvSampleMax > dYvSampleMax? dXvSampleMax:dYvSampleMax;
  
  viewportScale /= kEpsilonSubSample;
  //*************************************************
  //Solution I:
  //double dXvSampleMax= dV_dSsubMatrix[0][0] > dV_dSsubMatrix[1][0]? dV_dSsubMatrix[0][0]:dV_dSsubMatrix[1][0];
  //double dYvSampleMax= dV_dSsubMatrix[0][1] > dV_dSsubMatrix[1][1]? dV_dSsubMatrix[0][1]:dV_dSsubMatrix[1][1];
  //**************************************
  
  //*******************************
  //Solution II:
  /*Not proper to choose the interesection of the two equation!!!!
    VLIVector4D imageScale(2,2,0,0);
    dS_dVsubMatrix = dV_dSsubMatrix.Inverse();
    imageScale = dS_dVsubMatrix * imageScale;
    
    outMinImageWidth  = (int)ceil( (viewportWidth -1)/imageScale[0] +1);
    outMinImageHeight = (int)ceil( (viewportHeight-1)/imageScale[1] +1);*/
    
    // Instead of using scaling dXvSampleMax/2.0, using dZvSampleMax/kEpsionSubSample
    outMinImageWidth  = (int)ceil( double (viewportWidth -1)*viewportScale +1);
    outMinImageHeight = (int)ceil( double (viewportHeight-1)*viewportScale +1);
    
    return status;
}

void vtkVolumeProVP1000Mapper::SetSuperSamplingFactor( double x, double y, double z )
{
  if ( z < 0.0625 || z > 16  )
    {
    vtkErrorMacro(
      << "Invalid supersampling factor" << endl <<
      "Only the Z value is considered, and it must be in the range [0.0625...16].");
    return;
    }

  this->SuperSamplingFactor[0] = x;
  this->SuperSamplingFactor[1] = y;
  this->SuperSamplingFactor[2] = z;

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkVolumeProVP1000Mapper::SetMipmapLevel(int level)
{
  this->Superclass::SetMipmapLevel(level);
  level = level > this->MaximumMipmapLevel ? this->MaximumMipmapLevel : level;
  level = level < this->MinimumMipmapLevel ? this->MinimumMipmapLevel : level;
  if (this->Volume)
    {
    this->Volume->SetMipmapRange(level, level);
    }
}

//----------------------------------------------------------------------------
void vtkVolumeProVP1000Mapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
