/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeVolumeRayCaster.cxx
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

#include "vtkCompositeVolumeRayCaster.h"
#include "vtkCamera.h"
#include "vtkRenderWindow.h"
#include "vtkUnsignedCharScalars.h"
#include "vtkUnsignedShortScalars.h"
#include "vtkLightCollection.h"
#include "vtkLight.h"
#include "vtkRayCaster.h"

#define VTK_REMAINING_OPACITY		0.02

// Description:
// This is the templated function that actually casts a ray and computes
// The composite value. This version uses nearest neighbor interpolation
// and does not perform shading.
template <class T>
static void CastRay_NN_Unshaded( vtkCompositeVolumeRayCaster *mapper, 
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
  float           *COTF;
  float           *CTF;
  int             offset;
  int             steps_this_ray = 0;
 
  COTF =  mapper->CorrectedOpacityTFArray;
  CTF =  mapper->ColorTFArray;


  // Move the increments into local variables
  xinc = mapper->DataIncrement[0];
  yinc = mapper->DataIncrement[1];
  zinc = mapper->DataIncrement[2];

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
  opacity = COTF[value];
  
  // Keep track of previous voxel to know when we step into a new one
  prev_voxel[0] = voxel[0];
  prev_voxel[1] = voxel[1];
  prev_voxel[2] = voxel[2];
  

  // Two cases - we are working with a single color or a color transfer
  // function - break them up to make it more efficient
  if ( mapper->ColorType == VTK_SINGLE_COLOR ) 
    {
    // For each step along the ray
    for ( loop = 0; 
          loop < num_steps && remaining_opacity > VTK_REMAINING_OPACITY; loop++ )
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
	opacity = COTF[value];
	
	prev_voxel[0] = voxel[0];
	prev_voxel[1] = voxel[1];
	prev_voxel[2] = voxel[2];
	}
    
    
      // Accumulate some light intensity and opacity
      accum_intensity += opacity * remaining_opacity;
      remaining_opacity *= (1.0 - opacity);
      
      // Increment our position and compute our voxel location
      ray_position[0] += ray_increment[0];
      ray_position[1] += ray_increment[1];
      ray_position[2] += ray_increment[2];
      voxel[0] = vtkRoundFuncMacro( ray_position[0] );
      voxel[1] = vtkRoundFuncMacro( ray_position[1] );
      voxel[2] = vtkRoundFuncMacro( ray_position[2] );
      }
    accum_red_intensity   = accum_intensity * mapper->SingleColor[0];
    accum_green_intensity = accum_intensity * mapper->SingleColor[1];
    accum_blue_intensity  = accum_intensity * mapper->SingleColor[2];
    }
  else if ( mapper->ColorType == VTK_TRANSFER_FUNCTION )
    {
    // For each step along the ray
    for ( loop = 0; 
          loop < num_steps && remaining_opacity > VTK_REMAINING_OPACITY; loop++ )
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
	opacity = COTF[value];
	
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
static void CastRay_NN_Shaded( vtkCompositeVolumeRayCaster *mapper, 
			       T *data_ptr,
			       float ray_start[3], 
			       float ray_increment[3],
			       int num_steps, float pixel_value[6] )
{
  int             value;
  float           accum_red_intensity;
  float           accum_green_intensity;
  float           accum_blue_intensity;
  float           remaining_opacity;
  float           opacity;
  int             loop;
  int             xinc, yinc, zinc;
  int             voxel[3];
  float           ray_position[3];
  int             prev_voxel[3];
  float           *COTF;
  float           *CTF;
  float           *red_d_shade, *green_d_shade, *blue_d_shade;
  float           *red_s_shade, *green_s_shade, *blue_s_shade;
  unsigned short  *encoded_normals;
  float           red_shaded_value, green_shaded_value, blue_shaded_value;
  int             offset;
  float           single_r, single_g, single_b;
  int             steps_this_ray = 0;
 
  // Get diffuse shading table pointers
  red_d_shade = mapper->NormalEncoder.GetRedDiffuseShadingTable();
  green_d_shade = mapper->NormalEncoder.GetGreenDiffuseShadingTable();
  blue_d_shade = mapper->NormalEncoder.GetBlueDiffuseShadingTable();

  // Get specular shading table pointers
  red_s_shade = mapper->NormalEncoder.GetRedSpecularShadingTable();
  green_s_shade = mapper->NormalEncoder.GetGreenSpecularShadingTable();
  blue_s_shade = mapper->NormalEncoder.GetBlueSpecularShadingTable();

  // Get a pointer to the encoded normals for this volume
  encoded_normals = mapper->NormalEncoder.GetEncodedNormals();

  // Get the opacity transfer function for this volume (which maps
  // scalar input values to opacities)
  COTF =  mapper->CorrectedOpacityTFArray;

  // Get the color transfer function for this volume (which maps
  // scalar input values to RGB values)
  CTF =  mapper->ColorTFArray;

  // Move the increments into local variables
  xinc = mapper->DataIncrement[0];
  yinc = mapper->DataIncrement[1];
  zinc = mapper->DataIncrement[2];

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

  // Set up the data values for the first pass through the loop
  offset = voxel[2] * zinc + voxel[1] * yinc + voxel[0];
  value = *(data_ptr + offset);

  // Compute the opacity, and shaded value (r,g,b) of this voxel
  opacity = COTF[value];
  // Two cases - we are working with a single color or a color transfer
  // function - break them up to make it more efficient
  if ( mapper->ColorType == VTK_SINGLE_COLOR ) 
    {
      single_r = mapper->SingleColor[0];
      single_g = mapper->SingleColor[1];
      single_b = mapper->SingleColor[2];

      if ( opacity )
	{
	  red_shaded_value = opacity *  remaining_opacity *
	    ( red_d_shade[*(encoded_normals + offset)] * single_r + 
	      red_s_shade[*(encoded_normals + offset)] );
	  green_shaded_value = opacity *  remaining_opacity *
	    ( green_d_shade[*(encoded_normals + offset)] * single_g + 
	      green_s_shade[*(encoded_normals + offset)] );
	  blue_shaded_value = opacity *  remaining_opacity *
	    ( blue_d_shade[*(encoded_normals + offset)] * single_b +
	      blue_s_shade[*(encoded_normals + offset)] );
	}
      else
	{
	  red_shaded_value = 0.0;
	  green_shaded_value = 0.0;
	  blue_shaded_value = 0.0;
	}
    }
  else  if ( mapper->ColorType == VTK_TRANSFER_FUNCTION )
    {
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
    }
  
  // Keep track of previous voxel to know when we step into a new one  
  prev_voxel[0] = voxel[0];
  prev_voxel[1] = voxel[1];
  prev_voxel[2] = voxel[2];
  
  // Two cases - we are working with a single color or a color transfer
  // function - break them up to make it more efficient
  if ( mapper->ColorType == VTK_SINGLE_COLOR ) 
    {
    single_r = mapper->SingleColor[0];
    single_g = mapper->SingleColor[1];
    single_b = mapper->SingleColor[2];
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
	
	opacity = COTF[value];
	if ( opacity ) 
	  {
	  red_shaded_value = opacity *  remaining_opacity *
	    ( red_d_shade[*(encoded_normals + offset)] * single_r +
	      red_s_shade[*(encoded_normals + offset)] );
	  green_shaded_value = opacity *  remaining_opacity *
	    ( green_d_shade[*(encoded_normals + offset)] * single_g +
	      green_s_shade[*(encoded_normals + offset)] );
	  blue_shaded_value = opacity *  remaining_opacity *
	    ( blue_d_shade[*(encoded_normals + offset)] * single_b +
	      blue_s_shade[*(encoded_normals + offset)] );
 	  }

	prev_voxel[0] = voxel[0];
	prev_voxel[1] = voxel[1];
	prev_voxel[2] = voxel[2];
	}
    
    
      // Accumulate the shaded intensity and opacity of this sample
      if ( opacity )
	{
	accum_red_intensity += red_shaded_value;
	accum_green_intensity += green_shaded_value;
	accum_blue_intensity += blue_shaded_value;
	remaining_opacity *= (1.0 - opacity);
	}

      // Increment our position and compute our voxel location
      ray_position[0] += ray_increment[0];
      ray_position[1] += ray_increment[1];
      ray_position[2] += ray_increment[2];
      voxel[0] = vtkRoundFuncMacro( ray_position[0] );
      voxel[1] = vtkRoundFuncMacro( ray_position[1] );
      voxel[2] = vtkRoundFuncMacro( ray_position[2] );
      }

    }
  else if ( mapper->ColorType == VTK_TRANSFER_FUNCTION )
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
	
	opacity = COTF[value];
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
static void CastRay_TrilinVertices_Unshaded( 
					  vtkCompositeVolumeRayCaster *mapper, 
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
  int             prev_voxel[3];
  float           A, B, C, D, E, F, G, H;
  int             Binc, Cinc, Dinc, Einc, Finc, Ginc, Hinc;
  T               *dptr;
  float           *COTF;
  float           *CTF;
  float           x, y, z, t1, t2, t3;
  float           tA, tB, tC, tD, tE, tF, tG, tH;
  int             offset;
  int             steps_this_ray = 0;
 
  // Get the opacity transfer function which maps scalar input values
  // to opacities
  COTF =  mapper->CorrectedOpacityTFArray;

  // Get the color transfer function which maps scalar input values
  // to RGB colors
  CTF =  mapper->ColorTFArray;

  // Move the increments into local variables
  xinc = mapper->DataIncrement[0];
  yinc = mapper->DataIncrement[1];
  zinc = mapper->DataIncrement[2];

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
  
  // Compute the values for the first pass through the loop
  offset = voxel[2] * zinc + voxel[1] * yinc + voxel[0];
  dptr = data_ptr + offset;

  A = COTF[(*(dptr))];
  B = COTF[(*(dptr + Binc))];
  C = COTF[(*(dptr + Cinc))];
  D = COTF[(*(dptr + Dinc))];
  E = COTF[(*(dptr + Einc))];
  F = COTF[(*(dptr + Finc))];
  G = COTF[(*(dptr + Ginc))];
  H = COTF[(*(dptr + Hinc))];  

  // Keep track of previous voxel to know when we step into a new one  
  prev_voxel[0] = voxel[0];
  prev_voxel[1] = voxel[1];
  prev_voxel[2] = voxel[2];

  // Two cases - we are working with a single color or a color transfer
  // function - break them up to make it more efficient
  if ( mapper->ColorType == VTK_SINGLE_COLOR ) 
    {
    // For each step along the ray
    for ( loop = 0; 
          loop < num_steps && remaining_opacity > VTK_REMAINING_OPACITY; loop++ )
      {	    
      // We've taken another step
      steps_this_ray++;
      
      // Have we moved into a new voxel? If so we need to recompute A-H
      if ( prev_voxel[0] != voxel[0] ||
	   prev_voxel[1] != voxel[1] ||
	   prev_voxel[2] != voxel[2] )
	{
	offset = voxel[2] * zinc + voxel[1] * yinc + voxel[0];
	dptr = data_ptr + offset;
	
	A = COTF[(*(dptr))];
	B = COTF[(*(dptr + Binc))];
	C = COTF[(*(dptr + Cinc))];
	D = COTF[(*(dptr + Dinc))];
	E = COTF[(*(dptr + Einc))];
	F = COTF[(*(dptr + Finc))];
	G = COTF[(*(dptr + Ginc))];
	H = COTF[(*(dptr + Hinc))];
	
	prev_voxel[0] = voxel[0];
	prev_voxel[1] = voxel[1];
	prev_voxel[2] = voxel[2];
	}
      
      // Compute our offset in the voxel, and use that to trilinearly
      // interpolate the value
      x = ray_position[0] - (float) voxel[0];
      y = ray_position[1] - (float) voxel[1];
      z = ray_position[2] - (float) voxel[2];
      
      t1 = 1.0 - x;
      t2 = 1.0 - y;
      t3 = 1.0 - z;
      
      // For this sample we have do not yet have any opacity
      opacity = 0.0;
      
      // Now add the opacity in vertex by vertex.  If any of the A-H
      // have a non-transparent opacity value, then add its contribution
      // to the opacity
      if ( A )
	{
	tA = t1*t2*t3;
	opacity += A * tA;
	}
      
      if ( B )
	{
	tB = x*t2*t3;
	opacity += B * tB;
	}
      
      if ( C )
	{
	tC = t1*y*t3;
	opacity += C * tC;
	}
      
      if ( D )
	{
	tD = x*y*t3;
	opacity += D * tD;
	}
      
      if ( E )
	{
	tE = t1*t2*z;
	opacity += E * tE;
	}
      
      if ( F )
	{
	tF = x*z*t2;
	opacity += F * tF;
	}
      
      if ( G )
	{
	tG = t1*y*z;
	opacity += G * tG;
	} 
      
      if ( H )
	{
	tH = x*z*y;
	opacity += H * tH;
	}
      
      // Accumulate intensity and opacity for this sample location
      accum_intensity += opacity * remaining_opacity;
      remaining_opacity *= (1.0 - opacity);
      
      // Increment our position and compute our voxel location
      ray_position[0] += ray_increment[0];
      ray_position[1] += ray_increment[1];
      ray_position[2] += ray_increment[2];      
      voxel[0] = (int)( ray_position[0] );
      voxel[1] = (int)( ray_position[1] );
      voxel[2] = (int)( ray_position[2] );
      }
    accum_red_intensity   = accum_intensity * mapper->SingleColor[0];
    accum_green_intensity = accum_intensity * mapper->SingleColor[1];
    accum_blue_intensity  = accum_intensity * mapper->SingleColor[2];
    }
  else if ( mapper->ColorType == VTK_TRANSFER_FUNCTION )
    {
    // For each step along the ray
    for ( loop = 0; 
          loop < num_steps && remaining_opacity > VTK_REMAINING_OPACITY; loop++ )
      {	    
      // We've taken another step
      steps_this_ray++;
      
      // Have we moved into a new voxel? If so we need to recompute A-H
      if ( prev_voxel[0] != voxel[0] ||
	   prev_voxel[1] != voxel[1] ||
	   prev_voxel[2] != voxel[2] )
	{
	offset = voxel[2] * zinc + voxel[1] * yinc + voxel[0];
	dptr = data_ptr + offset;
	
	A = COTF[(*(dptr))];
	B = COTF[(*(dptr + Binc))];
	C = COTF[(*(dptr + Cinc))];
	D = COTF[(*(dptr + Dinc))];
	E = COTF[(*(dptr + Einc))];
	F = COTF[(*(dptr + Finc))];
	G = COTF[(*(dptr + Ginc))];
	H = COTF[(*(dptr + Hinc))];
	
	prev_voxel[0] = voxel[0];
	prev_voxel[1] = voxel[1];
	prev_voxel[2] = voxel[2];
	}
      
      // Compute our offset in the voxel, and use that to trilinearly
      // interpolate the value
      x = ray_position[0] - (float) voxel[0];
      y = ray_position[1] - (float) voxel[1];
      z = ray_position[2] - (float) voxel[2];
      
      t1 = 1.0 - x;
      t2 = 1.0 - y;
      t3 = 1.0 - z;
      
      // For this sample we have do not yet have any opacity
      opacity     = 0.0;
      red_value   = 0.0;
      green_value = 0.0;
      blue_value  = 0.0;
      
      // Now add the opacity in vertex by vertex.  If any of the A-H
      // have a non-transparent opacity value, then add its contribution
      // to the opacity
      if ( A )
	{
	tA = t1*t2*t3;
	opacity += A * tA;
	red_value   += A * tA * CTF[((*dptr)) * 3    ];
	green_value += A * tA * CTF[((*dptr)) * 3 + 1];
	blue_value  += A * tA * CTF[((*dptr)) * 3 + 2];
	}
      
      if ( B )
	{
	tB = x*t2*t3;
	opacity += B * tB;
	red_value   += B * tB * CTF[((*(dptr + Binc))) * 3    ];
	green_value += B * tB * CTF[((*(dptr + Binc))) * 3 + 1];
	blue_value  += B * tB * CTF[((*(dptr + Binc))) * 3 + 2];
	}
      
      if ( C )
	{
	tC = t1*y*t3;
	opacity += C * tC;
	red_value   += C * tC * CTF[((*(dptr + Cinc))) * 3    ];
	green_value += C * tC * CTF[((*(dptr + Cinc))) * 3 + 1];
	blue_value  += C * tC * CTF[((*(dptr + Cinc))) * 3 + 2];
	}
      
      if ( D )
	{
	tD = x*y*t3;
	opacity += D * tD;
	red_value   += D * tD * CTF[((*(dptr + Dinc))) * 3    ];
	green_value += D * tD * CTF[((*(dptr + Dinc))) * 3 + 1];
	blue_value  += D * tD * CTF[((*(dptr + Dinc))) * 3 + 2];
	}
      
      if ( E )
	{
	tE = t1*t2*z;
	opacity += E * tE;
	red_value   += E * tE * CTF[((*(dptr + Einc))) * 3    ];
	green_value += E * tE * CTF[((*(dptr + Einc))) * 3 + 1];
	blue_value  += E * tE * CTF[((*(dptr + Einc))) * 3 + 2];
	}
      
      if ( F )
	{
	tF = x*z*t2;
	opacity += F * tF;
	red_value   += F * tF * CTF[((*(dptr + Finc))) * 3    ];
	green_value += F * tF * CTF[((*(dptr + Finc))) * 3 + 1];
	blue_value  += F * tF * CTF[((*(dptr + Finc))) * 3 + 2];
	}
      
      if ( G )
	{
	tG = t1*y*z;
	opacity += G * tG;
	red_value   += G * tG * CTF[((*(dptr + Ginc))) * 3    ];
	green_value += G * tG * CTF[((*(dptr + Ginc))) * 3 + 1];
	blue_value  += G * tG * CTF[((*(dptr + Ginc))) * 3 + 2];
	} 
      
      if ( H )
	{
	tH = x*z*y;
	opacity += H * tH;
	red_value   += H * tH * CTF[((*(dptr + Hinc))) * 3    ];
	green_value += H * tH * CTF[((*(dptr + Hinc))) * 3 + 1];
	blue_value  += H * tH * CTF[((*(dptr + Hinc))) * 3 + 2];
	}
      
      // Accumulate intensity and opacity for this sample location
      accum_red_intensity   += remaining_opacity * red_value;
      accum_green_intensity += remaining_opacity * green_value;
      accum_blue_intensity  += remaining_opacity * blue_value;
      remaining_opacity *= (1.0 - opacity);
      
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
static void CastRay_TrilinVertices_Shaded( 
					  vtkCompositeVolumeRayCaster *mapper, 
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
  int             prev_voxel[3];
  float           A, B, C, D, E, F, G, H;
  int             Binc, Cinc, Dinc, Einc, Finc, Ginc, Hinc;
  T               *dptr;
  float           *COTF;
  float           *CTF;
  float           x, y, z, t1, t2, t3;
  float           tA, tB, tC, tD, tE, tF, tG, tH;
  float           *red_d_shade, *green_d_shade, *blue_d_shade;
  float           *red_s_shade, *green_s_shade, *blue_s_shade;
  unsigned short  *encoded_normals, *nptr;
  float           red_shaded_value, green_shaded_value, blue_shaded_value;
  int             offset;
  float           single_r, single_g, single_b;
  int             steps_this_ray = 0;

  // Get diffuse shading table pointers
  red_d_shade = mapper->NormalEncoder.GetRedDiffuseShadingTable();
  green_d_shade = mapper->NormalEncoder.GetGreenDiffuseShadingTable();
  blue_d_shade = mapper->NormalEncoder.GetBlueDiffuseShadingTable();


  // Get diffuse shading table pointers
  red_s_shade = mapper->NormalEncoder.GetRedSpecularShadingTable();
  green_s_shade = mapper->NormalEncoder.GetGreenSpecularShadingTable();
  blue_s_shade = mapper->NormalEncoder.GetBlueSpecularShadingTable();

  // Get a pointer to the encoded normals for this volume
  encoded_normals = mapper->NormalEncoder.GetEncodedNormals();

  // Get the opacity transfer function which maps scalar input values
  // to opacities
  COTF =  mapper->CorrectedOpacityTFArray;

  // Get the color transfer function which maps scalar input values
  // to RGB values
  CTF =  mapper->ColorTFArray;

  // Move the increments into local variables
  xinc = mapper->DataIncrement[0];
  yinc = mapper->DataIncrement[1];
  zinc = mapper->DataIncrement[2];

  // Initialize the ray position and voxel location
  ray_position[0] = ray_start[0];
  ray_position[1] = ray_start[1];
  ray_position[2] = ray_start[2];
  voxel[0] = (int)( ray_position[0] );
  voxel[1] = (int)( ray_position[1] );
  voxel[2] = (int)( ray_position[2] );

  // So far we haven't accumulated anything
  accum_red_intensity   = 0.0;
  accum_green_intensity   = 0.0;
  accum_blue_intensity   = 0.0;
  remaining_opacity = 1.0;

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

  A = COTF[(*(dptr))];
  B = COTF[(*(dptr + Binc))];
  C = COTF[(*(dptr + Cinc))];
  D = COTF[(*(dptr + Dinc))];
  E = COTF[(*(dptr + Einc))];
  F = COTF[(*(dptr + Finc))];
  G = COTF[(*(dptr + Ginc))];
  H = COTF[(*(dptr + Hinc))];
 
  // Keep track of previous voxel to know when we step into a new one   
  prev_voxel[0] = voxel[0];
  prev_voxel[1] = voxel[1];
  prev_voxel[2] = voxel[2];

  // Two cases - we are working with a single color or a color transfer
  // function - break them up to make it more efficient
  if ( mapper->ColorType == VTK_SINGLE_COLOR ) 
    {
    single_r = mapper->SingleColor[0];
    single_g = mapper->SingleColor[1];
    single_b = mapper->SingleColor[2];

    // For each step along the ray
    for ( loop = 0; 
          loop < num_steps && remaining_opacity > VTK_REMAINING_OPACITY; 
	  loop++ )
      {	    
      // We've taken another step
      steps_this_ray++;
      
      // Have we moved into a new voxel? If so we need to recompute A-H
      if ( prev_voxel[0] != voxel[0] ||
	   prev_voxel[1] != voxel[1] ||
	   prev_voxel[2] != voxel[2] )
	{
	offset = voxel[2] * zinc + voxel[1] * yinc + voxel[0];
	dptr = data_ptr + offset;
	nptr = encoded_normals + offset;
	
	A = COTF[(*(dptr))];
	B = COTF[(*(dptr + Binc))];
	C = COTF[(*(dptr + Cinc))];
	D = COTF[(*(dptr + Dinc))];
	E = COTF[(*(dptr + Einc))];
	F = COTF[(*(dptr + Finc))];
	G = COTF[(*(dptr + Ginc))];
	H = COTF[(*(dptr + Hinc))];
	
	prev_voxel[0] = voxel[0];
	prev_voxel[1] = voxel[1];
	prev_voxel[2] = voxel[2];
	}
      
      // Compute our offset in the voxel, and use that to trilinearly
      // interpolate a value
      x = ray_position[0] - (float) voxel[0];
      y = ray_position[1] - (float) voxel[1];
      z = ray_position[2] - (float) voxel[2];
      
      t1 = 1.0 - x;
      t2 = 1.0 - y;
      t3 = 1.0 - z;
      
      // For this sample we have do not yet have any opacity or
      // shaded intensity yet
      opacity = 0.0;
      red_shaded_value = 0.0;
      green_shaded_value = 0.0;
      blue_shaded_value = 0.0;
      
      // Now add the opacity and shaded intensity value in vertex by 
      // vertex.  If any of the A-H have a non-transparent opacity value, 
      // then add its contribution to the opacity
      if ( A )
	{
	tA = t1*t2*t3 * A;
	opacity += tA;
	red_shaded_value   += tA * ( red_d_shade[ *(nptr) ] * 
				     single_r + 
				     red_s_shade[ *(nptr) ] );
	green_shaded_value += tA * ( green_d_shade[ *(nptr) ] * 
				     single_g + + 
				     green_s_shade[ *(nptr) ] );
	blue_shaded_value  += tA * ( blue_d_shade[ *(nptr) ] * 
				     single_b +
				     blue_s_shade[ *(nptr) ]  );
	}
      
      if ( B )
	{
	tB = x*t2*t3 * B;
	opacity += tB;
	red_shaded_value   += tB * ( red_d_shade[ *(nptr + Binc) ] * 
				     single_r + 
				     red_s_shade[ *(nptr + Binc) ] );
	green_shaded_value += tB * ( green_d_shade[ *(nptr + Binc) ] *
				     single_g + 
				     green_s_shade[ *(nptr + Binc) ] );
	blue_shaded_value  += tB * ( blue_d_shade[ *(nptr + Binc) ] *
				     single_b +
				     blue_s_shade[ *(nptr + Binc) ] );
	}
      
      if ( C )
	{
	tC = t1*y*t3 * C;
	opacity += tC;
	red_shaded_value   += tC * ( red_d_shade[ *(nptr + Cinc) ] *
				     single_r + 
				     red_s_shade[ *(nptr + Cinc) ] );
	green_shaded_value += tC * ( green_d_shade[ *(nptr + Cinc) ] *
				     single_g + 
				     green_s_shade[ *(nptr + Cinc) ] );
	blue_shaded_value  += tC * ( blue_d_shade[ *(nptr + Cinc) ] *
				     single_b +
				     blue_s_shade[ *(nptr + Cinc) ] );
	}

      if ( D )
	{
	tD = x*y*t3 * D;
	opacity += tD;
	red_shaded_value   += tD * ( red_d_shade[ *(nptr + Dinc) ] *
				     single_r + 
				     red_s_shade[ *(nptr + Dinc) ] );
	green_shaded_value += tD * ( green_d_shade[ *(nptr + Dinc) ] *
				     single_g + 
				     green_s_shade[ *(nptr + Dinc) ] );
	blue_shaded_value  += tD * ( blue_d_shade[ *(nptr + Dinc) ] *
				     single_b +
				     blue_s_shade[ *(nptr + Dinc) ] );
	}
      
      if ( E )
	{
	tE = t1*t2*z * E;
	opacity += tE;
	red_shaded_value   += tE * ( red_d_shade[ *(nptr + Einc) ] *
				     single_r + 
				     red_s_shade[ *(nptr + Einc) ] );
	green_shaded_value += tE * ( green_d_shade[ *(nptr + Einc) ] *
				     single_g + 
				     green_s_shade[ *(nptr + Einc) ] );
	blue_shaded_value  += tE * ( blue_d_shade[ *(nptr + Einc) ] *
				     single_b +
				     blue_s_shade[ *(nptr + Einc) ] );
	}
      
      if ( F )
	{
	tF = x*z*t2 * F;
	opacity += tF;
	red_shaded_value   += tF * ( red_d_shade[ *(nptr + Finc) ] *
				     single_r + 
				     red_s_shade[ *(nptr + Finc) ] );
	green_shaded_value += tF * ( green_d_shade[ *(nptr + Finc) ] *
				     single_g + 
				     green_s_shade[ *(nptr + Finc) ] );
	blue_shaded_value  += tF * ( blue_d_shade[ *(nptr + Finc) ] *
				     single_b +
				     blue_s_shade[ *(nptr + Finc) ] );
	}
      
      if ( G )
	{
	tG = t1*y*z * G;
	opacity += tG;
	red_shaded_value   += tG * ( red_d_shade[ *(nptr + Ginc) ] *
				     single_r + 
				     red_s_shade[ *(nptr + Ginc) ] );
	green_shaded_value += tG * ( green_d_shade[ *(nptr + Ginc) ] *
				     single_g + 
				     green_s_shade[ *(nptr + Ginc) ] );
	blue_shaded_value  += tG * ( blue_d_shade[ *(nptr + Ginc) ] *
				     single_b +
				     blue_s_shade[ *(nptr + Ginc) ] );
      } 
      
      if ( H )
	{
	tH = x*z*y * H;
	opacity += tH;
	red_shaded_value   += tH * ( red_d_shade[ *(nptr + Hinc) ] *
				     single_r + 
				     red_s_shade[ *(nptr + Hinc) ] );
	green_shaded_value += tH * ( green_d_shade[ *(nptr + Hinc) ] *
				     single_g + 
				     green_s_shade[ *(nptr + Hinc) ] );
	blue_shaded_value  += tH * ( blue_d_shade[ *(nptr + Hinc) ] *
				     single_b +
				     blue_s_shade[ *(nptr + Hinc) ] );
	}
      
      // Accumulate intensity and opacity for this sample location   
      accum_red_intensity   += red_shaded_value   * remaining_opacity;
      accum_green_intensity += green_shaded_value * remaining_opacity;
      accum_blue_intensity  += blue_shaded_value  * remaining_opacity;
      remaining_opacity *= (1.0 - opacity);
      
      // Increment our position and compute our voxel location
      ray_position[0] += ray_increment[0];
      ray_position[1] += ray_increment[1];
      ray_position[2] += ray_increment[2];      
      voxel[0] = (int)( ray_position[0] );
      voxel[1] = (int)( ray_position[1] );
      voxel[2] = (int)( ray_position[2] );
      }
    }
  else if ( mapper->ColorType == VTK_TRANSFER_FUNCTION )
    {
    // For each step along the ray
    for ( loop = 0; 
          loop < num_steps && remaining_opacity > VTK_REMAINING_OPACITY; 
	  loop++ )
      {	    
      // We've taken another step
      steps_this_ray++;
      
      // Have we moved into a new voxel? If so we need to recompute A-H
      if ( prev_voxel[0] != voxel[0] ||
	   prev_voxel[1] != voxel[1] ||
	   prev_voxel[2] != voxel[2] )
	{
	offset = voxel[2] * zinc + voxel[1] * yinc + voxel[0];
	dptr = data_ptr + offset;
	nptr = encoded_normals + offset;
	
	A = COTF[(*(dptr))];
	B = COTF[(*(dptr + Binc))];
	C = COTF[(*(dptr + Cinc))];
	D = COTF[(*(dptr + Dinc))];
	E = COTF[(*(dptr + Einc))];
	F = COTF[(*(dptr + Finc))];
	G = COTF[(*(dptr + Ginc))];
	H = COTF[(*(dptr + Hinc))];
	
	prev_voxel[0] = voxel[0];
	prev_voxel[1] = voxel[1];
	prev_voxel[2] = voxel[2];
	}
      
      // Compute our offset in the voxel, and use that to trilinearly
      // interpolate a value
      x = ray_position[0] - (float) voxel[0];
      y = ray_position[1] - (float) voxel[1];
      z = ray_position[2] - (float) voxel[2];
      
      t1 = 1.0 - x;
      t2 = 1.0 - y;
      t3 = 1.0 - z;
      
      // For this sample we have do not yet have any opacity or
      // shaded intensity yet
      opacity = 0.0;
      red_shaded_value = 0.0;
      green_shaded_value = 0.0;
      blue_shaded_value = 0.0;
      
      // Now add the opacity and shaded intensity value in vertex by 
      // vertex.  If any of the A-H have a non-transparent opacity value, 
      // then add its contribution to the opacity
      if ( A )
	{
	tA = t1*t2*t3 * A;
	opacity += tA;
	red_shaded_value   += tA * ( red_d_shade[ *(nptr) ] * 
				     CTF[*(dptr) * 3] + 
				     red_s_shade[ *(nptr) ] );
	green_shaded_value += tA * ( green_d_shade[ *(nptr) ] * 
				     CTF[*(dptr) * 3 + 1] + + 
				     green_s_shade[ *(nptr) ] );
	blue_shaded_value  += tA * ( blue_d_shade[ *(nptr) ] * 
				     CTF[*(dptr) * 3 + 2] +
				     blue_s_shade[ *(nptr) ]  );
	}
      
      if ( B )
	{
	tB = x*t2*t3 * B;
	opacity += tB;
	red_shaded_value   += tB * ( red_d_shade[ *(nptr + Binc) ] * 
				     CTF[*(dptr+Binc) * 3] + 
				     red_s_shade[ *(nptr + Binc) ] );
	green_shaded_value += tB * ( green_d_shade[ *(nptr + Binc) ] *
				     CTF[*(dptr+Binc) * 3 + 1] + 
				     green_s_shade[ *(nptr + Binc) ] );
	blue_shaded_value  += tB * ( blue_d_shade[ *(nptr + Binc) ] *
				     CTF[*(dptr+Binc) * 3 + 2] +
				     blue_s_shade[ *(nptr + Binc) ] );
	}
      
      if ( C )
	{
	tC = t1*y*t3 * C;
	opacity += tC;
	red_shaded_value   += tC * ( red_d_shade[ *(nptr + Cinc) ] *
				     CTF[*(dptr+Cinc) * 3] + 
				     red_s_shade[ *(nptr + Cinc) ] );
	green_shaded_value += tC * ( green_d_shade[ *(nptr + Cinc) ] *
				     CTF[*(dptr+Cinc) * 3 + 1] + 
				     green_s_shade[ *(nptr + Cinc) ] );
	blue_shaded_value  += tC * ( blue_d_shade[ *(nptr + Cinc) ] *
				     CTF[*(dptr+Cinc) * 3 + 2] +
				     blue_s_shade[ *(nptr + Cinc) ] );
	}

      if ( D )
	{
	tD = x*y*t3 * D;
	opacity += tD;
	red_shaded_value   += tD * ( red_d_shade[ *(nptr + Dinc) ] *
				     CTF[*(dptr+Dinc) * 3] + 
				     red_s_shade[ *(nptr + Dinc) ] );
	green_shaded_value += tD * ( green_d_shade[ *(nptr + Dinc) ] *
				     CTF[*(dptr+Dinc) * 3 + 1] + 
				     green_s_shade[ *(nptr + Dinc) ] );
	blue_shaded_value  += tD * ( blue_d_shade[ *(nptr + Dinc) ] *
				     CTF[*(dptr+Dinc) * 3 + 2] +
				     blue_s_shade[ *(nptr + Dinc) ] );
	}
      
      if ( E )
	{
	tE = t1*t2*z * E;
	opacity += tE;
	red_shaded_value   += tE * ( red_d_shade[ *(nptr + Einc) ] *
				     CTF[*(dptr+Einc) * 3] + 
				     red_s_shade[ *(nptr + Einc) ] );
	green_shaded_value += tE * ( green_d_shade[ *(nptr + Einc) ] *
				     CTF[*(dptr+Einc) * 3 + 1] + 
				     green_s_shade[ *(nptr + Einc) ] );
	blue_shaded_value  += tE * ( blue_d_shade[ *(nptr + Einc) ] *
				     CTF[*(dptr+Einc) * 3 + 2] +
				     blue_s_shade[ *(nptr + Einc) ] );
	}
      
      if ( F )
	{
	tF = x*z*t2 * F;
	opacity += tF;
	red_shaded_value   += tF * ( red_d_shade[ *(nptr + Finc) ] *
				     CTF[*(dptr+Finc) * 3] + 
				     red_s_shade[ *(nptr + Finc) ] );
	green_shaded_value += tF * ( green_d_shade[ *(nptr + Finc) ] *
				     CTF[*(dptr+Finc) * 3 + 1] + 
				     green_s_shade[ *(nptr + Finc) ] );
	blue_shaded_value  += tF * ( blue_d_shade[ *(nptr + Finc) ] *
				     CTF[*(dptr+Finc) * 3 + 2] +
				     blue_s_shade[ *(nptr + Finc) ] );
	}
      
      if ( G )
	{
	tG = t1*y*z * G;
	opacity += tG;
	red_shaded_value   += tG * ( red_d_shade[ *(nptr + Ginc) ] *
				     CTF[*(dptr+Ginc) * 3] + 
				     red_s_shade[ *(nptr + Ginc) ] );
	green_shaded_value += tG * ( green_d_shade[ *(nptr + Ginc) ] *
				     CTF[*(dptr+Ginc) * 3 + 1] + 
				     green_s_shade[ *(nptr + Ginc) ] );
	blue_shaded_value  += tG * ( blue_d_shade[ *(nptr + Ginc) ] *
				     CTF[*(dptr+Ginc) * 3 + 2] +
				     blue_s_shade[ *(nptr + Ginc) ] );
      } 
      
      if ( H )
	{
	tH = x*z*y * H;
	opacity += tH;
	red_shaded_value   += tH * ( red_d_shade[ *(nptr + Hinc) ] *
				     CTF[*(dptr+Hinc) * 3] + 
				     red_s_shade[ *(nptr + Hinc) ] );
	green_shaded_value += tH * ( green_d_shade[ *(nptr + Hinc) ] *
				     CTF[*(dptr+Hinc) * 3 + 1] + 
				     green_s_shade[ *(nptr + Hinc) ] );
	blue_shaded_value  += tH * ( blue_d_shade[ *(nptr + Hinc) ] *
				     CTF[*(dptr+Hinc) * 3 + 2] +
				     blue_s_shade[ *(nptr + Hinc) ] );
	}
      
      // Accumulate intensity and opacity for this sample location   
      accum_red_intensity   += red_shaded_value   * remaining_opacity;
      accum_green_intensity += green_shaded_value * remaining_opacity;
      accum_blue_intensity  += blue_shaded_value  * remaining_opacity;
      remaining_opacity *= (1.0 - opacity);
      
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
// This is the templated function that actually casts a ray and computes
// the composite value.  This version uses trilinear interpolation and
// does not compute shading
template <class T>
static void CastRay_TrilinSample_Unshaded( 
					  vtkCompositeVolumeRayCaster *mapper, 
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
  float           *COTF;
  float           *CTF;
  float           x, y, z, t1, t2, t3;
  int             offset;
  int             steps_this_ray = 0;
  float           scalar_value;

  // Get the opacity transfer function which maps scalar input values
  // to opacities
  COTF =  mapper->CorrectedOpacityTFArray;

  // Get the color transfer function which maps scalar input values
  // to RGB colors
  CTF =  mapper->ColorTFArray;

  // Move the increments into local variables
  xinc = mapper->DataIncrement[0];
  yinc = mapper->DataIncrement[1];
  zinc = mapper->DataIncrement[2];

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
  
  // Two cases - we are working with a single color or a color transfer
  // function - break them up to make it more efficient
  if ( mapper->ColorType == VTK_SINGLE_COLOR ) 
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
      else if ( scalar_value > mapper->OpacityTFArraySize - 1 )
	scalar_value = mapper->OpacityTFArraySize - 1;

      opacity = COTF[(int)scalar_value];
      
      // Accumulate intensity and opacity for this sample location
      accum_intensity += opacity * remaining_opacity;
      remaining_opacity *= (1.0 - opacity);
      
      // Increment our position and compute our voxel location
      ray_position[0] += ray_increment[0];
      ray_position[1] += ray_increment[1];
      ray_position[2] += ray_increment[2];      
      voxel[0] = (int)( ray_position[0] );
      voxel[1] = (int)( ray_position[1] );
      voxel[2] = (int)( ray_position[2] );
      }

    accum_red_intensity   = accum_intensity * mapper->SingleColor[0];
    accum_green_intensity = accum_intensity * mapper->SingleColor[1];
    accum_blue_intensity  = accum_intensity * mapper->SingleColor[2];
    }
  else if ( mapper->ColorType == VTK_TRANSFER_FUNCTION )
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
      else if ( scalar_value > mapper->OpacityTFArraySize - 1 )
	scalar_value = mapper->OpacityTFArraySize - 1;

      opacity = COTF[(int)scalar_value];

      red_value   = opacity * CTF[((int)scalar_value) * 3    ];
      green_value = opacity * CTF[((int)scalar_value) * 3 + 1];
      blue_value  = opacity * CTF[((int)scalar_value) * 3 + 2];

      // Accumulate intensity and opacity for this sample location
      accum_red_intensity   += remaining_opacity * red_value;
      accum_green_intensity += remaining_opacity * green_value;
      accum_blue_intensity  += remaining_opacity * blue_value;
      remaining_opacity *= (1.0 - opacity);
      
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
					vtkCompositeVolumeRayCaster *mapper, 
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
  float           *COTF;
  float           *CTF;
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
  red_d_shade = mapper->NormalEncoder.GetRedDiffuseShadingTable();
  green_d_shade = mapper->NormalEncoder.GetGreenDiffuseShadingTable();
  blue_d_shade = mapper->NormalEncoder.GetBlueDiffuseShadingTable();


  // Get diffuse shading table pointers
  red_s_shade = mapper->NormalEncoder.GetRedSpecularShadingTable();
  green_s_shade = mapper->NormalEncoder.GetGreenSpecularShadingTable();
  blue_s_shade = mapper->NormalEncoder.GetBlueSpecularShadingTable();

  // Get a pointer to the encoded normals for this volume
  encoded_normals = mapper->NormalEncoder.GetEncodedNormals();

  // Get the opacity transfer function which maps scalar input values
  // to opacities
  COTF =  mapper->CorrectedOpacityTFArray;

  // Get the color transfer function which maps scalar input values
  // to RGB values
  CTF =  mapper->ColorTFArray;

  // Move the increments into local variables
  xinc = mapper->DataIncrement[0];
  yinc = mapper->DataIncrement[1];
  zinc = mapper->DataIncrement[2];

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

  // Two cases - we are working with a single color or a color transfer
  // function - break them up to make it more efficient
  if ( mapper->ColorType == VTK_SINGLE_COLOR ) 
    {
    r = mapper->SingleColor[0];
    g = mapper->SingleColor[1];
    b = mapper->SingleColor[2];
    }
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
    else if ( scalar_value > mapper->OpacityTFArraySize - 1 )
      scalar_value = mapper->OpacityTFArraySize - 1;
    
    opacity = COTF[scalar_value];
    
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
    
    if ( mapper->ColorType == VTK_TRANSFER_FUNCTION )
      {
      r = CTF[(scalar_value) * 3    ];
      g = CTF[(scalar_value) * 3 + 1];
      b = CTF[(scalar_value) * 3 + 2];
      }

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
      
    // Increment our position and compute our voxel location
    ray_position[0] += ray_increment[0];
    ray_position[1] += ray_increment[1];
    ray_position[2] += ray_increment[2];      
    voxel[0] = (int)( ray_position[0] );
    voxel[1] = (int)( ray_position[1] );
    voxel[2] = (int)( ray_position[2] );
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
// Constructor for the vtkCompositeVolumeRayCaster class
// Initially we have a parc threshold value of 1, shading is
// off, and the opacity transfer function and threshold builder
// are NULL.
vtkCompositeVolumeRayCaster::vtkCompositeVolumeRayCaster()
{
  this->OpacityTransferFunction = NULL;
  this->OpacityTFArray          = NULL;
  this->CorrectedOpacityTFArray = NULL;
  this->CorrectedStepSize	= 0.0;
  this->Shading                 = 0;
  this->Ambient                 = 0.1;
  this->Diffuse                 = 0.8;
  this->Specular                = 0.2;
  this->SpecularPower           = 30.0;
  this->ColorTransferFunction   = NULL;
  this->ColorTFArray            = NULL;
  this->SingleColor[0]          = 1.0;
  this->SingleColor[1]          = 1.0;
  this->SingleColor[2]          = 1.0;
  this->ColorType               = VTK_SINGLE_COLOR;
  this->InterpolationLocation   = VTK_INTERPOLATE_AT_VERTICES;
}

// Description:
// Destruct the vtkCompositeVolumeRayCaster - free up all the
// memory that we have been using
vtkCompositeVolumeRayCaster::~vtkCompositeVolumeRayCaster()
{
  if ( this->OpacityTFArray )
    delete this->OpacityTFArray;

  if ( this->CorrectedOpacityTFArray )
    delete this->CorrectedOpacityTFArray;

  if ( this->ColorTFArray )
    delete this->ColorTFArray;
}

// Description:
// This is called from RenderAnImage (in vtkDepthPARCMapper.cxx)
// It uses the integer data type flag that is passed in to
// determine what type of ray needs to be cast (which is handled
// by a templated function.  It also uses the shading and
// interpolation types to determine which templated function
// to call.
void vtkCompositeVolumeRayCaster::CastARay( int ray_type, void *data_ptr,
					float ray_position[3], 
					float ray_increment[3],
					int num_steps, float pixel_value[6] )
{
  // Cast the ray for the data type and shading/interpolation type
  if ( this->InterpolationType == 0 )
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
	case 2:
	  CastRay_NN_Unshaded( this, (short *)data_ptr, ray_position, 
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
	case 2:
	  CastRay_NN_Shaded( this, (short *)data_ptr, ray_position, 
			     ray_increment, num_steps, pixel_value );
	  break;
	}
      }
    }
  else 
    {
    if ( this->Shading == 0 )
      {
      if ( this->InterpolationLocation == VTK_INTERPOLATE_AT_VERTICES )
	{
        // Trilinear interpolation at vertices and no shading
        switch ( ray_type )
	  {
	  case 0:
	    CastRay_TrilinVertices_Unshaded( this, (unsigned char *)data_ptr, 
					     ray_position, 
					     ray_increment, num_steps, 
					     pixel_value );
	    break;
	  case 1:
	    CastRay_TrilinVertices_Unshaded( this, (unsigned short *)data_ptr, 
					     ray_position, 
					     ray_increment, num_steps, 
					     pixel_value );
	    break;
	  case 2:
	    CastRay_TrilinVertices_Unshaded( this, (short *)data_ptr, 
					     ray_position, 
					     ray_increment, num_steps, 
					     pixel_value );
	    break;
	  }
	}
      else
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
	  case 2:
	    CastRay_TrilinSample_Unshaded( this, (short *)data_ptr, 
					   ray_position, 
					   ray_increment, num_steps, 
					   pixel_value );
	    break;
	  }
	}	
      }
    else
      {
      if ( this->InterpolationLocation == VTK_INTERPOLATE_AT_VERTICES )
	{
        // Trilinear interpolation and shading
        switch ( ray_type )
	  {
	  case 0:
	    CastRay_TrilinVertices_Shaded( this, (unsigned char *)data_ptr, 
					   ray_position, 
					   ray_increment, num_steps, 
					   pixel_value );
	    break;
	  case 1:
	    CastRay_TrilinVertices_Shaded( this, (unsigned short *)data_ptr, 
					   ray_position, 
					   ray_increment, num_steps, 
					   pixel_value );
	    break;
	  case 2:
	    CastRay_TrilinVertices_Shaded( this, (short *)data_ptr, 
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
	  case 2:
	    CastRay_TrilinSample_Shaded( this, (short *)data_ptr, 
					 ray_position, 
					 ray_increment, num_steps, 
					 pixel_value );
	    break;
	  }
	}	
      }
    }
}


// Description:
float vtkCompositeVolumeRayCaster::GetZeroOpacityThreshold()
{
  return( this->OpacityTransferFunction->GetFirstNonZeroValue() );
}

// Description:
// This method computes the corrected alpha blending for a given
// step size.  The OpacityTFArray reflects step size 1.
// The CorrectedOpacityTFArray reflects step size CorrectedStepSize.

void vtkCompositeVolumeRayCaster::UpdateOpacityTFforSampleSize(vtkRenderer *ren,
	vtkVolume *vol) 
  {
  int i;
  int needsRecomputing;
  float originalAlpha,correctedAlpha;
  float ray_scale;
  float volumeScale;
  float interactionScale;
  vtkRayCaster *ray_caster;

  ray_caster = ren->GetRayCaster();
  interactionScale = ray_caster->GetViewportStepSize(ren);
  volumeScale = vol->GetScale();
  ray_scale = this->SampleDistance * interactionScale * volumeScale;


  // step size changed
  needsRecomputing =  
      this->CorrectedStepSize-ray_scale >  0.0001;
  
  needsRecomputing = needsRecomputing || 
      this->CorrectedStepSize-ray_scale < -0.0001;

  if (!needsRecomputing)
    {
    // updated opacity xfer function
    needsRecomputing = needsRecomputing || 
	this->OpacityTFArrayMTime > this->CorrectedOTFArrayMTime;
    }
  if (needsRecomputing)
    {
    this->CorrectedOTFArrayMTime.Modified();
    this->CorrectedStepSize = ray_scale;
    for (i = 0;i < this->OpacityTFArraySize;i++)
      {
      originalAlpha = *(this->OpacityTFArray+i);

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
      *(this->CorrectedOpacityTFArray+i) = correctedAlpha;
      }
    }
  }

// Description:
// This is an update method that is called from Render (in
// vtkDepthPARCMapper.cxx).  It allows the specific mapper type to
// update any local caster variables.  In this case, normals
// are checked to see if they need updating. In addition, if
// shading is on, the shading table is computed, and if the opacity
// transfer function has changed, a new opacity table is created.
void vtkCompositeVolumeRayCaster::CasterUpdate( vtkRenderer *ren, 
						vtkVolume *vol )
{
  float               light_direction[3], material[4], light_color[3];
  float               light_position[3], light_focal_point[3];
  float               light_intensity, view_direction[3];
  float               camera_position[3], camera_focal_point[3], mag;
  vtkLightCollection  *light_collection;
  vtkLight            *light;
  float               norm;
  int                 update_flag;
  char                *data_type;


  // Update the normals if necessary
  if ( this->NormalEncoder.GetEncodedNormals () == NULL ||
       this->NormalEncoder.GetMTime() < this->ScalarInput->GetMTime() )
    {
    this->NormalEncoder.SetScalarInput( this->ScalarInput );
    this->NormalEncoder.UpdateNormals();
    this->NormalEncoder.Modified();
    }

  // If shading is on, update the shading table
  if ( this->Shading )
    {
    material[0] = this->Ambient;
    material[1] = this->Diffuse;
    material[2] = this->Specular;
    material[3] = this->SpecularPower;

    // Set up the lights for traversal
    light_collection = ren->GetLights();
    light_collection->InitTraversal();
    
    // Loop through all lights and compute a shading table. For
    // the first light, pass in an update_flag of 0, which means
    // overwrite the shading table.  For each light after that, pass
    // in an update flag of 1, which means add to the shading table.
    // Currently, all lights are forced to be directional light sources
    // regardless of what they really are - in the future, all lights
    // will be handled - but it will be slow for anything but 
    // directional lights
    update_flag = 0;
    while ( (light = light_collection->GetNextItem()) != NULL  )
      { 
      // Get the light color, position, focal point, and intensity
      light->GetColor( light_color );
      light->GetPosition( light_position );
      light->GetFocalPoint( light_focal_point );
      light_intensity = light->GetIntensity( );

      ren->GetActiveCamera()->GetPosition( camera_position );
      ren->GetActiveCamera()->GetFocalPoint( camera_focal_point );

      view_direction[0] =  camera_focal_point[0] - camera_position[0];
      view_direction[1] =  camera_focal_point[1] - camera_position[1];
      view_direction[2] =  camera_focal_point[2] - camera_position[2];

      mag = sqrt( (double)( 
	view_direction[0] * view_direction[0] + 
	view_direction[1] * view_direction[1] + 
	view_direction[2] * view_direction[2] ) );

      if ( mag )
	{
	view_direction[0] /= mag;
	view_direction[1] /= mag;
	view_direction[2] /= mag;
	}

      // Compute the light direction and normalize it
      light_direction[0] = light_focal_point[0] - light_position[0];
      light_direction[1] = light_focal_point[1] - light_position[1];
      light_direction[2] = light_focal_point[2] - light_position[2];
      
      norm = sqrt( (double) ( light_direction[0] * light_direction[0] + 
			      light_direction[1] * light_direction[1] +
			      light_direction[2] * light_direction[2] ) );
      
      light_direction[0] /= -norm;
      light_direction[1] /= -norm;
      light_direction[2] /= -norm;
      
      // Build / Add to the shading table
      this->NormalEncoder.BuildShadingTable( light_direction, light_color, 
					     light_intensity, view_direction,
					     material, update_flag );
      
      update_flag = 1;
      }
    }
  
  // Update the OpacityTFArray if necessary.  This is necessary if
  // the OpacityTFArray does not exist, or the transfer function has
  // been modified more recently than the OpacityTFArray has.
  data_type = this->ScalarInput->GetPointData()->GetScalars()->GetDataType();
  if ( strcmp( data_type, "unsigned char" ) == 0 )
    {
    if ( this->OpacityTransferFunction == NULL )
      {
      vtkErrorMacro( << "Error: no transfer function!" );
      }
    else if ( this->OpacityTFArray == NULL ||
	      this->OpacityTransferFunction->GetMTime() >
	      this->OpacityTFArrayMTime )
      {
	  //	 Get values 0-255 (256 values)
	  if ( this->OpacityTFArray )
	    {
	      delete this->OpacityTFArray;
	    }

	this->OpacityTFArray = new float[(int)(0x100)];
	this->OpacityTransferFunction->GetTable( (float)(0x00),
						 (float)(0xff),  
						 (int)(0x100), 
						 this->OpacityTFArray );
	this->OpacityTFArraySize = (int)(0x100);
	if ( this->CorrectedOpacityTFArray )
	  {
	    delete this->CorrectedOpacityTFArray;
	  }
	this->CorrectedOpacityTFArray = new float[this->OpacityTFArraySize];
	this->OpacityTFArrayMTime.Modified();
      }
    if ( this->ColorType == VTK_TRANSFER_FUNCTION )
      {
      if ( this->ColorTransferFunction == NULL )
	{
	vtkErrorMacro( << "Error: no color transfer function!" );
	}
      else if ( this->ColorTFArray == NULL ||
		this->ColorTransferFunction->GetMTime() >
		this->ColorTFArrayMTime )
	{
	  // Get values 0-255 (256 values)

	  if ( this->ColorTFArray )
	    {
	      delete this->ColorTFArray;
	    }

	  this->ColorTFArray = new float[3 * (int)(0x100)];
	  this->ColorTransferFunction->GetTable( (float)(0x00),
						 (float)(0xff),  
						 (int)(0x100), 
						 this->ColorTFArray
						 );
	  this->ColorTFArrayMTime.Modified();
	}
      }
    }
  else if ( strcmp( data_type, "unsigned short" ) == 0 ||
            strcmp( data_type, "short" ) == 0 )
    {
    if ( this->OpacityTransferFunction == NULL )
      {
      vtkErrorMacro( << "Error: no transfer function!" );
      }
    else if ( this->OpacityTFArray == NULL ||
	      this->OpacityTransferFunction->GetMTime() >
	      this->OpacityTFArrayMTime )
      {
	// Get values 0-65535 (65536 values)
	if ( this->OpacityTFArray )
	  {
	    delete this->OpacityTFArray;
	  }
	
	this->OpacityTFArray = new float[(int)(0x10000)];
	this->OpacityTransferFunction->GetTable( (float)(0x0000), 
						 (float)(0xffff), 
						 (int)(0x10000),
						 this->OpacityTFArray
						 );
	this->OpacityTFArraySize = (int)(0x10000);
	if ( this->CorrectedOpacityTFArray )
	  {
	    delete this->CorrectedOpacityTFArray;
	  }
	this->CorrectedOpacityTFArray = new float[this->OpacityTFArraySize];
	this->OpacityTFArrayMTime.Modified();
      }
    if ( this->ColorType == VTK_TRANSFER_FUNCTION )
      {
      if ( this->ColorTransferFunction == NULL )
	{
	vtkErrorMacro( << "Error: no color transfer function!" );
	}
      else if ( this->ColorTFArray == NULL ||
		this->ColorTransferFunction->GetMTime() >
		this->ColorTFArrayMTime )
	{
	  // Get values 0-65535 (65536 values)
	  if ( this->ColorTFArray )
	    {
	      delete this->ColorTFArray;
	    }

	  this->ColorTFArray = new float[3 * (int)(0x10000)];
	  this->ColorTransferFunction->GetTable( (float)(0x0000),
					       (float)(0xffff),  
					       (int)(0x10000),
					       this->ColorTFArray
					       );
	  this->ColorTFArrayMTime.Modified();
	}
      }
    }
  // check that the corrected opacity transfer function
  // is update to date with the current step size.
  // Update CorrectedOpacityTFArray if it is required.

  this->UpdateOpacityTFforSampleSize(ren,vol);
}


// Description:
// Print method for vtkCompositeVolumeRayCaster
void vtkCompositeVolumeRayCaster::PrintSelf(ostream& os, vtkIndent indent)
{
  if ( this->Shading )
    os << indent << "Shading: On\n";
  else
    os << indent << "Shading: Off\n";

  if ( this->OpacityTransferFunction )
    {
    os << indent << "Scalar Opacity Transfer Function: (" 
       << this->OpacityTransferFunction << ")\n";
    }
  else
    {
    os << indent << "Scalar Opacity Transfer Function: (none)\n";
    }

  vtkVolumeRayCaster::PrintSelf(os,indent);
}





