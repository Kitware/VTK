/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMIPVolumeRayCaster.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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

#include "vtkMIPVolumeRayCaster.h"
#include "vtkCamera.h"
#include "vtkRenderWindow.h"
#include "vtkUnsignedCharScalars.h"
#include "vtkUnsignedShortScalars.h"
#include "vtkShortScalars.h"
#include "vtkIntScalars.h"
#include "vtkFloatScalars.h"
#include "vtkColorTransferFunction.h"

// Description:
// This is the templated function that actually casts a ray and computes
// the maximum value.  It is valid for unsigned char, unsigned short,
// short, int and float data.
template <class T>
static void CastMIPRay( vtkMIPVolumeRayCaster *mapper, T *data_ptr,
		 float ray_start[3], float ray_increment[3],
		 int num_steps, float pixel_value[6] )
{
  float     max;
  float     max_value;
  float     value;
  int       loop;
  int       xinc, yinc, zinc;
  int       voxel[3];
  int       prev_voxel[3];
  float     ray_position[3];
  float     t00, t01, t10, t11, t0, t1;
  T         A, B, C, D, E, F, G, H;
  int       Binc, Cinc, Dinc, Einc, Finc, Ginc, Hinc;
  float     xoff, yoff, zoff;
  float     *color;
  T         *dptr;
  int       steps_this_ray = 0;
  float     max_limit;

  if( num_steps == 0 )
    {
    pixel_value[0] = 0.0;
    pixel_value[1] = 0.0;
    pixel_value[2] = 0.0;
    pixel_value[3] = 0.0;
    pixel_value[4] = 0.0;
    pixel_value[5] = 0.0;
    return;
    }

  // Set the max value.  This will not always be correct and should be fixed
  max = -9.99e10;

  // If our max value is at the limit, we can stop looking for a max 
  // This is that limit.
  max_limit = mapper->LinearRampRange[1];

  // Move the increments into local variables
  xinc = mapper->DataIncrement[0];
  yinc = mapper->DataIncrement[1];
  zinc = mapper->DataIncrement[2];

  // Initialize the ray position and voxel location
  ray_position[0] = ray_start[0];
  ray_position[1] = ray_start[1];
  ray_position[2] = ray_start[2];

  // If we have nearest neighbor interpolation
  if ( mapper->InterpolationType == 0 )
    {

    voxel[0] = vtkRoundFuncMacro( ray_position[0] );
    voxel[1] = vtkRoundFuncMacro( ray_position[1] );
    voxel[2] = vtkRoundFuncMacro( ray_position[2] );

    // For each step along the ray
    for ( loop = 0; loop < num_steps; loop++ )
      {	    
      // We've taken another step
      steps_this_ray++;
      
      // Access the value at this voxel location
      value = *(data_ptr + voxel[2] * zinc +
		voxel[1] * yinc + voxel[0] );

      // If this is greater than the max, this is the new max.
      if ( (float) value > max ) max = (float) value;
      
      // If this is the largest possible value we could encounter, stop.
      if ( max >= max_limit )
	break;
      
      // Increment our position and compute our voxel location
      ray_position[0] += ray_increment[0];
      ray_position[1] += ray_increment[1];
      ray_position[2] += ray_increment[2];
      voxel[0] = vtkRoundFuncMacro( ray_position[0] );
      voxel[1] = vtkRoundFuncMacro( ray_position[1] );
      voxel[2] = vtkRoundFuncMacro( ray_position[2] );
      }
    }
  // We are using trilinear interpolation
  else if ( mapper->InterpolationType == 1 )
    {
    voxel[0] = (int)( ray_position[0] );
    voxel[1] = (int)( ray_position[1] );
    voxel[2] = (int)( ray_position[2] );

    // Compute the increments to get to the other 7 voxel vertices from A
    Binc = xinc;
    Cinc = yinc;
    Dinc = xinc + yinc;
    Einc = zinc;
    Finc = zinc + xinc;
    Ginc = zinc + yinc;
    Hinc = zinc + xinc + yinc;
  
    // Set values for the first pass through the loop
    dptr = data_ptr + voxel[2] * zinc + voxel[1] * yinc + voxel[0];
    A = *(dptr);
    B = *(dptr + Binc);
    C = *(dptr + Cinc);
    D = *(dptr + Dinc);
    E = *(dptr + Einc);
    F = *(dptr + Finc);
    G = *(dptr + Ginc);
    H = *(dptr + Hinc);

    // Keep the voxel location so that we know when we've moved into a
    // new voxel
    prev_voxel[0] = voxel[0];
    prev_voxel[1] = voxel[1];
    prev_voxel[2] = voxel[2];

    // For each step along the ray
    for ( loop = 0; loop < num_steps; loop++ )
      {	    
      // We've taken another step
      steps_this_ray++;

      // Have we moved into a new voxel? If so we need to recompute A-H
      if ( prev_voxel[0] != voxel[0] ||
	   prev_voxel[1] != voxel[1] ||
	   prev_voxel[2] != voxel[2] )
	{
	dptr = data_ptr + voxel[2] * zinc + voxel[1] * yinc + voxel[0];

	A = *(dptr);
	B = *(dptr + Binc);
	C = *(dptr + Cinc);
	D = *(dptr + Dinc);
	E = *(dptr + Einc);
	F = *(dptr + Finc);
	G = *(dptr + Ginc);
	H = *(dptr + Hinc);

	prev_voxel[0] = voxel[0];
	prev_voxel[1] = voxel[1];
	prev_voxel[2] = voxel[2];
	}

      // Compute our offset in the voxel, and use that to trilinearly
      // interpolate a value
      xoff = ray_position[0] - (float) voxel[0];
      yoff = ray_position[1] - (float) voxel[1];
      zoff = ray_position[2] - (float) voxel[2];
      vtkTrilinFuncMacro( value, xoff, yoff, zoff, A, B, C, D, E, F, G, H );

      // If this value is greater than max, it is the new max
      if ( value > max ) max = value;

      // If this value is the largest value we could possibly find, stop.
      if ( max >= max_limit )
	break;

      // Increment our position and compute our voxel location
      ray_position[0] += ray_increment[0];
      ray_position[1] += ray_increment[1];
      ray_position[2] += ray_increment[2];      
      voxel[0] = (int)( ray_position[0] );
      voxel[1] = (int)( ray_position[1] );
      voxel[2] = (int)( ray_position[2] );
      }
    }

  // Now that we have the max scalar value, convert this into the
  // correct intensity value according to  LinearRampRange and
  // LinearRampValue.  If the max value is less than LinearRampRange[0],
  // then the intensity is 0.  If the max value is greater than
  // LinearRampRange[1], then the intensity is LinearRampValue[1].
  // If the max value is between LinearRampRange[0] and LinearRampRange[1],
  // then linearly interpolate between LinearRampValue[0] and
  // LinearRampValue[1] to find the intensity value.
  if ( max >= mapper->LinearRampRange[0] )
    {
    if ( max < mapper->LinearRampRange[1] )
      {
      max_value = (max - mapper->LinearRampRange[0]) /
	(mapper->LinearRampRange[1] - mapper->LinearRampRange[0]);
      max_value = max_value * mapper->LinearRampValue[1] +
	(1.0 - max_value) * mapper->LinearRampValue[0];
      }
    else
      max_value = mapper->LinearRampValue[1];
    }
  else
    max_value = 0;
  
  // Set the return pixel value.  The depth value is currently useless and
  // should be fixed.
  if( mapper->ColorType == 0 )
    {
    pixel_value[0] = max_value * mapper->SingleColor[0];
    pixel_value[1] = max_value * mapper->SingleColor[1];
    pixel_value[2] = max_value * mapper->SingleColor[2];
    pixel_value[3] = max_value;
    pixel_value[4] = 0.3;
    }
  else if ( mapper->ColorType == 1)
    {
    if( mapper->ColorTransferFunction )
      {
      color = mapper->ColorTransferFunction->GetValue( max );
      pixel_value[0] = max_value * color[0];
      pixel_value[1] = max_value * color[1];
      pixel_value[2] = max_value * color[2];
      }
    else
      {
      pixel_value[0] = 0.0;
      pixel_value[1] = 0.0;
      pixel_value[2] = 0.0;
      }

    pixel_value[3] = max_value;
    pixel_value[4] = 0.3;
    }
  else
    {
    // This is an error condition...
    pixel_value[0] = 0.0;
    pixel_value[1] = 0.0;
    pixel_value[2] = 0.0;
    pixel_value[3] = 0.0;
    pixel_value[4] = 0.0;
    }

  pixel_value[5] = steps_this_ray;
}

