/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolume.cxx
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
#include <stdlib.h>
#include <math.h>

#include "vtkVolume.h"
#include "vtkVolumeMapper.h"

// Creates a Volume with the following defaults: origin(0,0,0) 
// position=(0,0,0) scale=1 visibility=1 pickable=1 dragable=1
// orientation=(0,0,0).
vtkVolume::vtkVolume()
{
  this->Scale		= 1.0;
  this->VolumeMapper	= NULL;
  this->VolumeProperty	= NULL;
  this->ScalarOpacityArray          = NULL;
  this->RGBArray                    = NULL;
  this->GrayArray                   = NULL;
  this->CorrectedScalarOpacityArray = NULL;
  this->CorrectedStepSize             = -1;

}

// Destruct a volume
vtkVolume::~vtkVolume()
{
  if (this->VolumeProperty )
    {
    this->VolumeProperty->UnRegister(this);
    }

  this->SetVolumeMapper(NULL);

  if ( this->ScalarOpacityArray )
    {
    delete [] this->ScalarOpacityArray;
    }

  if ( this->RGBArray )
    {
    delete [] this->RGBArray;
    }

  if ( this->GrayArray )
    {
    delete [] this->GrayArray;
    }

  if ( this->CorrectedScalarOpacityArray )
    {
    delete [] this->CorrectedScalarOpacityArray;
    }

}

// Shallow copy of an volume.
vtkVolume& vtkVolume::operator=(const vtkVolume& volume)
{

  this->UserMatrix = volume.UserMatrix;
  
  this->VolumeMapper = volume.VolumeMapper;

  *((vtkProp *)this) = volume;
  
  this->Scale = volume.Scale;
  
  this->VolumeProperty = volume.VolumeProperty;
  
  return *this;
}

void vtkVolume::SetVolumeMapper(vtkVolumeMapper *mapper)
{
  if (this->VolumeMapper != mapper)
    {
    if (this->VolumeMapper != NULL) {this->VolumeMapper->UnRegister(this);}
    this->VolumeMapper = mapper;
    if (this->VolumeMapper != NULL) {this->VolumeMapper->Register(this);}
    this->Modified();
    }
}

// Copy the volume's composite 4x4 matrix into the matrix provided.
void vtkVolume::GetMatrix(vtkMatrix4x4 *result)
{
  // check whether or not need to rebuild the matrix
  if ( this->GetMTime() > this->MatrixMTime )
    {
    this->GetOrientation();
    this->Transform->Push();  
    this->Transform->Identity();  
    this->Transform->PreMultiply();  

    // apply user defined matrix last if there is one 
    if (this->UserMatrix)
      {
      this->Transform->Concatenate(this->UserMatrix);
      }

    // first translate
    this->Transform->Translate(this->Position[0],
			      this->Position[1],
			      this->Position[2]);
   
    // shift to origin
    this->Transform->Translate(this->Origin[0],
			      this->Origin[1],
			      this->Origin[2]);
   
    // rotate
    this->Transform->RotateZ(this->Orientation[2]);
    this->Transform->RotateX(this->Orientation[0]);
    this->Transform->RotateY(this->Orientation[1]);

    // scale
    this->Transform->Scale(this->Scale,
			  this->Scale,
			  this->Scale);

    // shift back from origin
    this->Transform->Translate(-this->Origin[0],
			      -this->Origin[1],
			      -this->Origin[2]);

    this->Matrix->DeepCopy(this->Transform->GetMatrixPointer());
    this->MatrixMTime.Modified();
    this->Transform->Pop();  
    }
  result->DeepCopy(this->Matrix);
} 

