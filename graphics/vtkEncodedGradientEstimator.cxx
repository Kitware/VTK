/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEncodedGradientEstimator.cxx
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
#include <math.h>
#include "vtkEncodedGradientEstimator.h"
#include "vtkRecursiveSphereDirectionEncoder.h"
#include "vtkTimerLog.h"

// Construct a vtkEncodedGradientEstimator with initial values of NULL for
// the ScalarInput, EncodedNormal, and GradientMagnitude. Also,
// indicate that the IndexTable has not yet been initialized. The
// GradientMagnitudeRange and the GradientMangitudeTable are 
// initialized to default values - these will change in the future
// when magnitude of gradient opacities are included
vtkEncodedGradientEstimator::vtkEncodedGradientEstimator()
{
  this->ScalarInput                = NULL;
  this->EncodedNormals             = NULL;
  this->GradientMagnitudes         = NULL;
  this->GradientMagnitudeScale     = 1.0;
  this->GradientMagnitudeBias      = 0.0;
  this->Threader                   = vtkMultiThreader::New();
  this->NumberOfThreads            = this->Threader->GetNumberOfThreads();
  this->DirectionEncoder           = vtkRecursiveSphereDirectionEncoder::New();
  this->ComputeGradientMagnitudes  = 1;
  this->ClipOutsideCircle          = 0;
  this->CircleLimits               = NULL;
  this->CircleLimitsSize           = -1;
  this->UseCircleClip              = 0;
  this->LastUpdateTimeInSeconds    = -1.0;
  this->LastUpdateTimeInCPUSeconds = -1.0;

}

// Destruct a vtkEncodedGradientEstimator - free up any memory used
vtkEncodedGradientEstimator::~vtkEncodedGradientEstimator()
{
  this->SetScalarInput(NULL);
  this->Threader->Delete();
  this->Threader = NULL;
  
  if ( this->EncodedNormals )
    {
    delete [] this->EncodedNormals;
    }

  if ( this->GradientMagnitudes )
    {
    delete [] this->GradientMagnitudes;
    }

  if ( this->DirectionEncoder )
    {
    this->DirectionEncoder->UnRegister( this );
    }
}

void 
vtkEncodedGradientEstimator::SetDirectionEncoder(vtkDirectionEncoder *direnc)
{
  // If we are setting it to its current value, don't do anything
  if ( this->DirectionEncoder == direnc )
    {
    return;
    }

  // If we already have a direction encoder, unregister it.
  if ( this->DirectionEncoder )
    {
    this->DirectionEncoder->UnRegister(this);
    this->DirectionEncoder = NULL;
    }

  // If we are passing in a non-NULL encoder, register it
  if ( direnc )
    {
    direnc->Register( this );
    }

  // Actually set the encoder, and consider the object Modified
  this->DirectionEncoder = direnc;
  this->Modified();
}

int vtkEncodedGradientEstimator::GetEncodedNormalIndex( int xyz_index ) 
{
  this->Update();
  return *(this->EncodedNormals + xyz_index);
}

int vtkEncodedGradientEstimator::GetEncodedNormalIndex( int x_index, 
							int y_index,
							int z_index )
{
  int ystep, zstep;

  this->Update();

  // Compute steps through the volume in x, y, and z
  ystep = this->ScalarInputSize[0];
  zstep = this->ScalarInputSize[0] * this->ScalarInputSize[1];

  return *(this->EncodedNormals + z_index * zstep + y_index * ystep + x_index);
}

unsigned short *vtkEncodedGradientEstimator::GetEncodedNormals()
{
  this->Update();

  return this->EncodedNormals;
}

unsigned char *vtkEncodedGradientEstimator::GetGradientMagnitudes()
{
  this->Update();

  return this->GradientMagnitudes;
}

