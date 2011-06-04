/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEncodedGradientEstimator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEncodedGradientEstimator.h"

#include "vtkGarbageCollector.h"
#include "vtkImageData.h"
#include "vtkMultiThreader.h"
#include "vtkRecursiveSphereDirectionEncoder.h"
#include "vtkTimerLog.h"

#include <math.h>


vtkCxxSetObjectMacro(vtkEncodedGradientEstimator, Input, vtkImageData );

// Construct a vtkEncodedGradientEstimator with initial values of NULL for
// the Input, EncodedNormal, and GradientMagnitude. Also,
// indicate that the IndexTable has not yet been initialized. The
// GradientMagnitudeRange and the GradientMangitudeTable are 
// initialized to default values - these will change in the future
// when magnitude of gradient opacities are included
vtkEncodedGradientEstimator::vtkEncodedGradientEstimator()
{
  this->Input                      = NULL;
  this->EncodedNormals             = NULL;
  this->EncodedNormalsSize[0]      = 0;
  this->EncodedNormalsSize[1]      = 0;
  this->EncodedNormalsSize[2]      = 0;
  this->GradientMagnitudes         = NULL;
  this->GradientMagnitudeScale     = 1.0;
  this->GradientMagnitudeBias      = 0.0;
  this->Threader                   = vtkMultiThreader::New();
  this->NumberOfThreads            = this->Threader->GetNumberOfThreads();
  this->DirectionEncoder           = vtkRecursiveSphereDirectionEncoder::New();
  this->ComputeGradientMagnitudes  = 1;
  this->CylinderClip               = 0;
  this->CircleLimits               = NULL;
  this->CircleLimitsSize           = -1;
  this->UseCylinderClip            = 0;
  this->LastUpdateTimeInSeconds    = -1.0;
  this->LastUpdateTimeInCPUSeconds = -1.0;
  this->ZeroNormalThreshold        = 0.0;
  this->ZeroPad                    = 1;
  this->BoundsClip                 = 0;
  this->Bounds[0] = 
    this->Bounds[1] = 
    this->Bounds[2] = 
    this->Bounds[3] =
    this->Bounds[4] =
    this->Bounds[5] = 0;
  
}

// Destruct a vtkEncodedGradientEstimator - free up any memory used
vtkEncodedGradientEstimator::~vtkEncodedGradientEstimator()
{
  this->SetInput(NULL);
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

  if ( this->CircleLimits )
    {
    delete [] this->CircleLimits;
    }
}

