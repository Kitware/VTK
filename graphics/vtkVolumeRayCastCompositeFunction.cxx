/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastCompositeFunction.cxx
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

#include "vtkVolumeRayCastCompositeFunction.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeRayCastMapper.h"

#define VTK_REMAINING_OPACITY		0.02

// Description:
// This is the templated function that actually casts a ray and computes
// The composite value. This version uses nearest neighbor interpolation
// and does not perform shading.
template <class T>
static void CastRay_NN_Unshaded( vtkVolumeRayCastCompositeFunction *cast_function, 
				 T *data_ptr,
				 float ray_start[3], 
				 float ray_increment[3],
				 int num_steps, float pixel_value[6] )
{
  int             value;
  float           accum_red_intensity;
  float           accum_green_intensity;
  float           accum_blue_intensity;
  float           accum_intensity;
  float           remaining_opacity;
  float           opacity;
  int             loop;
  int             xinc, yinc, zinc;
  int             voxel[3];
  float           ray_position[3];
  int             prev_voxel[3];
  float           *OTF;
  float           *CTF;
  float           *GTF;
  int             offset;
  int             steps_this_ray = 0;
 
  OTF =  cast_function->OpacityTFArray;
  CTF =  cast_function->RGBTFArray;
  GTF =  cast_function->GrayTFArray;

  // Move the increments into local variables
  xinc = cast_function->DataIncrement[0];
  yinc = cast_function->DataIncrement[1];
  zinc = cast_function->DataIncrement[2];

  // Initialize the ray position and voxel location
  ray_position[0] = ray_start[0];
  ray_position[1] = ray_start[1];
  ray_position[2] = ray_start[2];
  voxel[0] = vtkRoundFuncMacro( ray_position[0] );
  voxel[1] = vtkRoundFuncMacro( ray_position[1] );
  voxel[2] = vtkRoundFuncMacro( ray_position[2] );

  // So far we haven't accumulated anything
  accum_intensity         = 0.0;
  accum_red_intensity     = 0.0;
  accum_green_intensity   = 0.0;
  accum_blue_intensity    = 0.0;
  remaining_opacity       = 1.0;

  // Set up the data values for the first pass through the loop
  offset = voxel[2] * zinc + voxel[1] * yinc + voxel[0];
  value = *(data_ptr + offset);
  opacity = OTF[value];
  
  // Keep track of previous voxel to know when we step into a new one
  prev_voxel[0] = voxel[0];
  prev_voxel[1] = voxel[1];
  prev_voxel[2] = voxel[2];
  
  // Two cases - we are working with a gray or RGB transfer
  // function - break them up to make it more efficient
  if ( cast_function->ColorChannels == 1 ) 
    {
    // For each step along the ray
    for ( loop = 0; 
	  loop < num_steps && remaining_opacity > VTK_REMAINING_OPACITY; 
	  loop++ )
      {	    
      // We've taken another step
      steps_this_ray++;
      
      // Access the value at this voxel location
      if ( prev_voxel[0] != voxel[0] ||
	   prev_voxel[1] != voxel[1] ||
	   prev_voxel[2] != voxel[2] )
	{
	offset = voxel[2] * zinc + voxel[1] * yinc + voxel[0];
	value = *(data_ptr + offset);
	opacity = OTF[value];
	
	prev_voxel[0] = voxel[0];
	prev_voxel[1] = voxel[1];
	prev_voxel[2] = voxel[2];
	}
        
      // Accumulate some light intensity and opacity
      accum_red_intensity   += ( opacity * remaining_opacity * 
				 GTF[(value)] );
      remaining_opacity *= (1.0 - opacity);
      
      // Increment our position and compute our voxel location
      ray_position[0] += ray_increment[0];
      ray_position[1] += ray_increment[1];
      ray_position[2] += ray_increment[2];
      voxel[0] = vtkRoundFuncMacro( ray_position[0] );
      voxel[1] = vtkRoundFuncMacro( ray_position[1] );
      voxel[2] = vtkRoundFuncMacro( ray_position[2] );
      }
    accum_green_intensity = accum_red_intensity;
    accum_blue_intensity = accum_red_intensity;
    }
  else if ( cast_function->ColorChannels == 3 )
    {
    // For each step along the ray
    for ( loop = 0; 
	  loop < num_steps && remaining_opacity > VTK_REMAINING_OPACITY; 
	  loop++ )
      {	    
      // We've taken another step
      steps_this_ray++;
      
      // Access the value at this voxel location
      if ( prev_voxel[0] != voxel[0] ||
	   prev_voxel[1] != voxel[1] ||
	   prev_voxel[2] != voxel[2] )
	{
	offset = voxel[2] * zinc + voxel[1] * yinc + voxel[0];
	value = *(data_ptr + offset);
	opacity = OTF[value];
	
	prev_voxel[0] = voxel[0];
	prev_voxel[1] = voxel[1];
	prev_voxel[2] = voxel[2];
	}
        
      // Accumulate some light intensity and opacity
      accum_red_intensity   += ( opacity * remaining_opacity * 
				 CTF[(value)*3] );
      accum_green_intensity += ( opacity * remaining_opacity * 
				 CTF[(value)*3 + 1] );
      accum_blue_intensity  += ( opacity * remaining_opacity * 
				 CTF[(value)*3 + 2] );
      remaining_opacity *= (1.0 - opacity);
      
      // Increment our position and compute our voxel location
      ray_position[0] += ray_increment[0];
      ray_position[1] += ray_increment[1];
      ray_position[2] += ray_increment[2];
      voxel[0] = vtkRoundFuncMacro( ray_position[0] );
      voxel[1] = vtkRoundFuncMacro( ray_position[1] );
      voxel[2] = vtkRoundFuncMacro( ray_position[2] );
      }
    }

  // Cap the intensity value at 1.0
  if ( accum_red_intensity > 1.0 )
    accum_red_intensity = 1.0;
  if ( accum_green_intensity > 1.0 )
    accum_green_intensity = 1.0;
  if ( accum_blue_intensity > 1.0 )
    accum_blue_intensity = 1.0;
  
  if( remaining_opacity < VTK_REMAINING_OPACITY )
    remaining_opacity = 0.0;

  // Set the return pixel value.  The depth value is currently useless and
  // should be fixed.  What should depth be in this case?  First 
  // non-opaque or total opacity or what??
  pixel_value[0] = accum_red_intensity;
  pixel_value[1] = accum_green_intensity;
  pixel_value[2] = accum_blue_intensity;
  pixel_value[3] = 1.0 - remaining_opacity;
  pixel_value[4] = 0.3;
  pixel_value[5] = steps_this_ray;
  
}


