/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFiniteDifferenceGradientEstimator.cxx
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
#include "vtkFiniteDifferenceGradientEstimator.h"
#include "vtkCharArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkShortArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkLongArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkDoubleArray.h"

// This is the templated function that actually computes the EncodedNormal
// and the GradientMagnitude
template <class T>
static void ComputeGradients( 
         vtkFiniteDifferenceGradientEstimator *estimator, T *data_ptr,
	 int thread_id, int thread_count )
{
  int                 xstep, ystep, zstep;
  int                 x, y, z;
  int                 offset;
  int                 z_start, z_limit;
  T                   *dptr;
  unsigned char       *gptr;
  unsigned short      *nptr;
  float               n[3], t;
  float               gvalue;
  float               normalize_factor;
  vtkDirectionEncoder *direction_encoder;

  // Compute steps through the volume in x, y, and z
  xstep = 1;
  ystep = estimator->ScalarInputSize[0];
  zstep = estimator->ScalarInputSize[0] * estimator->ScalarInputSize[1];

  // Multiply by the spacing used for normal estimation
  xstep *= estimator->SampleSpacingInVoxels;
  ystep *= estimator->SampleSpacingInVoxels;
  zstep *= estimator->SampleSpacingInVoxels;

  // Compute an offset based on the thread_id. The volume will
  // be broken into large slabs (thread_count slabs). For this thread
  // we need to access the correct slab. Also compute the z plane that
  // this slab starts on, and the z limit of this slab (one past the
  // end of the slab)
  z_start = (int)(( (float)thread_id / (float)thread_count ) *
		  estimator->ScalarInputSize[2] );
  offset = z_start * estimator->ScalarInputSize[0] * 
                     estimator->ScalarInputSize[1];
  z_limit = (int)(( (float)(thread_id + 1) / (float)thread_count ) *
		  estimator->ScalarInputSize[2] );

  // Make sure out z_limit didn't get too big - it shouldn't so print an
  // error message if this happens and return
  if ( z_limit > estimator->ScalarInputSize[2] )
    {
    return;
    }


  // Set some pointers
  dptr = data_ptr + offset;
  nptr = estimator->EncodedNormals + offset;
  gptr = estimator->GradientMagnitudes + offset;

  // Normalization factor used for magnitude of gradient so that the
  // magnitude is based on a unit distance normal
  normalize_factor = 1.0 / ( 2.0 * ( estimator->ScalarInputAspect[0] *
				     estimator->ScalarInputAspect[1] * 
				     estimator->ScalarInputAspect[2] ) );

  direction_encoder = estimator->GetDirectionEncoder();

  // Loop through all the data and compute the encoded normal and
  // gradient magnitude for each scalar location
  for ( z = z_start; z < z_limit; z++ )
    for ( y = 0; y < estimator->ScalarInputSize[1]; y++ )
      for ( x = 0; x < estimator->ScalarInputSize[0]; x++ )
	{
	// Use a central difference method if possible,
	// otherwise use a forward or backward difference if
	// we are on the edge

	// Compute the X component
	if ( x >= estimator->SampleSpacingInVoxels && 
	     x < estimator->ScalarInputSize[0] - 
	         estimator->SampleSpacingInVoxels )
	  n[0] = (float)*(dptr-xstep) - (float)*(dptr+xstep); 
	else if ( x == 0 )
	  n[0] = -((float)*(dptr+xstep));
	else
	  n[0] =  ((float)*(dptr-xstep));
	
	// Compute the Y component
	if ( y >= estimator->SampleSpacingInVoxels && 
	     y < estimator->ScalarInputSize[1] - 
	         estimator->SampleSpacingInVoxels )
	  n[1] = (float)*(dptr-ystep) - (float)*(dptr+ystep); 
	else if ( y == 0 )
	  n[1] = -((float)*(dptr+ystep));
	else
	  n[1] =  ((float)*(dptr-ystep));
	
	// Compute the Z component
	if ( z >= estimator->SampleSpacingInVoxels && 
	     z < estimator->ScalarInputSize[2] - 
	         estimator->SampleSpacingInVoxels )
	  n[2] = (float)*(dptr-zstep) - (float)*(dptr+zstep); 
	else if ( z == 0 )
	  n[2] = -((float)*(dptr+zstep));
	else
	  n[2] =  ((float)*(dptr-zstep));

	// Take care of the aspect ratio of the data
	// Scaling in the vtkVolume is isotropic, so this is the
	// only place we have to worry about non-isotropic scaling.
	n[0] *= estimator->ScalarInputAspect[1] * 
	        estimator->ScalarInputAspect[2];
	n[1] *= estimator->ScalarInputAspect[0] * 
	        estimator->ScalarInputAspect[2];
	n[2] *= estimator->ScalarInputAspect[0] * 
	        estimator->ScalarInputAspect[1];
	
	// Compute the gradient magnitude
	t = sqrt( (double)( n[0]*n[0] + 
			    n[1]*n[1] + 
			    n[2]*n[2] ) );
	
	// Encode this into an 8 bit value 
	gvalue = t * normalize_factor * estimator->GradientMagnitudeScale + 
	  estimator->GradientMagnitudeBias;
	  
	if ( gvalue < 0.0 )
	  *gptr = 0;
	else if ( gvalue > 255.0 )
	  *gptr = 255;
	else 
	  *gptr = (unsigned char) gvalue;
	
	// Normalize the gradient direction
	if ( t )
	  {
	  n[0] /= t;
	  n[1] /= t;
	  n[2] /= t;
	  }

	// Convert the gradient direction into an encoded index value
	*nptr = direction_encoder->GetEncodedDirection( n );

	nptr++;
	gptr++;
	dptr++;

	}
}