// Get the bounds for this Volume as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
float *vtkVolume::GetBounds()
{
  int i,n;
  float *bounds, bbox[24], *fptr;
  float *result;
  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  
  // get the bounds of the Mapper if we have one
  if (!this->VolumeMapper)
    {
    matrix->Delete();
    return this->Bounds;
    }

  bounds = this->VolumeMapper->GetBounds();

  // fill out vertices of a bounding box
  bbox[ 0] = bounds[1]; bbox[ 1] = bounds[3]; bbox[ 2] = bounds[5];
  bbox[ 3] = bounds[1]; bbox[ 4] = bounds[2]; bbox[ 5] = bounds[5];
  bbox[ 6] = bounds[0]; bbox[ 7] = bounds[2]; bbox[ 8] = bounds[5];
  bbox[ 9] = bounds[0]; bbox[10] = bounds[3]; bbox[11] = bounds[5];
  bbox[12] = bounds[1]; bbox[13] = bounds[3]; bbox[14] = bounds[4];
  bbox[15] = bounds[1]; bbox[16] = bounds[2]; bbox[17] = bounds[4];
  bbox[18] = bounds[0]; bbox[19] = bounds[2]; bbox[20] = bounds[4];
  bbox[21] = bounds[0]; bbox[22] = bounds[3]; bbox[23] = bounds[4];
  
  // save the old transform
  this->GetMatrix(matrix);
  this->Transform->Push();
  this->Transform->PostMultiply();
  this->Transform->Identity();
  this->Transform->Concatenate(matrix);

  // and transform into actors coordinates
  fptr = bbox;
  for (n = 0; n < 8; n++) 
    {
    this->Transform->SetPoint(fptr[0],fptr[1],fptr[2],1.0);
  
    // now store the result
    result = this->Transform->GetPoint();
    fptr[0] = result[0] / result[3];
    fptr[1] = result[1] / result[3];
    fptr[2] = result[2] / result[3];
    fptr += 3;
    }
  
  this->Transform->PreMultiply();
  this->Transform->Pop();  
  
  // now calc the new bounds
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_LARGE_FLOAT;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;
  for (i = 0; i < 8; i++)
    {
    for (n = 0; n < 3; n++)
      {
      if (bbox[i*3+n] < this->Bounds[n*2])
	{
	this->Bounds[n*2] = bbox[i*3+n];
	}
      if (bbox[i*3+n] > this->Bounds[n*2+1])
	{
	this->Bounds[n*2+1] = bbox[i*3+n];
	}
      }
    }

  matrix->Delete();
  return this->Bounds;
}

// Get the minimum X bound
float vtkVolume::GetMinXBound( )
{
  this->GetBounds();
  return this->Bounds[0];
}

// Get the maximum X bound
float vtkVolume::GetMaxXBound( )
{
  this->GetBounds();
  return this->Bounds[1];
}

// Get the minimum Y bound
float vtkVolume::GetMinYBound( )
{
  this->GetBounds();
  return this->Bounds[2];
}

// Get the maximum Y bound
float vtkVolume::GetMaxYBound( )
{
  this->GetBounds();
  return this->Bounds[3];
}

// Get the minimum Z bound
float vtkVolume::GetMinZBound( )
{
  this->GetBounds();
  return this->Bounds[4];
}

// Get the maximum Z bound
float vtkVolume::GetMaxZBound( )
{
  this->GetBounds();
  return this->Bounds[5];
}

void vtkVolume::Render( vtkRenderer *ren )
{
  if ( !this->VolumeMapper )
    {
    return;
    }

  if( !this->VolumeProperty )
    {
    // Force the creation of a property
    this->GetVolumeProperty();
    }
}

void vtkVolume::ReleaseGraphicsResources(vtkRenderWindow *renWin)
{
  // pass this information onto the mapper
  if (this->VolumeMapper)
    {
    this->VolumeMapper->ReleaseGraphicsResources(renWin);
    }
}

void vtkVolume::Update()
{
  if ( this->VolumeMapper )
    {
    this->VolumeMapper->Update();
    }
}

void vtkVolume::SetVolumeProperty(vtkVolumeProperty *property)
{
  if( this->VolumeProperty != property )
    {
    if (this->VolumeProperty != NULL) {this->VolumeProperty->UnRegister(this);}
    this->VolumeProperty = property;
    if (this->VolumeProperty != NULL) 
      {
      this->VolumeProperty->Register(this);
      this->VolumeProperty->UpdateMTimes();
      }
    this->Modified();
    }
}

vtkVolumeProperty *vtkVolume::GetVolumeProperty()
{
  if( this->VolumeProperty == NULL )
    {
    this->VolumeProperty = vtkVolumeProperty::New();
    this->VolumeProperty->Register(this);
    this->VolumeProperty->Delete();
    }
  return this->VolumeProperty;
}