// Description:
// This is the templated function that actually casts a ray and computes
// the composite value. This version uses nearest neighbor and does
// perform shading.
template <class T>
static void CastRay_NN_Shaded( vtkVolumeRayCastCompositeFunction *cast_function, 
			       T *data_ptr,
			       float ray_start[3], 
			       float ray_increment[3],
			       int num_steps, float pixel_value[6] )
{
  int             value = 0;
  float           accum_red_intensity;
  float           accum_green_intensity;
  float           accum_blue_intensity;
  float           remaining_opacity;
  float           opacity = 0.0;
  int             loop;
  int             xinc, yinc, zinc;
  int             voxel[3];
  float           ray_position[3];
  int             prev_voxel[3];
  float           *OTF;
  float           *CTF;
  float           *GTF;
  float           *red_d_shade, *green_d_shade, *blue_d_shade;
  float           *red_s_shade, *green_s_shade, *blue_s_shade;
  unsigned short  *encoded_normals;
  float           red_shaded_value   = 0.0;
  float           green_shaded_value = 0.0;
  float           blue_shaded_value  = 0.0;
  int             offset = 0;
  int             steps_this_ray = 0;
 
  // Get diffuse shading table pointers
  red_d_shade = cast_function->RedDiffuseShadingTable;
  green_d_shade = cast_function->GreenDiffuseShadingTable;
  blue_d_shade = cast_function->BlueDiffuseShadingTable;

  // Get specular shading table pointers
  red_s_shade = cast_function->RedSpecularShadingTable;
  green_s_shade = cast_function->GreenSpecularShadingTable;
  blue_s_shade = cast_function->BlueSpecularShadingTable;

  // Get a pointer to the encoded normals for this volume
  encoded_normals = cast_function->EncodedNormals;

  // Get the opacity transfer function for this volume (which maps
  // scalar input values to opacities)
  OTF =  cast_function->OpacityTFArray;

  // Get the color transfer function for this volume (which maps
  // scalar input values to RGB values)
  CTF =  cast_function->RGBTFArray;
  GTF =  cast_function->GrayTFArray;

  // Move the increments into local variables
  xinc = cast_function->DataIncrement[0];
  yinc = cast_function->DataIncrement[1];
  zinc = cast_function->DataIncrement[2];

  // Initialize the ray position and voxel location
  ray_position[0] = ray_start[0];
  ray_position[1] = ray_start[1];
  ray_position[2] = ray_start[2];

  voxel[0] = vtkRoundFuncMacro( ray_position[0] );
  voxel[1] = vtkRoundFuncMacro( ray_position[1] );
  voxel[2] = vtkRoundFuncMacro( ray_position[2] );

  // So far we haven't accumulated anything
  accum_red_intensity     = 0.0;
  accum_green_intensity   = 0.0;
  accum_blue_intensity    = 0.0;
  remaining_opacity       = 1.0;

  // Keep track of previous voxel to know when we step into a new one  
  prev_voxel[0] = voxel[0]-1;
  prev_voxel[1] = voxel[1]-1;
  prev_voxel[2] = voxel[2]-1;
  
  // Two cases - we are working with a gray or RGB transfer
  // function - break them up to make it more efficient
  if ( cast_function->ColorChannels == 1 ) 
    {
    // For each step along the ray
    for ( loop = 0; 
	  loop < num_steps && remaining_opacity > VTK_REMAINING_OPACITY; loop++ )
      {	    
      // We've taken another step
      steps_this_ray++;
      
      // Access the value at this voxel location and compute
      // opacity and shaded value
      if ( prev_voxel[0] != voxel[0] ||
	   prev_voxel[1] != voxel[1] ||
	   prev_voxel[2] != voxel[2] )
	{
	offset = voxel[2] * zinc + voxel[1] * yinc + voxel[0];
	value = *(data_ptr + offset);
      
	opacity = OTF[value];
	if ( opacity )
	  red_shaded_value = opacity *  remaining_opacity *
	    ( red_d_shade[*(encoded_normals + offset)] * GTF[value] +
	      red_s_shade[*(encoded_normals + offset)] );
	else
	  red_shaded_value = 0.0;

	prev_voxel[0] = voxel[0];
	prev_voxel[1] = voxel[1];
	prev_voxel[2] = voxel[2];
	}
    
    
      // Accumulate the shaded intensity and opacity of this sample
      accum_red_intensity += red_shaded_value;
      remaining_opacity *= (1.0 - opacity);
    
      // Increment our position and compute our voxel location
      ray_position[0] += ray_increment[0];
      ray_position[1] += ray_increment[1];
      ray_position[2] += ray_increment[2];
      voxel[0] = vtkRoundFuncMacro( ray_position[0] );
      voxel[1] = vtkRoundFuncMacro( ray_position[1] );
      voxel[2] = vtkRoundFuncMacro( ray_position[2] );
      }
    accum_green_intensity = accum_red_intensity;
    accum_blue_intensity = accum_red_intensity;
    }
  else if ( cast_function->ColorChannels == 3 )
    {
    // For each step along the ray
    for ( loop = 0; 
	  loop < num_steps && remaining_opacity > VTK_REMAINING_OPACITY; loop++ )
      {	    
      // We've taken another step
      steps_this_ray++;
      
      // Access the value at this voxel location and compute
      // opacity and shaded value
      if ( prev_voxel[0] != voxel[0] ||
	   prev_voxel[1] != voxel[1] ||
	   prev_voxel[2] != voxel[2] )
	{
	offset = voxel[2] * zinc + voxel[1] * yinc + voxel[0];
	value = *(data_ptr + offset);
      
	opacity = OTF[value];
	if ( opacity )
	  {
	  red_shaded_value = opacity *  remaining_opacity *
	    ( red_d_shade[*(encoded_normals + offset)] * CTF[value*3] +
	      red_s_shade[*(encoded_normals + offset)] );
	  green_shaded_value = opacity *  remaining_opacity *
	    ( green_d_shade[*(encoded_normals + offset)] * CTF[value*3 + 1] +
	      green_s_shade[*(encoded_normals + offset)] );
	  blue_shaded_value = opacity *  remaining_opacity *
	    ( blue_d_shade[*(encoded_normals + offset)] * CTF[value*3 + 2] +
	      blue_s_shade[*(encoded_normals + offset)] );
	  }	
	else
	  {
	  red_shaded_value = 0.0;
	  green_shaded_value = 0.0;
	  blue_shaded_value = 0.0;
	  }

	prev_voxel[0] = voxel[0];
	prev_voxel[1] = voxel[1];
	prev_voxel[2] = voxel[2];
	}
    
    
      // Accumulate the shaded intensity and opacity of this sample
      accum_red_intensity += red_shaded_value;
      accum_green_intensity += green_shaded_value;
      accum_blue_intensity += blue_shaded_value;
      remaining_opacity *= (1.0 - opacity);
    
      // Increment our position and compute our voxel location
      ray_position[0] += ray_increment[0];
      ray_position[1] += ray_increment[1];
      ray_position[2] += ray_increment[2];
      voxel[0] = vtkRoundFuncMacro( ray_position[0] );
      voxel[1] = vtkRoundFuncMacro( ray_position[1] );
      voxel[2] = vtkRoundFuncMacro( ray_position[2] );
      }
    }
  
  // Cap the intensities at 1.0
  if ( accum_red_intensity > 1.0 )
    accum_red_intensity = 1.0;
  if ( accum_green_intensity > 1.0 )
    accum_green_intensity = 1.0;
  if ( accum_blue_intensity > 1.0 )
    accum_blue_intensity = 1.0;
  
  if( remaining_opacity < VTK_REMAINING_OPACITY )
    remaining_opacity = 0.0;
  
  // Set the return pixel value.  The depth value is currently useless and
  // should be fixed.  What should depth be in this case?  First 
  // non-opaque or total opacity or what??
  pixel_value[0] = accum_red_intensity;
  pixel_value[1] = accum_green_intensity;
  pixel_value[2] = accum_blue_intensity;
  pixel_value[3] = 1.0 - remaining_opacity;
  pixel_value[4] = 0.3;
  pixel_value[5] = steps_this_ray;
  
}