// Construct a vtkFiniteDifferenceGradientEstimator 
vtkFiniteDifferenceGradientEstimator::vtkFiniteDifferenceGradientEstimator()
{
  this->SampleSpacingInVoxels      = 1;
}

// Destruct a vtkFiniteDifferenceGradientEstimator - free up any memory used
vtkFiniteDifferenceGradientEstimator::~vtkFiniteDifferenceGradientEstimator()
{
}

static VTK_THREAD_RETURN_TYPE vtkSwitchOnDataType( void *arg )
{
  vtkFiniteDifferenceGradientEstimator   *estimator;
  int                                    thread_count;
  int                                    thread_id;
  vtkScalars                             *scalars;

  thread_id = ((ThreadInfoStruct *)(arg))->ThreadID;
  thread_count = ((ThreadInfoStruct *)(arg))->NumberOfThreads;
  estimator = (vtkFiniteDifferenceGradientEstimator *)
    (((ThreadInfoStruct *)(arg))->UserData);
  scalars = estimator->ScalarInput->GetPointData()->GetScalars();

  // Find the data type of the ScalarInput and call the correct 
  // templated function to actually compute the normals and magnitudes

  switch (estimator->GetScalarInput()->GetPointData()->GetScalars()->GetDataType())
    {
    case VTK_CHAR:
      {
      char *ptr = ((vtkCharArray *) scalars->GetData())->GetPointer(0);
      ComputeGradients( estimator, ptr, thread_id, thread_count );
      }
    break;
    case VTK_UNSIGNED_CHAR:
      {
      unsigned char *ptr = ((vtkUnsignedCharArray *) 
			    scalars->GetData())->GetPointer(0);
      ComputeGradients( estimator, ptr, thread_id, thread_count );
      }
    break;
    case VTK_SHORT:
      {
      short *ptr = ((vtkShortArray *) scalars->GetData())->GetPointer(0);
      ComputeGradients( estimator, ptr, thread_id, thread_count );
      }
    break;
    case VTK_UNSIGNED_SHORT:
      {
      unsigned short *ptr = ((vtkUnsignedShortArray *) 
			     scalars->GetData())->GetPointer(0);
      ComputeGradients( estimator, ptr, thread_id, thread_count );
      }
    break;
    case VTK_INT:
      {
      int *ptr = ((vtkIntArray *) scalars->GetData())->GetPointer(0);
      ComputeGradients( estimator, ptr, thread_id, thread_count );
      }
    break;
    case VTK_UNSIGNED_INT:
      {
      unsigned int *ptr = ((vtkUnsignedIntArray *) 
			   scalars->GetData())->GetPointer(0);
      ComputeGradients( estimator, ptr, thread_id, thread_count );
      }
    break;
    case VTK_LONG:
      {
      long *ptr = ((vtkLongArray *) scalars->GetData())->GetPointer(0);
      ComputeGradients( estimator, ptr, thread_id, thread_count );
      }
    break;
    case VTK_UNSIGNED_LONG:
      {
      unsigned long *ptr = ((vtkUnsignedLongArray *) 
			    scalars->GetData())->GetPointer(0);
      ComputeGradients( estimator, ptr, thread_id, thread_count );
      }
    break;
    case VTK_FLOAT:
      {
      float *ptr = ((vtkFloatArray *) scalars->GetData())->GetPointer(0);
      ComputeGradients( estimator, ptr, thread_id, thread_count );
      }
    break;
    case VTK_DOUBLE:
      {
      double *ptr = ((vtkDoubleArray *) scalars->GetData())->GetPointer(0);
      ComputeGradients( estimator, ptr, thread_id, thread_count );
      }
    break;
    default:
      vtkGenericWarningMacro("unable to encode scalar type!");
    }
  
  return VTK_THREAD_RETURN_VALUE;
}


// This method is used to compute the encoded normal and the
// magnitude of the gradient for each voxel location in the 
// ScalarInput.
void vtkFiniteDifferenceGradientEstimator::UpdateNormals( )
{
  vtkDebugMacro( << "Updating Normals!" );
  this->Threader.SetNumberOfThreads( this->NumberOfThreads );
  
  this->Threader.SetSingleMethod( vtkSwitchOnDataType,
				  (vtkObject *)this );
  
  this->Threader.SingleMethodExecute();
}

// Print the vtkFiniteDifferenceGradientEstimator
void vtkFiniteDifferenceGradientEstimator::PrintSelf(ostream& os, 
						     vtkIndent indent)
{
  this->vtkEncodedGradientEstimator::PrintSelf(os, indent);
  
  os << indent << "Sample spacing in voxels: " << 
    this->SampleSpacingInVoxels << endl;
}