void vtkEncodedGradientEstimator::SetZeroNormalThreshold( float v )
{
  if ( this->ZeroNormalThreshold != v )
    {
    if ( v < 0.0 )
      {
      vtkErrorMacro( << "The ZeroNormalThreshold must be a value >= 0.0" );
      return;
      }

    this->ZeroNormalThreshold = v;
    this->Modified();
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

int vtkEncodedGradientEstimator::GetEncodedNormalIndex( vtkIdType xyzIndex ) 
{
  this->Update();
  return *(this->EncodedNormals + xyzIndex);
}

int vtkEncodedGradientEstimator::GetEncodedNormalIndex( int xIndex, 
                                                        int yIndex,
                                                        int zIndex )
{
  vtkIdType ystep, zstep;

  this->Update();

  // Compute steps through the volume in x, y, and z
  ystep = this->InputSize[0];
  zstep = ystep * this->InputSize[1];

  return *(this->EncodedNormals + zIndex * zstep + yIndex * ystep + xIndex);
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
  int                scalarInputSize[3];
  double             scalarInputAspect[3];
  double             startSeconds, endSeconds;
  double             startCPUSeconds, endCPUSeconds;

  if ( !this->Input )
    {
    vtkErrorMacro(<< "No input in gradient estimator.");
    return;
    }
    
  if ( this->GetMTime() > this->BuildTime || 
       this->DirectionEncoder->GetMTime() > this->BuildTime ||
       this->Input->GetMTime() > this->BuildTime ||
       !this->EncodedNormals )
    {
    this->Input->UpdateInformation();
    this->Input->SetUpdateExtentToWholeExtent();
    this->Input->Update();
    
    startSeconds = vtkTimerLog::GetUniversalTime();
    startCPUSeconds = vtkTimerLog::GetCPUTime();
    
    // Get the dimensions of the data and its aspect ratio
    this->Input->GetDimensions( scalarInputSize );
    this->Input->GetSpacing( scalarInputAspect );
    
    // If we previously have allocated space for the encoded normals,
    // and this space is no longer the right size, delete it
    if ( this->EncodedNormalsSize[0] != scalarInputSize[0] ||
         this->EncodedNormalsSize[1] != scalarInputSize[1] ||
         this->EncodedNormalsSize[2] != scalarInputSize[2] )
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

    // Compute the number of encoded voxels
    vtkIdType encodedSize = scalarInputSize[0];
    encodedSize *= scalarInputSize[1];
    encodedSize *= scalarInputSize[2];

    // Allocate space for the encoded normals if necessary
    if ( !this->EncodedNormals )
      {
      this->EncodedNormals = new unsigned short[ encodedSize ];
      this->EncodedNormalsSize[0] = scalarInputSize[0];
      this->EncodedNormalsSize[1] = scalarInputSize[1];
      this->EncodedNormalsSize[2] = scalarInputSize[2];
      }

    if ( !this->GradientMagnitudes && this->ComputeGradientMagnitudes )
      {
      this->GradientMagnitudes = new unsigned char[ encodedSize ];
      }

    // Copy info that multi threaded function will need into temp variables
    memcpy( this->InputSize, scalarInputSize, 3 * sizeof(int) );
    // TODO cleanup when double changes are further along
    this->InputAspect[0] = static_cast<float>(scalarInputAspect[0]);
    this->InputAspect[1] = static_cast<float>(scalarInputAspect[1]);
    this->InputAspect[2] = static_cast<float>(scalarInputAspect[2]);
    // memcpy( this->InputAspect, scalarInputAspect, 3 * sizeof(float) );

    if ( this->CylinderClip && 
         (this->InputSize[0] == this->InputSize[1]) )
      {
      this->UseCylinderClip = 1;
      this->ComputeCircleLimits( this->InputSize[0] );
      }
    else
      {
      this->UseCylinderClip = 0;
      }
    this->UpdateNormals();

    this->BuildTime.Modified();

    endSeconds = vtkTimerLog::GetUniversalTime();
    endCPUSeconds = vtkTimerLog::GetCPUTime();
  
    this->LastUpdateTimeInSeconds    = static_cast<float>(endSeconds    - startSeconds);
    this->LastUpdateTimeInCPUSeconds = static_cast<float>(endCPUSeconds - startCPUSeconds);
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
      delete [] this->CircleLimits;
      }
    this->CircleLimits = new int[2*size];
    this->CircleLimitsSize = size;
    }

  ptr = this->CircleLimits;

  halfsize = (size-1)/2.0;

  for ( y = 0; y < size; y++ )
    {
    w = halfsize - y;
    length = static_cast<int>( sqrt( (halfsize*halfsize) - (w*w) ) + 0.5 );
    start = halfsize - length - 1;
    end   = halfsize + length + 1;
    start = (start<0)?(0):(start);
    end   = (end>(size-1))?(size-1):(end);

    *(ptr++) = static_cast<int>(start);
    *(ptr++) = static_cast<int>(end);
    }
}

// Print the vtkEncodedGradientEstimator
void vtkEncodedGradientEstimator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if ( this->Input )
    {
    os << indent << "Input: (" << this->Input << ")\n";
    }
  else
    {
    os << indent << "Input: (none)\n";
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

  os << indent << "Zero Pad: " 
     << ((this->ZeroPad)?"On":"Off") << endl;
  
  os << indent << "Bounds Clip: " 
     << ((this->BoundsClip)?"On":"Off") << endl;
  
  os << indent << "Bounds: ("
     << this->Bounds[0] << ", " << this->Bounds[1] << ", " 
     << this->Bounds[2] << ", " << this->Bounds[3] << ", " 
     << this->Bounds[4] << ", " << this->Bounds[5] << ")\n";

  os << indent << "Zero Normal Threshold: " 
     << this->ZeroNormalThreshold << endl;
    
  os << indent << "Compute Gradient Magnitudes: " 
     << ((this->ComputeGradientMagnitudes)?"On":"Off") << endl;

  os << indent << "Cylinder Clip: " 
     << ((this->CylinderClip)?"On":"Off") << endl;

  os << indent << "Number Of Threads: " 
     << this->NumberOfThreads << endl;

  os << indent << "Last Update Time In Seconds: " 
     << this->LastUpdateTimeInSeconds << endl;

  os << indent << "Last Update Time In CPU Seconds: " 
     << this->LastUpdateTimeInCPUSeconds << endl;

  // I don't want to print out these variables - they are
  // internal and the get methods are included only for access
  // within the threaded function
  // os << indent << "Use Cylinder Clip: " 
  //    << this->UseCylinderClip << endl;
  // os << indent << " Input Size: " 
  //    << this->InputSize << endl;
  // os << indent << " Input Aspect Clip: " 
  //    << this->InputAspect << endl;
  
}

//----------------------------------------------------------------------------
void
vtkEncodedGradientEstimator::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->Input, "Input");
}