// Description:
// This is the templated function that actually casts a ray and computes
// the composite value.  This version uses trilinear interpolation and
// does not compute shading
template <class T>
static void CastRay_TrilinSample_Unshaded( 
					  vtkVolumeRayCastCompositeFunction *cast_function, 
					  T *data_ptr,
					  float ray_start[3], 
					  float ray_increment[3],
					  int num_steps, float pixel_value[6] )
{
  float           accum_intensity;
  float           accum_red_intensity;
  float           accum_green_intensity;
  float           accum_blue_intensity;
  float           remaining_opacity;
  float           red_value, green_value, blue_value;
  float           opacity;
  int             loop;
  int             xinc, yinc, zinc;
  int             voxel[3];
  float           ray_position[3];
  float           A, B, C, D, E, F, G, H;
  int             Binc, Cinc, Dinc, Einc, Finc, Ginc, Hinc;
  T               *dptr;
  float           *OTF;
  float           *CTF;
  float           *GTF;
  float           x, y, z, t1, t2, t3;
  int             offset;
  int             steps_this_ray = 0;
  float           scalar_value;

  // Get the opacity transfer function which maps scalar input values
  // to opacities
  OTF =  cast_function->OpacityTFArray;

  // Get the color transfer function which maps scalar input values
  // to RGB colors
  CTF =  cast_function->RGBTFArray;
  GTF =  cast_function->GrayTFArray;

  // Move the increments into local variables
  xinc = cast_function->DataIncrement[0];
  yinc = cast_function->DataIncrement[1];
  zinc = cast_function->DataIncrement[2];

  // Initialize the ray position and voxel location
  ray_position[0] = ray_start[0];
  ray_position[1] = ray_start[1];
  ray_position[2] = ray_start[2];
  voxel[0] = (int)( ray_position[0] );
  voxel[1] = (int)( ray_position[1] );
  voxel[2] = (int)( ray_position[2] );

  // So far we have not accumulated anything
  accum_intensity         = 0.0;
  accum_red_intensity     = 0.0;
  accum_green_intensity   = 0.0;
  accum_blue_intensity    = 0.0;
  remaining_opacity       = 1.0;

  // Compute the increments to get to the other 7 voxel vertices from A
  Binc = xinc;
  Cinc = yinc;
  Dinc = xinc + yinc;
  Einc = zinc;
  Finc = zinc + xinc;
  Ginc = zinc + yinc;
  Hinc = zinc + xinc + yinc;
  
  // Two cases - we are working with a gray or RGB transfer
  // function - break them up to make it more efficient
  if ( cast_function->ColorChannels == 1 ) 
    {
    // For each step along the ray
    for ( loop = 0; 
	  loop < num_steps && remaining_opacity > VTK_REMAINING_OPACITY; 
	  loop++ )
      {	    
      // We've taken another step
      steps_this_ray++;
      
      offset = voxel[2] * zinc + voxel[1] * yinc + voxel[0];
      dptr = data_ptr + offset;
      
      A = *(dptr);
      B = *(dptr + Binc);
      C = *(dptr + Cinc);
      D = *(dptr + Dinc);
      E = *(dptr + Einc);
      F = *(dptr + Finc);
      G = *(dptr + Ginc);
      H = *(dptr + Hinc);
      
      // Compute our offset in the voxel, and use that to trilinearly
      // interpolate the value
      x = ray_position[0] - (float) voxel[0];
      y = ray_position[1] - (float) voxel[1];
      z = ray_position[2] - (float) voxel[2];
      
      t1 = 1.0 - x;
      t2 = 1.0 - y;
      t3 = 1.0 - z;
      
      scalar_value = 
	A * t1 * t2 * t3 +
	B *  x * t2 * t3 +
	C * t1 *  y * t3 + 
	D *  x *  y * t3 +
	E * t1 * t2 *  z + 
	F *  x * t2 *  z + 
	G * t1 *  y *  z + 
	H *  x *  y *  z;
      
      if ( scalar_value < 0.0 ) 
	scalar_value = 0.0;
      else if ( scalar_value > cast_function->TFArraySize - 1 )
	scalar_value = cast_function->TFArraySize - 1;
      
      opacity = OTF[(int)scalar_value];
      
      if ( opacity )
	{
	red_value   = opacity * GTF[((int)scalar_value)];
	
	// Accumulate intensity and opacity for this sample location
	accum_red_intensity   += remaining_opacity * red_value;
	remaining_opacity *= (1.0 - opacity);
	}
    
      // Increment our position and compute our voxel location
      ray_position[0] += ray_increment[0];
      ray_position[1] += ray_increment[1];
      ray_position[2] += ray_increment[2];      
      voxel[0] = (int)( ray_position[0] );
      voxel[1] = (int)( ray_position[1] );
      voxel[2] = (int)( ray_position[2] );
      }
    accum_green_intensity = accum_red_intensity;
    accum_blue_intensity = accum_red_intensity;
    }
  else if ( cast_function->ColorChannels == 3 )
    {
    // For each step along the ray
    for ( loop = 0; 
	  loop < num_steps && remaining_opacity > VTK_REMAINING_OPACITY; 
	  loop++ )
      {	    
      // We've taken another step
      steps_this_ray++;
      
      offset = voxel[2] * zinc + voxel[1] * yinc + voxel[0];
      dptr = data_ptr + offset;
      
      A = *(dptr);
      B = *(dptr + Binc);
      C = *(dptr + Cinc);
      D = *(dptr + Dinc);
      E = *(dptr + Einc);
      F = *(dptr + Finc);
      G = *(dptr + Ginc);
      H = *(dptr + Hinc);
      
      // Compute our offset in the voxel, and use that to trilinearly
      // interpolate the value
      x = ray_position[0] - (float) voxel[0];
      y = ray_position[1] - (float) voxel[1];
      z = ray_position[2] - (float) voxel[2];
      
      t1 = 1.0 - x;
      t2 = 1.0 - y;
      t3 = 1.0 - z;
      
      scalar_value = 
	A * t1 * t2 * t3 +
	B *  x * t2 * t3 +
	C * t1 *  y * t3 + 
	D *  x *  y * t3 +
	E * t1 * t2 *  z + 
	F *  x * t2 *  z + 
	G * t1 *  y *  z + 
	H *  x *  y *  z;
      
      if ( scalar_value < 0.0 ) 
	scalar_value = 0.0;
      else if ( scalar_value > cast_function->TFArraySize - 1 )
	scalar_value = cast_function->TFArraySize - 1;
      
      opacity = OTF[(int)scalar_value];
      
      if ( opacity )
	{
	red_value   = opacity * CTF[((int)scalar_value) * 3    ];
	green_value = opacity * CTF[((int)scalar_value) * 3 + 1];
	blue_value  = opacity * CTF[((int)scalar_value) * 3 + 2];
	
	// Accumulate intensity and opacity for this sample location
	accum_red_intensity   += remaining_opacity * red_value;
	accum_green_intensity += remaining_opacity * green_value;
	accum_blue_intensity  += remaining_opacity * blue_value;
	remaining_opacity *= (1.0 - opacity);
	}
    
      // Increment our position and compute our voxel location
      ray_position[0] += ray_increment[0];
      ray_position[1] += ray_increment[1];
      ray_position[2] += ray_increment[2];      
      voxel[0] = (int)( ray_position[0] );
      voxel[1] = (int)( ray_position[1] );
      voxel[2] = (int)( ray_position[2] );
      }
    }

  // Cap the intensity value at 1.0
  if ( accum_red_intensity > 1.0 )
    accum_red_intensity = 1.0;
  if ( accum_green_intensity > 1.0 )
    accum_green_intensity = 1.0;
  if ( accum_blue_intensity > 1.0 )
    accum_blue_intensity = 1.0;
  
  if( remaining_opacity < VTK_REMAINING_OPACITY )
    remaining_opacity = 0.0;

  // Set the return pixel value.  The depth value is currently useless and
  // should be fixed.  What should depth be in this case?  First 
  // non-opaque or total opacity or what??
  pixel_value[0] = accum_red_intensity;
  pixel_value[1] = accum_green_intensity;
  pixel_value[2] = accum_blue_intensity;
  pixel_value[3] = 1.0 - remaining_opacity;
  pixel_value[4] = 0.3;
  pixel_value[5] = steps_this_ray;

}