void vtkEncodedGradientEstimator::Update( )
{
  int                scalar_input_size[3];
  float              scalar_input_aspect[3];
  double             startSeconds, endSeconds;
  double             startCPUSeconds, endCPUSeconds;

  if ( this->GetMTime() > this->BuildTime || 
       this->DirectionEncoder->GetMTime() > this->BuildTime ||
       this->ScalarInput->GetMTime() > this->BuildTime ||
       !this->EncodedNormals )
    {

    startSeconds = vtkTimerLog::GetCurrentTime();
    startCPUSeconds = vtkTimerLog::GetCPUTime();
    
    // Get the dimensions of the data and its aspect ratio
    this->ScalarInput->GetDimensions( scalar_input_size );
    this->ScalarInput->GetSpacing( scalar_input_aspect );
    
    // If we previously have allocated space for the encoded normals,
    // and this space is no longer the right size, delete it
    if ( this->EncodedNormalsSize[0] != scalar_input_size[0] ||
	 this->EncodedNormalsSize[1] != scalar_input_size[1] ||
	 this->EncodedNormalsSize[2] != scalar_input_size[2] )
      {
      if ( this->EncodedNormals )
	{
	delete [] this->EncodedNormals;
	this->EncodedNormals = NULL;
	}
      if ( this->GradientMagnitudes )
	{
	delete [] this->GradientMagnitudes;
	this->GradientMagnitudes = NULL;
	}
      }

    // Allocate space for the encoded normals if necessary
    if ( !this->EncodedNormals )
      {
      this->EncodedNormals = new unsigned short[ scalar_input_size[0] *
					         scalar_input_size[1] *
					         scalar_input_size[2] ];
      this->EncodedNormalsSize[0] = scalar_input_size[0];
      this->EncodedNormalsSize[1] = scalar_input_size[1];
      this->EncodedNormalsSize[2] = scalar_input_size[2];
      }

    if ( !this->GradientMagnitudes && this->ComputeGradientMagnitudes )
      {
      this->GradientMagnitudes = new unsigned char[ scalar_input_size[0] *
				 	            scalar_input_size[1] *
						    scalar_input_size[2] ];
      }

    // Copy info that multi threaded function will need into temp variables
    memcpy( this->ScalarInputSize, scalar_input_size, 3 * sizeof(int) );
    memcpy( this->ScalarInputAspect, scalar_input_aspect, 3 * sizeof(float) );

    if ( this->ClipOutsideCircle && 
	 (this->ScalarInputSize[0] == this->ScalarInputSize[1]) )
      {
      this->UseCircleClip = 1;
      this->ComputeCircleLimits( this->ScalarInputSize[0] );
      }
    else
      {
      this->UseCircleClip = 0;
      }
    this->UpdateNormals();

    this->BuildTime.Modified();

    endSeconds = vtkTimerLog::GetCurrentTime();
    endCPUSeconds = vtkTimerLog::GetCPUTime();
  
    this->LastUpdateTimeInSeconds    = (float)(endSeconds    - startSeconds);
    this->LastUpdateTimeInCPUSeconds = (float)(endCPUSeconds - startCPUSeconds);
    }
}

void vtkEncodedGradientEstimator::ComputeCircleLimits( int size )
{
  int     *ptr, y;
  double  w, halfsize, length, start, end;

  if ( this->CircleLimitsSize != size )
    {
    if ( this->CircleLimits )
      {
      delete this->CircleLimits;
      }
    this->CircleLimits = new int[2*size];
    this->CircleLimitsSize = size;
    }

  ptr = this->CircleLimits;

  halfsize = (double)(size-1)/2.0;

  for ( y = 0; y < size; y++ )
    {
    w = halfsize - (double)y;
    length = (int)( sqrt( (halfsize*halfsize) - (w*w) ) + 0.5 );
    start = halfsize - length - 1;
    end   = halfsize + length + 1;
    start = (start<0)?(0):(start);
    end   = (end>(size-1))?(size-1):(end);

    *(ptr++) = start;
    *(ptr++) = end;
    }
}

// Print the vtkEncodedGradientEstimator
void vtkEncodedGradientEstimator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);

  if ( this->ScalarInput )
    {
    os << indent << "ScalarInput: (" << this->ScalarInput << ")\n";
    }
  else
    {
    os << indent << "ScalarInput: (none)\n";
    }

  if ( this->DirectionEncoder )
    {
    os << indent << "DirectionEncoder: (" << this->DirectionEncoder << ")\n";
    }
  else
    {
    os << indent << "DirectionEncoder: (none)\n";
    }

  os << indent << "Build Time: " 
     << this->BuildTime.GetMTime() << endl;

  os << indent << "Gradient Magnitude Scale: " 
     << this->GradientMagnitudeScale << endl;

  os << indent << "Gradient Magnitude Bias: " 
     << this->GradientMagnitudeBias << endl;

  os << indent << "Compute Gradient Magnitudes: " 
     << ((this->ComputeGradientMagnitudes)?"On":"Off") << endl;

  os << indent << "Clip Outside Circle: " 
     << ((this->ClipOutsideCircle)?"On":"Off") << endl;

  os << indent << "Number Of Threads: " 
     << this->NumberOfThreads << endl;

  os << indent << "Last Update Time In Seconds: " 
     << this->LastUpdateTimeInSeconds;

  os << indent << "Last Update Time In CPU Seconds: " 
     << this->LastUpdateTimeInCPUSeconds;

}