// Description:
// Construct a new vtkMIPVolumeRayCaster with a default ramp.
// This ramp is best suited for unsigned char data and should
// probably be modified before rendering any other data type.
// The ParcBuildValue is set to LinearRampRange[0] + 1, ensuring
// that the Parc structure will be built during the first render.
vtkMIPVolumeRayCaster::vtkMIPVolumeRayCaster()
{
  this->ColorType            = 0;
  this->SingleColor[0]       = 1.0;
  this->SingleColor[1]       = 1.0;
  this->SingleColor[2]       = 1.0;
  this->ColorTransferFunction= NULL;

  this->LinearRampRange[0]   = 128.0;
  this->LinearRampRange[1]   = 255.0;
  this->LinearRampValue[0]   = 0.0;
  this->LinearRampValue[1]   = 1.0;
}

// Description:
// Destruct the vtkMIPVolumeRayCaster
vtkMIPVolumeRayCaster::~vtkMIPVolumeRayCaster()
{
}

// Description:
// This is called from RenderAnImage (in vtkDepthPARCMapper.cxx)
// It uses the integer data type flag that is passed in to
// determine what type of ray needs to be cast (which is handled
// by a templated function. 
void vtkMIPVolumeRayCaster::CastARay( int ray_type, void *data_ptr,
				  float ray_position[3], 
				  float ray_increment[3],
				  int num_steps, float pixel_value[6] )
{
  switch ( ray_type )
    {
    case 0:
      CastMIPRay( this, (unsigned char *)data_ptr, ray_position, 
		  ray_increment, num_steps, pixel_value );
      break;
    case 1:
      CastMIPRay( this, (unsigned short *)data_ptr, ray_position, 
		  ray_increment, num_steps, pixel_value );
      break;
    case 2:
      CastMIPRay( this, (short *)data_ptr, ray_position, 
		  ray_increment, num_steps, pixel_value );
      break;
    case 3:
      CastMIPRay( this, (int *)data_ptr, ray_position, 
		  ray_increment, num_steps, pixel_value );
      break;
    case 4:
      CastMIPRay( this, (float *)data_ptr, ray_position, 
		  ray_increment, num_steps, pixel_value );
      break;
    }  
}

// Description:
float vtkMIPVolumeRayCaster::GetZeroOpacityThreshold( )
{
  return ( this->LinearRampRange[0] );
}

// Description:
// This is an update method that is called from Render (in
// vtkDepthPARCMapper.cxx).  It allows the specific mapper type to
// update any local caster variables.  In this case, nothing needs
// to be done here
void vtkMIPVolumeRayCaster::CasterUpdate( vtkRenderer *ren, vtkVolume *vol )
{
}

// Description:
// Print method for vtkMIPVolumeRayCaster
void vtkMIPVolumeRayCaster::PrintSelf(ostream& os, vtkIndent indent)
{
  os << indent << "LinearRampRange: " << this->LinearRampRange[0] << " - "
    << this->LinearRampRange[1] << "\n";

  os << indent << "LinearRampValue: " << this->LinearRampValue[0] << " - "
    << this->LinearRampValue[1] << "\n";
  
  vtkVolumeRayCaster::PrintSelf(os,indent);
}