// Description:
// This is the templated function that actually casts a ray and computes
// the composite value.  This version uses trilinear interpolation, and
// does perform shading.
template <class T>
static void CastRay_TrilinSample_Shaded( 
					vtkVolumeRayCastCompositeFunction *cast_function, 
					T *data_ptr,
					float ray_start[3], 
					float ray_increment[3],
					int num_steps, float pixel_value[6] )
{
  float           accum_red_intensity;
  float           accum_green_intensity;
  float           accum_blue_intensity;
  float           remaining_opacity;
  float           opacity;
  int             loop;
  int             xinc, yinc, zinc;
  int             voxel[3];
  float           ray_position[3];
  float           A, B, C, D, E, F, G, H;
  int             A_n, B_n, C_n, D_n, E_n, F_n, G_n, H_n;
  float           final_rd, final_gd, final_bd;
  float           final_rs, final_gs, final_bs;
  int             Binc, Cinc, Dinc, Einc, Finc, Ginc, Hinc;
  T               *dptr;
  float           *OTF;
  float           *CTF;
  float           *GTF;
  float           x, y, z, t1, t2, t3;
  float           tA, tB, tC, tD, tE, tF, tG, tH;
  float           *red_d_shade, *green_d_shade, *blue_d_shade;
  float           *red_s_shade, *green_s_shade, *blue_s_shade;
  unsigned short  *encoded_normals, *nptr;
  float           red_shaded_value, green_shaded_value, blue_shaded_value;
  int             offset;
  int             steps_this_ray = 0;
  int             scalar_value;
  float           r, g, b;

  // Get diffuse shading table pointers
  red_d_shade = cast_function->RedDiffuseShadingTable;
  green_d_shade = cast_function->GreenDiffuseShadingTable;
  blue_d_shade = cast_function->BlueDiffuseShadingTable;


  // Get diffuse shading table pointers
  red_s_shade = cast_function->RedSpecularShadingTable;
  green_s_shade = cast_function->GreenSpecularShadingTable;
  blue_s_shade = cast_function->BlueSpecularShadingTable;

  // Get a pointer to the encoded normals for this volume
  encoded_normals = cast_function->EncodedNormals;

  // Get the opacity transfer function which maps scalar input values
  // to opacities
  OTF =  cast_function->OpacityTFArray;

  // Get the color transfer function which maps scalar input values
  // to RGB values
  CTF =  cast_function->RGBTFArray;
  GTF =  cast_function->GrayTFArray;

  // Move the increments into local variables
  xinc = cast_function->DataIncrement[0];
  yinc = cast_function->DataIncrement[1];
  zinc = cast_function->DataIncrement[2];

  // Initialize the ray position and voxel location
  ray_position[0] = ray_start[0];
  ray_position[1] = ray_start[1];
  ray_position[2] = ray_start[2];
  voxel[0] = (int)( ray_position[0] );
  voxel[1] = (int)( ray_position[1] );
  voxel[2] = (int)( ray_position[2] );

  // So far we haven't accumulated anything
  accum_red_intensity   = 0.0;
  accum_green_intensity = 0.0;
  accum_blue_intensity  = 0.0;
  remaining_opacity     = 1.0;

  // Compute the increments to get to the other 7 voxel vertices from A
  Binc = xinc;
  Cinc = yinc;
  Dinc = xinc + yinc;
  Einc = zinc;
  Finc = zinc + xinc;
  Ginc = zinc + yinc;
  Hinc = zinc + xinc + yinc;
  
   // Compute the values for the first pass through the loop
  offset = voxel[2] * zinc + voxel[1] * yinc + voxel[0];
  dptr = data_ptr + offset;
  nptr = encoded_normals + offset;

  // Two cases - we are working with a gray or RGB transfer
  // function - break them up to make it more efficient
  if ( cast_function->ColorChannels == 1 ) 
    {
    // For each step along the ray
    for ( loop = 0; 
	  loop < num_steps && remaining_opacity > VTK_REMAINING_OPACITY; 
	  loop++ )
      {	    
      // We've taken another step
      steps_this_ray++;
      
      offset = voxel[2] * zinc + voxel[1] * yinc + voxel[0];
      dptr = data_ptr + offset;
      nptr = encoded_normals + offset;
    
      A = *(dptr);
      B = *(dptr + Binc);
      C = *(dptr + Cinc);
      D = *(dptr + Dinc);
      E = *(dptr + Einc);
      F = *(dptr + Finc);
      G = *(dptr + Ginc);
      H = *(dptr + Hinc);
      
      A_n = *(nptr);
      B_n = *(nptr + Binc);
      C_n = *(nptr + Cinc);
      D_n = *(nptr + Dinc);
      E_n = *(nptr + Einc);
      F_n = *(nptr + Finc);
      G_n = *(nptr + Ginc);
      H_n = *(nptr + Hinc);

      // Compute our offset in the voxel, and use that to trilinearly
      // interpolate a value
      x = ray_position[0] - (float) voxel[0];
      y = ray_position[1] - (float) voxel[1];
      z = ray_position[2] - (float) voxel[2];
      
      t1 = 1.0 - x;
      t2 = 1.0 - y;
      t3 = 1.0 - z;
      
      tA = t1 * t2 * t3;
      tB =  x * t2 * t3;
      tC = t1 *  y * t3;
      tD =  x *  y * t3;
      tE = t1 * t2 *  z;
      tF =  x * t2 *  z;
      tG = t1 *  y *  z;
      tH =  x *  y *  z;
      
      scalar_value = (int) (
			    A * tA + B * tB + C * tC + D * tD + 
			    E * tE + F * tF + G * tG + H * tH );
      
      if ( scalar_value < 0 ) 
	scalar_value = 0;
      else if ( scalar_value > cast_function->TFArraySize - 1 )
	scalar_value = cast_function->TFArraySize - 1;
      
      opacity = OTF[scalar_value];
      
      if ( opacity )
	{
	  final_rd = 
	    red_d_shade[ A_n ] * tA + red_d_shade[ B_n ] * tB + 	
	    red_d_shade[ C_n ] * tC + red_d_shade[ D_n ] * tD + 
	    red_d_shade[ E_n ] * tE + red_d_shade[ F_n ] * tF +	
	    red_d_shade[ G_n ] * tG + red_d_shade[ H_n ] * tH;
	  
	  final_rs = 
	    red_s_shade[ A_n ] * tA + red_s_shade[ B_n ] * tB + 	
	    red_s_shade[ C_n ] * tC + red_s_shade[ D_n ] * tD + 
	    red_s_shade[ E_n ] * tE + red_s_shade[ F_n ] * tF +	
	    red_s_shade[ G_n ] * tG + red_s_shade[ H_n ] * tH;
	  	  
	  r = GTF[(scalar_value)];

	  // For this sample we have do not yet have any opacity or
	  // shaded intensity yet
	  red_shaded_value   = opacity * ( final_rd * r + final_rs );
	  
	  // Accumulate intensity and opacity for this sample location   
	  accum_red_intensity   += red_shaded_value   * remaining_opacity;
	  remaining_opacity *= (1.0 - opacity);
	}

      // Increment our position and compute our voxel location
      ray_position[0] += ray_increment[0];
      ray_position[1] += ray_increment[1];
      ray_position[2] += ray_increment[2];      
      voxel[0] = (int)( ray_position[0] );
      voxel[1] = (int)( ray_position[1] );
      voxel[2] = (int)( ray_position[2] );
      }
    accum_green_intensity = accum_red_intensity;
    accum_blue_intensity = accum_red_intensity;
    }
  else if ( cast_function->ColorChannels == 3 )
    {
    // For each step along the ray
    for ( loop = 0; 
	  loop < num_steps && remaining_opacity > VTK_REMAINING_OPACITY; 
	  loop++ )
      {	    
      // We've taken another step
      steps_this_ray++;
      
      offset = voxel[2] * zinc + voxel[1] * yinc + voxel[0];
      dptr = data_ptr + offset;
      nptr = encoded_normals + offset;
    
      A = *(dptr);
      B = *(dptr + Binc);
      C = *(dptr + Cinc);
      D = *(dptr + Dinc);
      E = *(dptr + Einc);
      F = *(dptr + Finc);
      G = *(dptr + Ginc);
      H = *(dptr + Hinc);
      
      A_n = *(nptr);
      B_n = *(nptr + Binc);
      C_n = *(nptr + Cinc);
      D_n = *(nptr + Dinc);
      E_n = *(nptr + Einc);
      F_n = *(nptr + Finc);
      G_n = *(nptr + Ginc);
      H_n = *(nptr + Hinc);

      // Compute our offset in the voxel, and use that to trilinearly
      // interpolate a value
      x = ray_position[0] - (float) voxel[0];
      y = ray_position[1] - (float) voxel[1];
      z = ray_position[2] - (float) voxel[2];
      
      t1 = 1.0 - x;
      t2 = 1.0 - y;
      t3 = 1.0 - z;
      
      tA = t1 * t2 * t3;
      tB =  x * t2 * t3;
      tC = t1 *  y * t3;
      tD =  x *  y * t3;
      tE = t1 * t2 *  z;
      tF =  x * t2 *  z;
      tG = t1 *  y *  z;
      tH =  x *  y *  z;
      
      scalar_value = (int) (
			    A * tA + B * tB + C * tC + D * tD + 
			    E * tE + F * tF + G * tG + H * tH );
      
      if ( scalar_value < 0 ) 
	scalar_value = 0;
      else if ( scalar_value > cast_function->TFArraySize - 1 )
	scalar_value = cast_function->TFArraySize - 1;
      
      opacity = OTF[scalar_value];
      
      if ( opacity )
	{
	  final_rd = 
	    red_d_shade[ A_n ] * tA + red_d_shade[ B_n ] * tB + 	
	    red_d_shade[ C_n ] * tC + red_d_shade[ D_n ] * tD + 
	    red_d_shade[ E_n ] * tE + red_d_shade[ F_n ] * tF +	
	    red_d_shade[ G_n ] * tG + red_d_shade[ H_n ] * tH;
	  
	  final_gd = 
	    green_d_shade[ A_n ] * tA + green_d_shade[ B_n ] * tB + 	
	    green_d_shade[ C_n ] * tC + green_d_shade[ D_n ] * tD + 
	    green_d_shade[ E_n ] * tE + green_d_shade[ F_n ] * tF +	
	    green_d_shade[ G_n ] * tG + green_d_shade[ H_n ] * tH;
	  
	  final_bd = 
	    blue_d_shade[ A_n ] * tA + blue_d_shade[ B_n ] * tB + 	
	    blue_d_shade[ C_n ] * tC + blue_d_shade[ D_n ] * tD + 
	    blue_d_shade[ E_n ] * tE + blue_d_shade[ F_n ] * tF +	
	    blue_d_shade[ G_n ] * tG + blue_d_shade[ H_n ] * tH;
	  
	  final_rs = 
	    red_s_shade[ A_n ] * tA + red_s_shade[ B_n ] * tB + 	
	    red_s_shade[ C_n ] * tC + red_s_shade[ D_n ] * tD + 
	    red_s_shade[ E_n ] * tE + red_s_shade[ F_n ] * tF +	
	    red_s_shade[ G_n ] * tG + red_s_shade[ H_n ] * tH;
	  
	  final_gs = 
	    green_s_shade[ A_n ] * tA + green_s_shade[ B_n ] * tB + 	
	    green_s_shade[ C_n ] * tC + green_s_shade[ D_n ] * tD + 
	    green_s_shade[ E_n ] * tE + green_s_shade[ F_n ] * tF +	
	    green_s_shade[ G_n ] * tG + green_s_shade[ H_n ] * tH;
	  
	  final_bs = 
	    blue_s_shade[ A_n ] * tA + blue_s_shade[ B_n ] * tB + 	
	    blue_s_shade[ C_n ] * tC + blue_s_shade[ D_n ] * tD + 
	    blue_s_shade[ E_n ] * tE + blue_s_shade[ F_n ] * tF +	
	    blue_s_shade[ G_n ] * tG + blue_s_shade[ H_n ] * tH;
	  
	  r = CTF[(scalar_value) * 3    ];
	  g = CTF[(scalar_value) * 3 + 1];
	  b = CTF[(scalar_value) * 3 + 2];

	  // For this sample we have do not yet have any opacity or
	  // shaded intensity yet
	  red_shaded_value   = opacity * ( final_rd * r + final_rs );
	  green_shaded_value = opacity * ( final_gd * g + final_gs );
	  blue_shaded_value  = opacity * ( final_bd * b + final_bs );
	  
	  // Accumulate intensity and opacity for this sample location   
	  accum_red_intensity   += red_shaded_value   * remaining_opacity;
	  accum_green_intensity += green_shaded_value * remaining_opacity;
	  accum_blue_intensity  += blue_shaded_value  * remaining_opacity;
	  remaining_opacity *= (1.0 - opacity);
	}

      // Increment our position and compute our voxel location
      ray_position[0] += ray_increment[0];
      ray_position[1] += ray_increment[1];
      ray_position[2] += ray_increment[2];      
      voxel[0] = (int)( ray_position[0] );
      voxel[1] = (int)( ray_position[1] );
      voxel[2] = (int)( ray_position[2] );
      }
    }

  // Cap the accumulated intensity at 1.0
  if ( accum_red_intensity > 1.0 )
    accum_red_intensity = 1.0;
  if ( accum_green_intensity > 1.0 )
    accum_green_intensity = 1.0;
  if ( accum_blue_intensity > 1.0 )
    accum_blue_intensity = 1.0;
  
  if( remaining_opacity < VTK_REMAINING_OPACITY )
    remaining_opacity = 0.0;

  // Set the return pixel value.  The depth value is currently useless and
  // should be fixed.  What should depth be in this case?  First 
  // non-opaque or total opacity or what??
  pixel_value[0] = accum_red_intensity;
  pixel_value[1] = accum_green_intensity;
  pixel_value[2] = accum_blue_intensity;
  pixel_value[3] = 1.0 - remaining_opacity;
  pixel_value[4] = 0.3;
  pixel_value[5] = steps_this_ray;

}

