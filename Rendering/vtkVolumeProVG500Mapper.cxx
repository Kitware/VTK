/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeProVG500Mapper.cxx
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

#include <math.h>
#include "vtkVolumeProVG500Mapper.h"
#include "vtkOpenGLVolumeProVG500Mapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkVolume.h"
#include "vtkObjectFactory.h"
#include "vtkGraphicsFactory.h"
#include "vtkToolkits.h"
#include "vtkDebugLeaks.h"

vtkVolumeProVG500Mapper::vtkVolumeProVG500Mapper()
{
  VLIStatus         status;
  VLIConfiguration  *config;

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

  this->Context->SetLookupTable(this->LookupTable);


  this->Cut = VLICutPlane::Create( 1.0, 0.0, 0.0, 0.0, 0.0, 0.0 );
  
  if ( !this->Cut )
    {
    vtkErrorMacro( << "Cut plane could not be created!" );
    return;    
    }
}



vtkVolumeProVG500Mapper::~vtkVolumeProVG500Mapper()
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
    this->Volume->Release();
    }

  // Free the context if necessary
  if (this->Context)
    {
    this->Context->Release();
    }

  // Terminate connection to the hardware
  VLIClose();
}

vtkVolumeProVG500Mapper *vtkVolumeProVG500Mapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkVolumeProVG500Mapper");
  if(ret)
    {
    return (vtkVolumeProVG500Mapper*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  const char *temp = vtkGraphicsFactory::GetRenderLibrary();
  
#ifdef VTK_USE_OGLR
  if (!strcmp("OpenGL",temp))
    {
#ifdef VTK_DEBUG_LEAKS
    vtkDebugLeaks::DestructClass("vtkVolumeProVG500Mapper");
#endif
    return vtkOpenGLVolumeProVG500Mapper::New();
    }
#endif
#ifdef _WIN32
  if (!strcmp("Win32OpenGL",temp))
    {
#ifdef VTK_DEBUG_LEAKS
    vtkDebugLeaks::DestructClass("vtkVolumeProVG500Mapper");
#endif
    return vtkOpenGLVolumeProVG500Mapper::New();
    }
#endif
  
  return new vtkVolumeProVG500Mapper;
}



void vtkVolumeProVG500Mapper::UpdateCamera( vtkRenderer *ren, vtkVolume * vtkNotUsed(vol) )
{
  VLICamera                 *camera;
  VLIMatrix                 *matrixVLI;
  VLIVector3D               *positionVLI;
  VLIVector3D               *focalPointVLI;
  VLIVector3D               *viewUpVLI;
  float                     positionVTK[3];
  float                     focalPointVTK[3];
  float                     viewUpVTK[3];
  VLIStatus                 status;

  matrixVLI = new VLIMatrix();

  // Get the necessary information from the vtk camera
  ren->GetActiveCamera()->GetPosition( positionVTK );
  ren->GetActiveCamera()->GetFocalPoint( focalPointVTK );
  ren->GetActiveCamera()->GetViewUp( viewUpVTK );

  // make sure we are in parallel mode
  if (!ren->GetActiveCamera()->GetParallelProjection())
    {
    vtkWarningMacro("The Volume Pro VG500 does not support perspective projection and the camera is currently not in ParallelProjection mode.");
    }

  // Create the three vectors we need to do the lookat
  positionVLI = new VLIVector3D( positionVTK );
  focalPointVLI = new VLIVector3D( focalPointVTK );
  viewUpVLI = new VLIVector3D( viewUpVTK );

  // Create a camera from this matrix
  camera = new VLICamera();
  status = camera->SetViewMatrix( VLIMatrix::LookAt( *positionVLI, *focalPointVLI, *viewUpVLI ) );

  if ( status != kVLIOK )
    {
    vtkErrorMacro( << "Camera matrix not set!" );
    }

  // Set this as the current camera of the context
  status = this->Context->SetCamera( *camera );
  if ( status != kVLIOK )
    {
    vtkErrorMacro( << "Camera not set!" );
    }


  // Delete everything
  delete positionVLI;
  delete focalPointVLI;
  delete viewUpVLI;
  delete matrixVLI;
  delete camera;

  //  this->Context->SetSuperSamplingSpace( VLICoordinateSpace::kVLICameraSpace );
  
  if ( this->SuperSampling )
    {
    status = this->Context->
      SetSuperSamplingFactor( this->SuperSamplingFactor[0],
			      this->SuperSamplingFactor[1],
			      this->SuperSamplingFactor[2] );
    if ( status != kVLIOK )
      {
      vtkErrorMacro( << "Could not set the supersampling factor!" );
      }
    }
  else
    {
    this->Context->SetSuperSamplingFactor( 1.0, 1.0, 1.0 );
    }
}



void vtkVolumeProVG500Mapper::UpdateLights( vtkRenderer *ren, vtkVolume *vol )
{
  vtkLight     *light;
  float        status;
  int          count;
  int          index;
  float        position[3], focalPoint[3];
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

void vtkVolumeProVG500Mapper::UpdateProperties( vtkRenderer *vtkNotUsed(ren), 
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
      scale = 1.0 / 16.0;
      break;
    case VTK_VOLUME_12BIT_LOWER:
      scale = 1.0;
      break;
    case VTK_VOLUME_12BIT_UPPER:
      scale = 16.0;
    }

  soFunc = vol->GetProperty()->GetScalarOpacity();

  switch ( vol->GetProperty()->GetColorChannels() )
    {
    case 1:
      grayFunc = vol->GetProperty()->GetGrayTransferFunction();
      for ( i= 0; i< 4096; i++)
	{
        val = 0.5 + grayFunc->GetValue((float)(i)*scale)*255.0;
        val = (val < 0)?(0):(val);
        val = (val > 255)?(255):(val);
      	rgbTable[i][0] = rgbTable[i][1] = rgbTable[i][2] = val;
        
        val = 0.5 + 4095.0 * soFunc->GetValue((float)(i)*scale);
        val = (val < 0)?(0):(val);
        val = (val > 4095)?(4095):(val);
  	aTable[i] = val;
	}
      break;
    case 3:
      rgbFunc = vol->GetProperty()->GetRGBTransferFunction();
      for ( i= 0; i< 4096; i++)
	{
        val = 0.5 + rgbFunc->GetRedValue((float)(i)*scale)*255.0;
        val = (val < 0)?(0):(val);
        val = (val > 255)?(255):(val);
  	rgbTable[i][0] = val;

        val = 0.5 + rgbFunc->GetGreenValue((float)(i)*scale)*255.0;
        val = (val < 0)?(0):(val);
        val = (val > 255)?(255):(val);
  	rgbTable[i][1] = val;

        val = 0.5 + rgbFunc->GetBlueValue((float)(i)*scale)*255.0;
        val = (val < 0)?(0):(val);
        val = (val > 255)?(255):(val);
  	rgbTable[i][2] = val;

        val = 0.5 + 4095.0 * soFunc->GetValue((float)(i)*scale);
        val = (val < 0)?(0):(val);
        val = (val > 4095)?(4095):(val);
  	aTable[i] = val;
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
      case VTK_VOLUME_12BIT_UPPER:
        scale = sqrt(3.0)*65536;
      }

    gradientTable = new double [this->GradientTableSize];
    float *spacing = this->GetInput()->GetSpacing();
    float avgSpacing = 0.333*(spacing[0] + spacing[1] + spacing[2]);
    scale = scale/(avgSpacing*(this->GradientTableSize-1));
    
    for ( i = 0; i < this->GradientTableSize; i++ )
      {
      // Take an average of five values in the region
      gradientTable[i] = 0.2 * ( 
        goFunc->GetValue(scale*((float)i - 0.4))  +
        goFunc->GetValue(scale*((float)i-0.2)) +
        goFunc->GetValue(scale*((float)i)) +
        goFunc->GetValue(scale*((float)i+0.2)) +
        goFunc->GetValue(scale*((float)i+0.4)));
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

  this->Context->SetLookupTable(this->LookupTable); 

  // Now the volume matrix. We have to get the composite matrix
  // out of the volume (both the scale, position, and orientation, and
  // the user matrix) then we need to scale by the spacing of the
  // data.


}

void vtkVolumeProVG500Mapper::UpdateCropping( vtkRenderer * vtkNotUsed(ren), vtkVolume * vtkNotUsed(vol) )
{
  VLICrop  *crop;

  crop = new VLICrop;

  crop->SetSlabs( this->CroppingRegionPlanes[0], 
		  this->CroppingRegionPlanes[1],
		  this->CroppingRegionPlanes[2], 
		  this->CroppingRegionPlanes[3],
		  this->CroppingRegionPlanes[4], 
		  this->CroppingRegionPlanes[5] );

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

void vtkVolumeProVG500Mapper::UpdateCutPlane( vtkRenderer * vtkNotUsed(ren), vtkVolume *vtkNotUsed(vol) )
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

void vtkVolumeProVG500Mapper::UpdateCursor( vtkRenderer *vtkNotUsed(ren), vtkVolume *vtkNotUsed(vol) )
{
  VLICursor     *cursor;

  cursor = new VLICursor;

  if ( this->Cursor == 0 )
    {
    cursor->SetAttributes( VLICursor::kDisable );
    }
  else
    {
    cursor->SetAttributes( VLICursor::kEnableAll | VLICursor::kDisableCrop );
    cursor->SetPosition( this->CursorPosition[0], 
			 this->CursorPosition[1],
			 this->CursorPosition[2] );
    
    cursor->SetWidth(1.0);
    
    if ( this->CursorType == VTK_CURSOR_TYPE_CROSSHAIR )
      {
      cursor->SetType( VLICursor::kCrossHair );
      }
    else if ( this->CursorType == VTK_CURSOR_TYPE_PLANE )
      {
      cursor->SetType( VLICursor::kPlane );
      }
    
    cursor->SetColor( VLICursor::kXAxis, 
                      this->CursorXAxisColor[0],
                      this->CursorXAxisColor[1],
                      this->CursorXAxisColor[2] );
    
    cursor->SetColor( VLICursor::kYAxis, 
                      this->CursorYAxisColor[0],
                      this->CursorYAxisColor[1],
                      this->CursorYAxisColor[2] );
    
    cursor->SetColor( VLICursor::kZAxis, 
                      this->CursorZAxisColor[0],
                      this->CursorZAxisColor[1],
                      this->CursorZAxisColor[2] );
                      
    }

  this->Context->SetCursor( *cursor );
  
  delete cursor;
}

void vtkVolumeProVG500Mapper::UpdateVolume( vtkRenderer * vtkNotUsed(ren), vtkVolume * vol )
{
  int                       dataSize[3];
  int			    dataType;
  unsigned char		    *uc_data_ptr;
  unsigned short	    *us_data_ptr;
  void                      *data_ptr;
  vtkImageData              *input = this->GetInput();
  vtkTransform              *volumeTransform;
  vtkTransform              *scalarTransform;
  int                       i, j;
  double                    matrixValues[16];
  VLIMatrix                 *matrixVLI;
  float                     dataOrigin[3];
  float                     dataSpacing[3];
  VLIStatus                 status;
  float                     range[2];


  // We need the size to create the volume and check the subvolume
  input->GetDimensions( dataSize );

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
	  uc_data_ptr = (unsigned char *) data_ptr;
	  this->Volume->UpdateVolume( kVLIVoxelFormatUINT8, uc_data_ptr,
				      0, 0, 0, 
				      dataSize[0], 
				      dataSize[1], 
				      dataSize[2] );
	  volumeUpdated = 1;
	  }

      break;

      case VTK_UNSIGNED_SHORT:
	if ( this->VolumeDataType == VTK_VOLUME_12BIT_UPPER ||
	     this->VolumeDataType == VTK_VOLUME_12BIT_LOWER )
	  {
	  us_data_ptr = (unsigned short *) data_ptr;

	  // If our scalar range is above 4095 (doesn't fit in 12 bits)
	  // then use the upper 12 bits of our 16 bit data, otherwise
	  // use the lower 12 bits.
	  input->GetPointData()->GetScalars()->GetRange( range );
	  if ( range[1] > 4095 )
	    {
	    this->Volume->UpdateVolume(kVLIVoxelFormatUINT12U, 
				       us_data_ptr,
				       0, 0, 0,
				       dataSize[0], 
				       dataSize[1], 
				       dataSize[2]);
	    this->VolumeDataType = VTK_VOLUME_12BIT_UPPER;
	    }
	  else
	    {
	    this->Volume->UpdateVolume(kVLIVoxelFormatUINT12L, 
				       us_data_ptr,
				       0, 0, 0,
				       dataSize[0], 
				       dataSize[1], 
				       dataSize[2]);
	    this->VolumeDataType = VTK_VOLUME_12BIT_LOWER;
	    }
	  volumeUpdated = 1;
	  }
	break;

      default:
	vtkErrorMacro( << "You must convert your data to unsigned char " <<
	                   "or unsigned short for a VolumePro mapper" );
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
	uc_data_ptr = (unsigned char *) data_ptr;
	this->Volume = VLIVolume::Create( kVLIVoxelFormatUINT8, uc_data_ptr,
					  dataSize[0], dataSize[1], 
					  dataSize[2] );

	this->VolumeDataType = VTK_VOLUME_8BIT;

      break;

      case VTK_UNSIGNED_SHORT:
	us_data_ptr = (unsigned short *) data_ptr;

	// If our scalar range is above 4095 (doesn't fit in 12 bits)
	// then use the upper 12 bits of our 16 bit data, otherwise
	// use the lower 12 bits.
	input->GetPointData()->GetScalars()->GetRange( range );
	if ( range[1] > 4095 )
	  {
	  this->Volume = VLIVolume::Create(kVLIVoxelFormatUINT12U, 
					   us_data_ptr,
					   dataSize[0], dataSize[1], 
					   dataSize[2]);
	  this->VolumeDataType = VTK_VOLUME_12BIT_UPPER;
	  }
	else
	  {
	  this->Volume = VLIVolume::Create(kVLIVoxelFormatUINT12L, 
					   us_data_ptr,
					   dataSize[0], dataSize[1], 
					   dataSize[2]);
	  this->VolumeDataType = VTK_VOLUME_12BIT_LOWER;
	  }

	break;

      default:
	vtkErrorMacro( << "You must convert your data to unsigned char " <<
	                   "or unsigned short for a VolumePro mapper" );
	break;
      }

    }

  // Keep the data size for our check next time
  this->LoadedDataSize[0] = dataSize[0];
  this->LoadedDataSize[1] = dataSize[1];
  this->LoadedDataSize[2] = dataSize[2];

  // Store the matrix of the volume in a temporary transformation matrix
  volumeTransform = vtkTransform::New();
  volumeTransform->SetMatrix( vol->vtkProp3D::GetMatrix() );

  // Get the origin of the data.  This translation is not accounted for in
  // the volume's matrix, so we must add it in.
  input->GetOrigin( dataOrigin );

  // Get the data spacing.  This scaling is not accounted for in
  // the volume's matrix, so we must add it in.
  input->GetSpacing( dataSpacing );

  // Create a transform that will account for the scaling and translation of
  // the scalar data
  scalarTransform = vtkTransform::New();
  scalarTransform->Identity();
  scalarTransform->Translate(dataOrigin[0], dataOrigin[1], dataOrigin[2]);
  scalarTransform->Scale( dataSpacing[0], dataSpacing[1], dataSpacing[2] );

  // Now concatenate the volume's matrix with this scalar data matrix
  volumeTransform->PostMultiply();
  volumeTransform->Concatenate( scalarTransform->GetMatrix() );

  // Now copy the matrix out (inverted) into an array of doubles
  for ( j = 0; j < 4; j++ )
    for ( i = 0; i < 4; i++ )
      {
      matrixValues[j*4 + i] = 
	volumeTransform->GetMatrix()->GetElement( i, j );
      }

  // Create the VLIMatrix and set the matrix values, then set this as
  // the model matrix of the volume

  matrixVLI = new VLIMatrix( matrixValues );
  status = this->Volume->SetModelMatrix( *matrixVLI );
  if ( status != kVLIOK )
    {
    vtkErrorMacro( << "Error setting the volume matrix: " << status );
    }

  // Delete the objects we created
  volumeTransform->Delete();
  scalarTransform->Delete();
  delete matrixVLI;

  // Update the subvolume if it is reasonable
  if ( this->SubVolume[0] >= 0 && 
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
    status = 
      this->Volume->SetActiveSubVolumeOrigin( (unsigned int) this->SubVolume[0],
					      (unsigned int) this->SubVolume[2],
					      (unsigned int) this->SubVolume[4] );
    if ( status != kVLIOK )
      {
      vtkErrorMacro( << "Could not set the subvolume origin" );
      }

    status = this->Volume->
      SetActiveSubVolumeSize( (this->SubVolume[1]-this->SubVolume[0]) + 1,
			      (this->SubVolume[3]-this->SubVolume[2]) + 1,
			      (this->SubVolume[5]-this->SubVolume[4]) + 1 );
    if ( status != kVLIOK )
      {
      vtkErrorMacro( << "Could not set the subvolume size" );
      }
    }
}

void vtkVolumeProVG500Mapper::CorrectBasePlaneSize( VLIPixel *basePlane, 
					       int size[2],
					       VLIPixel **newBasePlane,
					       int newSize[2],
					       VLIVector2D textureCoords[6] )
{
  int    i;
  double extent[4];
  int    imageExtent[4];
  double x, y;
  int    requiredSize[2];
  double newX, newY;
  double aspect[2];

  // look for the extent of the texture coordinates
  extent[0] = extent[2] = 1.0;
  extent[1] = extent[3] = 0.0;
  for ( i = 0; i < 6; i++ )
    {
    x = textureCoords[i].X();
    y = textureCoords[i].Y();
    extent[0] = ( x < extent[0] ) ? ( x ) : ( extent[0] );
    extent[1] = ( x > extent[1] ) ? ( x ) : ( extent[1] );
    extent[2] = ( y < extent[2] ) ? ( y ) : ( extent[2] );
    extent[3] = ( y > extent[3] ) ? ( y ) : ( extent[3] );
    }

  // Now compute what this 0-1 float extent means in pixels
  imageExtent[0] = (int)(extent[0] * (float)size[0]);
  imageExtent[1] = (int)(extent[1] * (float)size[0]);
  imageExtent[2] = (int)(extent[2] * (float)size[1]);
  imageExtent[3] = (int)(extent[3] * (float)size[1]);

  // make sure our image extent is within the original extent
  imageExtent[0] = (imageExtent[0]<0)?(0):(imageExtent[0]);
  imageExtent[1] = (imageExtent[1]>(size[0]-1))?(size[0]-1):(imageExtent[1]);
  imageExtent[2] = (imageExtent[2]<0)?(0):(imageExtent[2]);
  imageExtent[3] = (imageExtent[3]>(size[1]-1))?(size[1]-1):(imageExtent[3]);

  // Turn this image extent back into a floating point extent
  extent[0] = (float) imageExtent[0] / (float) (size[0] - 1);
  extent[1] = (float) imageExtent[1] / (float) (size[0] - 1);
  extent[2] = (float) imageExtent[2] / (float) (size[1] - 1);
  extent[3] = (float) imageExtent[3] / (float) (size[1] - 1);

  // How big a texture do we need
  requiredSize[0] = imageExtent[1] - imageExtent[0] + 1;
  requiredSize[1] = imageExtent[3] - imageExtent[2] + 1;

  // What power of two texture does this fit into
  for ( i = 0; i < 2; i++ )
    {
    newSize[i] = 2;
    while ( requiredSize[i] > newSize[i] )
      {
      newSize[i] *= 2;
      }
    }

  // Because of some problems with the memory returned from the
  // volumePro board, keep the full texture if either of the axes has
  // the same resolutions as the full texture
  if ( newSize[0] == size[0] || newSize[1] == size[1] )
    {
    newSize[0] = size[0];
    newSize[1] = size[1];
    }

  // If this is the size we came in with, do nothing
  if ( newSize[0] == size[0] && newSize[1] == size[1] )
    {
    *newBasePlane = basePlane;
    }
  // Otherwise, create the new texture, copy the old into the new, and
  // change the texture coordinates
  else
    {
    *newBasePlane = new VLIPixel[ newSize[0] * newSize[1] * 4 ];

    // Copy the texture into the new (smaller or bigger) space
    for ( i = 0; i < newSize[1]; i++ )
      {
       memcpy( (*newBasePlane + i*newSize[0]), 
	       (basePlane + (imageExtent[2]+i)*size[0] + imageExtent[0]), 
	       requiredSize[0]*4*sizeof(VLIPixel) );
      }

    aspect[0] = (float)size[0] / (float)newSize[0];
    aspect[1] = (float)size[1] / (float)newSize[1];

    // Change the texture coordinates
    for ( i = 0; i < 6; i++ )
      {
      newX = (textureCoords[i].X() - extent[0]) * aspect[0];
      newX = (newX < 0.0)?(0.0):(newX);
      newX = (newX > 1.0)?(1.0):(newX);

      newY = (textureCoords[i].Y() - extent[2]) * aspect[1];
      newY = (newY < 0.0)?(0.0):(newY);
      newY = (newY > 1.0)?(1.0):(newY);

      textureCoords[i].Assign(newX, newY);
      }
    }
}

int vtkVolumeProVG500Mapper::GetAvailableBoardMemory()
{
  int               memory;
  VLIConfiguration  *config;

  config = new VLIConfiguration;
  memory = config->GetAvailableMemory( 0 );
  delete config;
  
  return memory;
}

void vtkVolumeProVG500Mapper::GetLockSizesForBoardMemory( unsigned int type,
						     unsigned int *xSize,
						     unsigned int *ySize,
						     unsigned int *zSize )
{
  VLIConfiguration  *config;

  config = new VLIConfiguration;
  config->GetMaxLockedSize( type, *xSize, *ySize, *zSize );
  delete config;
}

void vtkVolumeProVG500Mapper::Render( vtkRenderer *ren, vtkVolume *vol )
{
  int                       baseWidth, baseHeight, imageWidth, imageHeight;
  VLIPixel                  *basePlane, *newBasePlane;
  VLIVector3D               hexagon[6];
  VLIVector2D               textureCoords[6];
  int                       size[2], newSize[2];
  VLIStatus                 status;

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

  this->UpdateCamera( ren, vol );

  this->UpdateLights( ren, vol);

  this->UpdateVolume( ren, vol );

  this->UpdateProperties( ren, vol );  

  if ( !this->Volume )
    {
    return;
    }

  this->UpdateCropping( ren, vol );

  this->UpdateCutPlane( ren, vol );

  this->UpdateCursor( ren, vol );


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

  status = this->Context->RenderBasePlane(this->Volume, 0, VLIfalse );
  if ( status != kVLIOK && status != kVLIMultiPass )
    {
    switch ( status )
      {
      case kVLIErrArgument:
	vtkErrorMacro( << "Base plane could not be rendered - bad argument!" );
	break;
      case kVLIErrVolumeInvalid:
	vtkErrorMacro( << "Base plane could not be rendered - invalid volume!" );
	break;
      case kVLIErrAlloc:
	vtkErrorMacro( << "Base plane could not be rendered - not enough resources!" );
	break;
      case kVLIErrBasePlaneAllocation:
	vtkErrorMacro( << "Base plane could not be rendered - could not allocate base plane!" );
	break;
      case kVLIErrAccess:
	vtkErrorMacro( << "Base plane could not be rendered - could not access volume!" );
	break;
      default:
        // Don't report the error - this volume just won't render
        // this error is occurring occasionally in vli 2.0 
	//vtkErrorMacro( << "Base plane could not be rendered - unkown error!" );
	break;	  
      }
    
    // Release the base plane for use next time
    this->Context->ReleaseBasePlane( 0 );

    return;
    }

  status = this->Context->FetchBasePlane(0, baseWidth, baseHeight,
					 imageWidth, imageHeight,
					 basePlane, hexagon, textureCoords);
  if ( status != kVLIOK )
    {
    vtkErrorMacro( << "Base plane could not be fetched!" );
    return;
    }

  size[0] = baseWidth;
  size[1] = baseHeight;

  this->CorrectBasePlaneSize( basePlane, size, 
			      &newBasePlane, newSize, textureCoords );
  this->RenderHexagon( ren, vol, newBasePlane, 
		       newSize, hexagon, textureCoords );

  // Release the base plane for use next time
  this->Context->ReleaseBasePlane( 0 );

  // Delete it only if was created - if it is the same as basePlane, then
  // we didn't create a new one since basePlane was already a power of 2
  if ( newBasePlane != basePlane )
    {
    delete [] newBasePlane;
    }

}