unsigned long int vtkVolume::GetMTime()
{
  unsigned long mTime=this->vtkObject::GetMTime();
  unsigned long time;

  if ( this->VolumeProperty != NULL )
    {
    time = this->VolumeProperty->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  if ( this->UserMatrix != NULL )
    {
    time = this->UserMatrix->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

void vtkVolume::UpdateTransferFunctions( vtkRenderer *ren )
{
  int                       data_type;
  vtkPiecewiseFunction      *scalar_opacity_transfer_function;
  vtkPiecewiseFunction      *gradient_opacity_transfer_function;
  vtkPiecewiseFunction      *gray_transfer_function;
  vtkColorTransferFunction  *rgb_transfer_function;
  int                       color_channels;
  int                       scalar_opacity_tf_needs_updating = 0;
  int                       gradient_opacity_tf_needs_updating = 0;
  int                       rgb_tf_needs_updating = 0;
  int                       gray_tf_needs_updating = 0;

  // Update the ScalarOpacityArray if necessary.  This is necessary if
  // the ScalarOpacityArray does not exist, or the transfer function has
  // been modified more recently than the ScalarOpacityArray has.
  scalar_opacity_transfer_function   = this->VolumeProperty->GetScalarOpacity();
  gradient_opacity_transfer_function = this->VolumeProperty->GetGradientOpacity();
  rgb_transfer_function              = this->VolumeProperty->GetRGBTransferFunction();
  gray_transfer_function             = this->VolumeProperty->GetGrayTransferFunction();
  color_channels                     = this->VolumeProperty->GetColorChannels();

  if ( this->VolumeMapper->GetScalarInput()->GetPointData()->GetScalars() == NULL )
    {
    vtkErrorMacro(<<"Need scalar data to volume render");
    return;
    }
    
  data_type = this->VolumeMapper->GetScalarInput()->GetPointData()->GetScalars()->GetDataType();

  if ( scalar_opacity_transfer_function == NULL )
    {
    vtkErrorMacro( << "Error: no transfer function!" );
    return;
    }
  else if ( this->ScalarOpacityArray == NULL ||
	    scalar_opacity_transfer_function->GetMTime() >
	    this->ScalarOpacityArrayMTime ||
	    this->VolumeProperty->GetScalarOpacityMTime() >
	    this->ScalarOpacityArrayMTime )
    {
    scalar_opacity_tf_needs_updating = 1;
    }

  if ( gradient_opacity_transfer_function == NULL )
    {
    vtkErrorMacro( << "Error: no gradient magnitude opacity function!" );
    return;
    }
  else if ( gradient_opacity_transfer_function->GetMTime() >
	    this->GradientOpacityArrayMTime ||
	    this->VolumeProperty->GetGradientOpacityMTime() >
	    this->GradientOpacityArrayMTime )
    {
    gradient_opacity_tf_needs_updating = 1;
    }
  
  switch ( color_channels )
    {
    case 1:
      if ( gray_transfer_function == NULL )
	{
	vtkErrorMacro( << "Error: no gray transfer function!" );
	}
      else if ( this->GrayArray == NULL ||
		gray_transfer_function->GetMTime() >
		this->GrayArrayMTime ||
		this->VolumeProperty->GetGrayTransferFunctionMTime() >
		this->GrayArrayMTime )
	{
	gray_tf_needs_updating = 1;
	}
      break;
    case 3:
      if ( rgb_transfer_function == NULL )
	{
	vtkErrorMacro( << "Error: no color transfer function!" );
	}
      else if ( this->RGBArray == NULL ||
		rgb_transfer_function->GetMTime() >
		this->RGBArrayMTime ||
		this->VolumeProperty->GetRGBTransferFunctionMTime() >
		this->RGBArrayMTime )
	{
	rgb_tf_needs_updating = 1;
	}
      break;
    }

  if ( gradient_opacity_tf_needs_updating )
    {
    // Get values 0-255 (256 values)
    gradient_opacity_transfer_function->GetTable(
					  (float)(0x00),
					  (float)(0xff),  
					  (int)(0x100), 
					  this->GradientOpacityArray );
    if ( !strcmp(gradient_opacity_transfer_function->GetType(), "Constant") )
      {
      this->GradientOpacityConstant = this->GradientOpacityArray[128];
      }
    else
      {
      this->GradientOpacityConstant = -1.0;
      }

    this->GradientOpacityArrayMTime.Modified();
    }


  if (data_type == VTK_UNSIGNED_CHAR)
    {
    this->ArraySize = (int)(0x100);

    if ( scalar_opacity_tf_needs_updating )
      {
      // Get values 0-255 (256 values)
      if ( this->ScalarOpacityArray )
	{
	delete [] this->ScalarOpacityArray;
	}

      this->ScalarOpacityArray = new float[(int)(0x100)];
      scalar_opacity_transfer_function->GetTable( (float)(0x00),
					   (float)(0xff),  
					   (int)(0x100), 
					   this->ScalarOpacityArray );
      this->ScalarOpacityArrayMTime.Modified();
      }

    if ( gray_tf_needs_updating )
      {
      if ( this->GrayArray )
	{
	delete [] this->GrayArray;
	}

      this->GrayArray = new float[(int)(0x100)];
      gray_transfer_function->GetTable( (float)(0x00),
					(float)(0xff),  
					(int)(0x100), 
					this->GrayArray );
      this->GrayArrayMTime.Modified();
      }

    if ( rgb_tf_needs_updating )
      {
      if ( this->RGBArray )
	{
	delete [] this->RGBArray;
	}
      
      this->RGBArray = new float[3 * (int)(0x100)];
      rgb_transfer_function->GetTable( (float)(0x00),
				       (float)(0xff),  
				       (int)(0x100), 
				       this->RGBArray );
      this->RGBArrayMTime.Modified();
      }
    }
  else if ( data_type == VTK_UNSIGNED_SHORT )
    {
    this->ArraySize = (int)(0x10000);

    if ( scalar_opacity_tf_needs_updating )
      {
      // Get values 0-65535 (65536 values)
      if ( this->ScalarOpacityArray )
	{
	delete [] this->ScalarOpacityArray;
	}

      this->ScalarOpacityArray = new float[(int)(0x10000)];
      scalar_opacity_transfer_function->GetTable( (float)(0x0000),
					   (float)(0xffff),  
					   (int)(0x10000), 
					   this->ScalarOpacityArray );
      this->ScalarOpacityArrayMTime.Modified();
      }

    if ( gray_tf_needs_updating )
      {
      if ( this->GrayArray )
	{
	delete [] this->GrayArray;
	}

      this->GrayArray = new float[(int)(0x10000)];
      gray_transfer_function->GetTable( (float)(0x0000),
					(float)(0xffff),  
					(int)(0x10000), 
					this->GrayArray );
      this->GrayArrayMTime.Modified();
      }

    if ( rgb_tf_needs_updating )
      {
      if ( this->RGBArray )
	{
	delete [] this->RGBArray;
	}
      
      this->RGBArray = new float[3 * (int)(0x10000)];
      rgb_transfer_function->GetTable( (float)(0x0000),
				       (float)(0xffff),  
				       (int)(0x10000), 
				       this->RGBArray );
      this->RGBArrayMTime.Modified();
      }
    }

  // check that the corrected scalar opacity transfer function
  // is update to date with the current step size.
  // Update CorrectedScalarOpacityArray if it is required.

  if ( scalar_opacity_tf_needs_updating )
    {
    if ( this->CorrectedScalarOpacityArray )
      {
      delete [] this->CorrectedScalarOpacityArray;
      }

    this->CorrectedScalarOpacityArray = new float[this->ArraySize];
    }

}

// This method computes the corrected alpha blending for a given
// step size.  The ScalarOpacityArray reflects step size 1.
// The CorrectedScalarOpacityArray reflects step size CorrectedStepSize.
void vtkVolume::UpdateScalarOpacityforSampleSize( vtkRenderer *ren, float sample_distance )
{
  int i;
  int needsRecomputing;
  float originalAlpha,correctedAlpha;
  float ray_scale;
  float volumeScale;
  float interactionScale;

  volumeScale = this->Scale;
  ray_scale = sample_distance * volumeScale;


  // step size changed
  needsRecomputing =  
      this->CorrectedStepSize-ray_scale >  0.0001;
  
  needsRecomputing = needsRecomputing || 
      this->CorrectedStepSize-ray_scale < -0.0001;

  if (!needsRecomputing)
    {
    // updated scalar opacity xfer function
    needsRecomputing = needsRecomputing || 
	this->ScalarOpacityArrayMTime > this->CorrectedScalarOpacityArrayMTime;
    }
  if (needsRecomputing)
    {
    this->CorrectedScalarOpacityArrayMTime.Modified();
    this->CorrectedStepSize = ray_scale;
    for (i = 0;i < this->ArraySize;i++)
      {
      originalAlpha = *(this->ScalarOpacityArray+i);

      // this test is to accelerate the Transfer function correction

      if (originalAlpha > 0.0001)
	{
	correctedAlpha = 
	  1.0-pow((double)(1.0-originalAlpha),double(this->CorrectedStepSize));
	}
      else
	{
	correctedAlpha = originalAlpha;
	}
      *(this->CorrectedScalarOpacityArray+i) = correctedAlpha;
      }
    }
}


void vtkVolume::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkProp::PrintSelf(os,indent);

  if( this->VolumeProperty )
    {
    os << indent << "Volume Property:\n";
    this->VolumeProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Volume Property: (not defined)\n";
    }

  // make sure our bounds are up to date
  if ( this->VolumeMapper )
    {
      this->GetBounds();
      os << indent << "Bounds: (" << this->Bounds[0] << ", " 
	 << this->Bounds[1] << ") (" << this->Bounds[2] << ") ("
	 << this->Bounds[3] << ") (" << this->Bounds[4] << ") ("
	 << this->Bounds[5] << ")\n";
    }
  else
    {
    os << indent << "Bounds: (not defined)\n";
    }
  os << indent << "Scale: (" << this->Scale << ")\n";
}