// Description:
// Constructor for the vtkVolumeRayCastCompositeFunction class
vtkVolumeRayCastCompositeFunction::vtkVolumeRayCastCompositeFunction()
{
}

// Description:
// Destruct the vtkVolumeRayCastCompositeFunction
vtkVolumeRayCastCompositeFunction::~vtkVolumeRayCastCompositeFunction()
{
}

// Description:
// This is called from RenderAnImage (in vtkDepthPARCMapper.cxx)
// It uses the integer data type flag that is passed in to
// determine what type of ray needs to be cast (which is handled
// by a templated function.  It also uses the shading and
// interpolation types to determine which templated function
// to call.
void vtkVolumeRayCastCompositeFunction::CastARay( int ray_type, void *data_ptr,
						  float ray_position[3], 
						  float ray_increment[3],
						  int num_steps, float pixel_value[6] )
{
  // Cast the ray for the data type and shading/interpolation type
  if ( this->InterpolationType == VTK_NEAREST_INTERPOLATION )
    {
    if ( this->Shading == 0 )
      {
      // Nearest neighbor and no shading
      switch ( ray_type )
	{
	case 0:
	  CastRay_NN_Unshaded( this, (unsigned char *)data_ptr, ray_position, 
			       ray_increment, num_steps, pixel_value );
	  break;
	case 1:
	  CastRay_NN_Unshaded( this, (unsigned short *)data_ptr, ray_position, 
			       ray_increment, num_steps, pixel_value );
	  break;
	}
      }
    else
      {
      // Nearest neighbor and shading
      switch ( ray_type )
	{
	case 0:
	  CastRay_NN_Shaded( this, (unsigned char *)data_ptr, ray_position, 
			     ray_increment, num_steps, pixel_value );
	  break;
	case 1:
	  CastRay_NN_Shaded( this, (unsigned short *)data_ptr, ray_position, 
			     ray_increment, num_steps, pixel_value );
	  break;
	}
      }
    }
  else 
    {
    if ( this->Shading == 0 )
      {
      // Trilinear interpolation at vertices and no shading
      switch ( ray_type )
	{
	case 0:
	  CastRay_TrilinSample_Unshaded( this, (unsigned char *)data_ptr, 
					 ray_position, 
					 ray_increment, num_steps, 
					 pixel_value );
	  break;
	case 1:
	  CastRay_TrilinSample_Unshaded( this, (unsigned short *)data_ptr, 
					 ray_position, 
					 ray_increment, num_steps, 
					 pixel_value );
	  break;
	}
      }	
    else
      {
      // Trilinear interpolation and shading
      switch ( ray_type )
	{
	case 0:
	  CastRay_TrilinSample_Shaded( this, (unsigned char *)data_ptr, 
				       ray_position, 
				       ray_increment, num_steps, 
				       pixel_value );
	  break;
	case 1:
	  CastRay_TrilinSample_Shaded( this, (unsigned short *)data_ptr, 
				       ray_position, 
				       ray_increment, num_steps, 
				       pixel_value );
	  break;
	}
      }	
    }
}

// Description:
// Bogus routine right now until I figure out how to get to the
// volume's properties from here....
float vtkVolumeRayCastCompositeFunction::GetZeroOpacityThreshold( vtkVolume *vol )
{
  return( 1.0 );
}

// Description:
// We don't need to do any specific initialization here...
void vtkVolumeRayCastCompositeFunction::SpecificFunctionInitialize( 
				vtkRenderer *vtkNotUsed(ren), 
				vtkVolume *vtkNotUsed(vol),
				vtkVolumeRayCastMapper *vtkNotUsed(mapper) )
{
}


// Description:
// Print method for vtkVolumeRayCastCompositeFunction
// Since there is nothing local to print, just print the object stuff.
void vtkVolumeRayCastCompositeFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);
}





