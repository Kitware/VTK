/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastIsosurfaceFunction.cxx
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
#include "vtkVolumeRayCastIsosurfaceFunction.h"
#include "vtkCamera.h"
#include "vtkRenderWindow.h"
#include "vtkColorTransferFunction.h"
#include "vtkMath.h"
#include "vtkVolumeProperty.h"
#include "vtkVolume.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkVolumeRayCastIsosurfaceFunction* vtkVolumeRayCastIsosurfaceFunction::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkVolumeRayCastIsosurfaceFunction");
  if(ret)
    {
    return (vtkVolumeRayCastIsosurfaceFunction*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkVolumeRayCastIsosurfaceFunction;
}




/*    Is x between y and z?                                     */
#define VTK_In_Range(x,y,z)      ((x) >= (y) && (x) <= (z))
#define VTK_Floor(x)             (((x) < 0.0)?((int)((x)-1.0)):((int)(x)))
#define VTK_Sign(x)              (((x) < 0.0)?(-1):(1))

#ifndef TRUE
#define TRUE        1
#define FALSE       0
#endif

typedef struct
{
  int             num_intersections;
  float           local_position[3][3];
  float           local_distance[3];
} LineIntersectInfo;

/************************************************************************/
/* 	This routine computes the intersection(s) of a vector and an 	*/
/* isosurface within the trilinear interpolation function. The starting	*/
/* position of the vector is given in variable "start" and the direction*/
/* of the vector is given in the variable "vec". The scalar values at 	*/
/* the vertices of the [0.0 <-> 1.0] cube are supplied within variables */
/* "A"-"H" (See macro Trilin()).					*/
/*									*/
/*	Scalar Field:							*/
/*									*/
/*			Trilin( x, y, z, A, B, C, D, E, F, G, H )	*/
/*									*/
/* 	Parametric Line Equation:					*/
/*									*/
/* 			x = x0 + at					*/
/* 			y = y0 + bt					*/
/* 			z = z0 + ct					*/
/*									*/
/*	Isosurface Threshold Value:					*/
/*									*/
/*			iso						*/
/*									*/
/*	Intermediate Calculations:					*/
/*									*/
/*			P =  A - B - C + D				*/
/*			Q =  A - C - E + G				*/
/*			R =  A - B - E + F				*/
/*			S = -A + B + C - D + E - F - G + H		*/
/*			T = a * b * c * S				*/
/*									*/
/*	Trilinear Interpolation With Parametric Substitutions:		*/
/*									*/
/*			c0*t^3 + c1*t^2 + c2*t + c3 = 0			*/
/*									*/
/*	Where:								*/
/*									*/
/*	c0 = a*b*c*S							*/
/*									*/
/*      c1 = a*b*P + b*c*Q + a*c*R + (x0*b*c + a*(y0*c + z0*b))*S	*/
/*									*/
/*	c2 = (x0*b + y0*a)*P + (y0*c + z0*b)*Q + (x0*c + z0*a)*R +	*/
/*	     (a*y0*z0 + x0*(y0*c + z0*b))*S + 				*/
/*	     (B - A)*a + (C - A)*b + (E - A)*c				*/
/*									*/
/*	c3 = (1.0-x0-y0-z0)*A + B*x0 + C*y0 + E*z0 + 			*/
/*	      x0*y0*P + y0*z0*Q + x0*z0*R + x0*y0*z0*S - iso		*/
/*									*/
/************************************************************************/

void trilin_line_intersection( float start[3], float vec[3], 
			       double A, double B, double C, double D, 
			       double E, double F, double G, double H, 
			       double iso, LineIntersectInfo *solution )
{
  double	c0, c1, c2, c3;	/* Coefficients Of Cubic Equation */
  double	r1, r2, r3;	/* Roots Of Equation */
  double	temp;		/* Swap Variable */
  int	        num_roots;	/* Number Of Unique Roots To Equation */
  int	        root;		/* Loops Through Roots */
  int	        pos_dist_num;	/* Number Of Positive Distance Roots */
  
  double	x0, y0, z0;
  double	a, b, c;
  double	P, Q, R, S, T;
  double	x, y, z;
  double	dist = 0; 
  //  This are used for alternative approach that is commented now  
  //  double ab, bc, ac;
  //  double x0y0, y0z0, x0z0;
  //  double z0S;
  
  x0 = start[0];
  y0 = start[1];
  z0 = start[2];

  a  = vec[0];
  b  = vec[1];
  c  = vec[2];

  /* Precision problem - this quantizes the ray direction */
  /* which keeps c0 from becoming too small */
  a = (float)((int)(a * 100000.0))/100000.0;
  b = (float)((int)(b * 100000.0))/100000.0;
  c = (float)((int)(c * 100000.0))/100000.0;
  
  P =  A - B - C + D;
  Q =  A - C - E + G;
  R =  A - B - E + F;
  S = -A + B + C - D + E - F - G + H;
  T =  a * b * c * S;
  
  /* Initialize the Number Of Intersections To Zero */
  solution->num_intersections = 0;
  
  /* 41 mults & 30 adds */
  c0 = T;
  
  c1 = (a*b*P + b*c*Q + a*c*R + (x0*b*c + a*(y0*c + z0*b))*S);
  
  c2 = ( (x0*b + y0*a)*P + (y0*c + z0*b)*Q + (x0*c + z0*a)*R +
	 (a*y0*z0 + x0*(y0*c + z0*b))*S + 
	 (B - A)*a + (C - A)*b + (E - A)*c
	 );
  
  c3 = ( (1.0-x0-y0-z0)*A + B*x0 + C*y0 + E*z0 + 
	 x0*y0*P + y0*z0*Q + x0*z0*R + x0*y0*z0*S - iso
	 );
  
  /* 36 mults & 28 adds */
  /***
    ab   =  a * b;
    bc   =  b * c;
    ac   =  a * c;
    
    x0y0 = x0 * y0;
    y0z0 = y0 * z0;
    x0z0 = x0 * z0;
    
    z0S  = z0 * S;
    
    c0 = T;
    
    c1 = (ab*P + bc*Q + ac*R + (x0*bc + y0*ac)*S + z0S*ab);
    
    c2 = ( (x0*b + y0*a)*P + (y0*c + z0*b)*Q + (x0*c + z0*a)*R +
    (a*y0z0 + x0y0*c + x0z0*b)*S + 
    (B - A)*a + (C - A)*b + (E - A)*c
    );
    
    c3 = ( (1.0-x0-y0-z0)*A + B*x0 + C*y0 + E*z0 + 
    x0y0*P + y0z0*Q + x0z0*R + x0y0*z0S - iso
    );
    
    ****/
  if ( (c0 >= 0.0 && c1 >= 0.0 && c2 >= 0.0 && c3 >= 0.0) 
       || (c0 <= 0.0 && c1 <= 0.0 && c2 <= 0.0 && c3 <= 0.0))
    {
    return;
    }
  
  vtkMath::SolveCubic( c0, c1, c2, c3, &r1, &r2, &r3, &num_roots );
  
  /* Remove Negative Solutions And Store In Distance Array */
  pos_dist_num = 0;
  for( root=0; root < num_roots; root++ )
    {
      switch( root )
	{
	case 0:
	  dist = r1;
	  break;
	case 1:
	  dist = r2;
	  break;
	case 2:
	  dist = r3;
	}
      
      if( dist >= 0.0 )
	{
	  solution->local_distance[pos_dist_num] = dist;
	  pos_dist_num += 1;
	}
    }
  
  solution->num_intersections = pos_dist_num;
  
  /* Sort The Solutions Based On Distance */
  if( pos_dist_num == 2 )
    {
      if( solution->local_distance[0] > solution->local_distance[1] )
	{
	  temp = solution->local_distance[0];
	  solution->local_distance[0] = solution->local_distance[1];
	  solution->local_distance[1] = temp;
	}
    }
  else if( pos_dist_num == 3 )
    {
      if( solution->local_distance[0] > solution->local_distance[1] )
	{
	  temp = solution->local_distance[0];
	  solution->local_distance[0] = solution->local_distance[1];
	  solution->local_distance[1] = temp;
	}
      if( solution->local_distance[1] > solution->local_distance[2] )
	{
	  temp = solution->local_distance[1];
	  solution->local_distance[1] = solution->local_distance[2];
	  solution->local_distance[2] = temp;
	}
      if( solution->local_distance[0] > solution->local_distance[1] )
	{
	  temp = solution->local_distance[0];
	  solution->local_distance[0] = solution->local_distance[1];
	  solution->local_distance[1] = temp;
	}
    }
  
  for( root=0; root < solution->num_intersections; root++ )
    {
      /**********************************************/
      /* Determine The (x,y,z) Position Of Solution */
      /**********************************************/
      x = x0 + a * solution->local_distance[root];
      y = y0 + b * solution->local_distance[root];
      z = z0 + c * solution->local_distance[root];
      
      solution->local_position[root][0] = x;
      solution->local_position[root][1] = y;
      solution->local_position[root][2] = z;
    }
}

// This is the templated function that actually casts a ray and computes
// the pixel_value for isosurface-ray intersection.  It is valid for
// unsigned char, unsigned short, short, int and float data.
template <class T>
static void CastRay_NN ( vtkVolumeRayCastIsosurfaceFunction *cast_function, 
			 T *data_ptr,
			 VTKRayCastRayInfo *rayInfo,
			 VTKRayCastVolumeInfo *volumeInfo )
{

  unsigned short  *encoded_normals;
  int       xinc, yinc, zinc;
  int       voxel_x, voxel_y, voxel_z;
  int       end_voxel_x, end_voxel_y, end_voxel_z;
  int       x_voxels, y_voxels, z_voxels;
  int       found_intersection;
  int	    tstep_x, tstep_y, tstep_z;
  int       offset;
  int       steps_this_ray = 0;
  T         A;
  T         *dptr;
  float     ray_position_x, ray_position_y, ray_position_z;
  float     ray_end[3];
  float     ray_direction_x, ray_direction_y, ray_direction_z;
  float	    tmax_x, tmax_y, tmax_z,
            tdelta_x, tdelta_y, tdelta_z;
  float     isovalue;
  float     *red_d_shade, *green_d_shade, *blue_d_shade;
  float     *red_s_shade, *green_s_shade, *blue_s_shade;
  int       num_steps;
  float     *ray_start, *ray_increment;
  float     r, g, b;
  float     volumeRed, volumeGreen, volumeBlue;
  float     texcoord[3];


  num_steps = rayInfo->NumberOfStepsToTake;
  ray_start = rayInfo->TransformedStart;
  ray_increment = rayInfo->TransformedIncrement;

  rayInfo->Color[0] = 0.0;
  rayInfo->Color[1] = 0.0;
  rayInfo->Color[2] = 0.0;
  rayInfo->Color[3] = 0.0;
  rayInfo->NumberOfStepsTaken = 0;

  // Move the increments into local variables
  xinc = volumeInfo->DataIncrement[0];
  yinc = volumeInfo->DataIncrement[1];
  zinc = volumeInfo->DataIncrement[2];

  // Initialize the ray position and voxel location
  ray_position_x = ray_start[0];
  ray_position_y = ray_start[1];
  ray_position_z = ray_start[2];

  voxel_x = VTK_Floor( ray_position_x );
  voxel_y = VTK_Floor( ray_position_y );
  voxel_z = VTK_Floor( ray_position_z );
  
  ray_end[0] = ray_start[0] + num_steps*ray_increment[0];
  ray_end[1] = ray_start[1] + num_steps*ray_increment[1];
  ray_end[2] = ray_start[2] + num_steps*ray_increment[2];

  ray_direction_x = ray_increment[0];
  ray_direction_y = ray_increment[1];
  ray_direction_z = ray_increment[2];

  x_voxels = volumeInfo->DataSize[0];
  y_voxels = volumeInfo->DataSize[1];
  z_voxels = volumeInfo->DataSize[2];
      
  if ( voxel_x >= x_voxels - 1 ||
       voxel_y >= y_voxels - 1 ||
       voxel_z >= z_voxels - 1 ||
       voxel_x < 0 || voxel_y < 0 || voxel_z < 0 )
  {
    return;
  }

  // Set the local variable to be isovalue for the surface
  isovalue = cast_function->IsoValue;

  tstep_x = VTK_Sign( ray_direction_x );
  tstep_y = VTK_Sign( ray_direction_y );
  tstep_z = VTK_Sign( ray_direction_z );
  
  end_voxel_x = (int)ray_end[0] + tstep_x;
  end_voxel_y = (int)ray_end[1] + tstep_y;
  end_voxel_z = (int)ray_end[2] + tstep_z;
  
  if (ray_direction_x != 0.0)
    {
      tmax_x = fabs((float)((voxel_x+(tstep_x==1)) - ray_position_x) / 
		    ray_direction_x);
      tdelta_x = fabs(1.0 / ray_direction_x);
    }
  else
    {
      tmax_x = VTK_LARGE_FLOAT;
      tdelta_x = VTK_LARGE_FLOAT;
    }
  
  if (ray_direction_y != 0.0)
    {
      tmax_y = fabs((float)((voxel_y+(tstep_y==1)) - ray_position_y) / 
		    ray_direction_y);
      tdelta_y = fabs(1.0 / ray_direction_y);
    }
  else
    {
      tmax_y = VTK_LARGE_FLOAT;
      tdelta_y = VTK_LARGE_FLOAT;
    }
  
  if (ray_direction_z != 0.0)
    {
      tmax_z = fabs((float)((voxel_z+(tstep_z==1)) - ray_position_z) / 
		    ray_direction_z);
      tdelta_z = fabs(1.0 / ray_direction_z);
    }
  else
    {
      tmax_z = VTK_LARGE_FLOAT;
      tdelta_z = VTK_LARGE_FLOAT;
    }
  
  dptr = data_ptr + 
    voxel_x * xinc + 
    voxel_y * yinc + 
    voxel_z * zinc;
  
  A = *(dptr);
  
  found_intersection = FALSE;
  
  while ( found_intersection == FALSE )
    {
      // We've taken another step
      steps_this_ray++;

      if ( A >= isovalue )
	{
	  found_intersection = TRUE;

	  // Store the color in volumeRed, volumeGreen, volumeBlue
	  // This may come from the color value for this isosurface,
	  // or from the texture (or a blend of these two)
	  if ( volumeInfo->RGBDataPointer )
	    {
	    texcoord[0] = 
	      ( voxel_x * volumeInfo->DataSpacing[0] + volumeInfo->DataOrigin[0] -
		volumeInfo->RGBDataOrigin[0] ) / volumeInfo->RGBDataSpacing[0];
	    texcoord[1] = 
	      ( voxel_y * volumeInfo->DataSpacing[1] + volumeInfo->DataOrigin[1] -
		volumeInfo->RGBDataOrigin[1] ) / volumeInfo->RGBDataSpacing[1];
	    texcoord[2] = 
	      ( voxel_z * volumeInfo->DataSpacing[2] + volumeInfo->DataOrigin[2] -
		volumeInfo->RGBDataOrigin[2] ) / volumeInfo->RGBDataSpacing[2];
	    if ( texcoord[0] >= 0 && texcoord[0] < volumeInfo->RGBDataSize[0] &&
		 texcoord[1] >= 0 && texcoord[1] < volumeInfo->RGBDataSize[1] &&
		 texcoord[2] >= 0 && texcoord[2] < volumeInfo->RGBDataSize[2] )
	      {
	      offset =
		( (int)texcoord[0] * volumeInfo->RGBDataIncrement[0] + 
		  (int)texcoord[1] * volumeInfo->RGBDataIncrement[1] +
		  (int)texcoord[2] * volumeInfo->RGBDataIncrement[2] );
	      volumeRed   = 
		volumeInfo->RGBTextureCoefficient * 
		(float)*(volumeInfo->RGBDataPointer + offset   ) / 255.0 + 
		( 1.0 - volumeInfo->RGBTextureCoefficient ) * volumeInfo->Color[0];
	      volumeGreen   = 
		volumeInfo->RGBTextureCoefficient * 
		(float)*(volumeInfo->RGBDataPointer + offset + 1) / 255.0 + 
		( 1.0 - volumeInfo->RGBTextureCoefficient ) * volumeInfo->Color[1];
	      volumeBlue   = 
		volumeInfo->RGBTextureCoefficient * 
		(float)*(volumeInfo->RGBDataPointer + offset + 2) / 255.0 + 
		( 1.0 - volumeInfo->RGBTextureCoefficient ) * volumeInfo->Color[2];
	      
	      }
	    else
	      {
	      volumeRed   = volumeInfo->Color[0];
	      volumeGreen = volumeInfo->Color[1];
	      volumeBlue  = volumeInfo->Color[2];
	      }
	    }
	  else
	    {
	    volumeRed   = volumeInfo->Color[0];
	    volumeGreen = volumeInfo->Color[1];
	    volumeBlue  = volumeInfo->Color[2];
	    }


	  if ( volumeInfo->Shading )
	    {
	      // Get diffuse shading table pointers
	      red_d_shade = volumeInfo->RedDiffuseShadingTable;
	      green_d_shade = volumeInfo->GreenDiffuseShadingTable;
	      blue_d_shade = volumeInfo->BlueDiffuseShadingTable;
	  
	      // Get specular shading table pointers
	      red_s_shade = volumeInfo->RedSpecularShadingTable;
	      green_s_shade = volumeInfo->GreenSpecularShadingTable;
	      blue_s_shade = volumeInfo->BlueSpecularShadingTable;
	      
	      // Get a pointer to the encoded normals for this volume
	      encoded_normals = volumeInfo->EncodedNormals;
	      

	      // Set up the offset into the normal array
	      offset = voxel_z * zinc + voxel_y * yinc + voxel_x;
	      
	      // Set the return pixel value.  This should be corrected later;
	      // 
	      r = red_d_shade[*(encoded_normals + offset)] 
		* volumeRed + red_s_shade[*(encoded_normals + offset)];
	      g = green_d_shade[*(encoded_normals + offset)] 
		* volumeGreen + green_s_shade[*(encoded_normals + offset)];
	      b = blue_d_shade[*(encoded_normals + offset)] 
		* volumeBlue + blue_s_shade[*(encoded_normals + offset)];

	      rayInfo->Color[0] = ( r > 1.0 ) ? 1.0 : r;
	      rayInfo->Color[1] = ( g > 1.0 ) ? 1.0 : g;
	      rayInfo->Color[2] = ( b > 1.0 ) ? 1.0 : b;
	      rayInfo->Color[3] = 1.0;
	    }
	  else 
	    {
	      // No shading
	      rayInfo->Color[0] = volumeRed;
	      rayInfo->Color[1] = volumeGreen;
	      rayInfo->Color[2] = volumeBlue;
	      rayInfo->Color[3] = 1.0;
	    }
	}
      
      if ( found_intersection == FALSE )
	{	
	  if (tmax_x < tmax_y)
	    {
	      if (tmax_x < tmax_z)
		{
		  voxel_x += tstep_x;
		  
		  if (voxel_x < 0 || voxel_x >= x_voxels-1 
		      || voxel_x == end_voxel_x )
		    {
		      found_intersection = TRUE;
		    }
		  else
		    {
		      tmax_x += tdelta_x;
		      dptr += tstep_x * xinc;
		      A = *dptr;
		    }
		}
	      else
		{
		  voxel_z += tstep_z;
		  
		  if (voxel_z < 0 ||  voxel_z >= z_voxels-1 
		      || voxel_z == end_voxel_z )
		    {
		      found_intersection = TRUE;
		    }
		  else
		    {
		      tmax_z += tdelta_z;
		      dptr += tstep_z * zinc;
		      A = *dptr;
		    }
		}
	    }
	  else
	    {
	      if (tmax_y < tmax_z)
		{
		  voxel_y += tstep_y;
		  
		  if (voxel_y < 0 ||  voxel_y >= y_voxels-1 
		      || voxel_y == end_voxel_y )
		    {
		      found_intersection = TRUE;
		    }
		  else
		    {
		      tmax_y += tdelta_y;
		      dptr += tstep_y * yinc;
		      A = *dptr;
		    }
		}
	      else
		{
		  voxel_z += tstep_z;
		  
		  if (voxel_z < 0 || voxel_z >= z_voxels-1 
		      || voxel_z == end_voxel_z )
		    {
		      found_intersection = TRUE;
		    }
		  else
		    {
		      tmax_z += tdelta_z;
		      dptr += tstep_z * zinc;
		      A = *dptr;
		    }
		}
	    }    
	}
    }

  rayInfo->NumberOfStepsTaken = steps_this_ray;

}
// This is the templated function that actually casts a ray and computes
// the pixel_value for isosurface-ray intersection.  It is valid for
// unsigned char, unsigned short, short, int and float data.
template <class T>
static void CastRay_Trilin ( vtkVolumeRayCastIsosurfaceFunction *cast_function, 
			     T *data_ptr, 
			     VTKRayCastRayInfo *rayInfo,
			     VTKRayCastVolumeInfo *volumeInfo )
{
  LineIntersectInfo  line_info;
  unsigned short  *encoded_normals, *nptr;
  int       loop;
  int       xinc, yinc, zinc;
  int       voxel_x, voxel_y, voxel_z;
  int       end_voxel_x, end_voxel_y, end_voxel_z;
  int       x_voxels, y_voxels, z_voxels;
  int       Binc, Cinc, Dinc, Einc, Finc, Ginc, Hinc;
  int       found_intersection;
  int	    tstep_x, tstep_y, tstep_z;
  int       offset;
  int       steps_this_ray = 0;
  T         A, B, C, D, E, F, G, H;
  T         *dptr;
  float     ray_position_x, ray_position_y, ray_position_z;
  float     ray_end[3];
  float     ray_direction_x, ray_direction_y, ray_direction_z;
  float	    tmax_x, tmax_y, tmax_z,
            tdelta_x, tdelta_y, tdelta_z;
  float     isovalue;
  float     trilin_origin[3];
  float     point_x = 0, point_y = 0, point_z = 0;
  float     *red_d_shade, *green_d_shade, *blue_d_shade;
  float     *red_s_shade, *green_s_shade, *blue_s_shade;
  float     x, y, z, t1, t2, t3;
  float     tA, tB, tC, tD, tE, tF, tG, tH;
  float     red_shaded_value, green_shaded_value, blue_shaded_value;
  int       num_steps;
  float     *ray_start, *ray_increment;
  float     volumeRed, volumeGreen, volumeBlue;
  float     texcoord[3];
  int       rgbBinc, rgbCinc, rgbDinc, rgbEinc, rgbFinc, rgbGinc, rgbHinc;
  unsigned char *rgbptr;

  num_steps = rayInfo->NumberOfStepsToTake;
  ray_start = rayInfo->TransformedStart;
  ray_increment = rayInfo->TransformedIncrement;

  rayInfo->Color[0] = 0.0;
  rayInfo->Color[1] = 0.0;
  rayInfo->Color[2] = 0.0;
  rayInfo->Color[3] = 0.0;
  rayInfo->NumberOfStepsTaken = 0;

  // Move the increments into local variables
  xinc = volumeInfo->DataIncrement[0];
  yinc = volumeInfo->DataIncrement[1];
  zinc = volumeInfo->DataIncrement[2];

  // Initialize the ray position and voxel location
  ray_position_x = ray_start[0];
  ray_position_y = ray_start[1];
  ray_position_z = ray_start[2];

  voxel_x = VTK_Floor( ray_position_x );
  voxel_y = VTK_Floor( ray_position_y );
  voxel_z = VTK_Floor( ray_position_z );

  ray_end[0] = ray_start[0] + num_steps*ray_increment[0];
  ray_end[1] = ray_start[1] + num_steps*ray_increment[1];
  ray_end[2] = ray_start[2] + num_steps*ray_increment[2];

  ray_direction_x = ray_increment[0];
  ray_direction_y = ray_increment[1];
  ray_direction_z = ray_increment[2];

  x_voxels = volumeInfo->DataSize[0];
  y_voxels = volumeInfo->DataSize[1];
  z_voxels = volumeInfo->DataSize[2];
      
  if ( voxel_x >= x_voxels - 1 ||
       voxel_y >= y_voxels - 1 ||
       voxel_z >= z_voxels - 1 ||
       voxel_x < 0 || voxel_y < 0 || voxel_z < 0 )
    {
    return;
    }

  // Set the local variable to be isovalue for the surface
  isovalue = cast_function->IsoValue;

  tstep_x = VTK_Sign( ray_direction_x );
  tstep_y = VTK_Sign( ray_direction_y );
  tstep_z = VTK_Sign( ray_direction_z );
  
  end_voxel_x = (int)ray_end[0] + tstep_x;
  end_voxel_y = (int)ray_end[1] + tstep_y;
  end_voxel_z = (int)ray_end[2] + tstep_z;
  
  if (ray_direction_x != 0.0)
    {
    tmax_x = fabs((float)((voxel_x+(tstep_x==1)) - ray_position_x) / 
		  ray_direction_x);
    tdelta_x = fabs(1.0 / ray_direction_x);
    }
  else
    {
    tmax_x = VTK_LARGE_FLOAT;
    tdelta_x = VTK_LARGE_FLOAT;
    }
  
  if (ray_direction_y != 0.0)
    {
    tmax_y = fabs((float)((voxel_y+(tstep_y==1)) - ray_position_y) / 
		  ray_direction_y);
    tdelta_y = fabs(1.0 / ray_direction_y);
    }
  else
    {
    tmax_y = VTK_LARGE_FLOAT;
    tdelta_y = VTK_LARGE_FLOAT;
    }
  
  if (ray_direction_z != 0.0)
    {
    tmax_z = fabs((float)((voxel_z+(tstep_z==1)) - ray_position_z) / 
		  ray_direction_z);
    tdelta_z = fabs(1.0 / ray_direction_z);
    }
  else
    {
    tmax_z = VTK_LARGE_FLOAT;
    tdelta_z = VTK_LARGE_FLOAT;
    }
  
  dptr = data_ptr + 
    voxel_x * xinc + 
    voxel_y * yinc + 
    voxel_z * zinc;
  
  // Compute the increments to get to the other 7 voxel vertices from A
  Binc = xinc;
  Cinc = yinc;
  Dinc = xinc + yinc;
  Einc = zinc;
  Finc = zinc + xinc;
  Ginc = zinc + yinc;
  Hinc = zinc + xinc + yinc;

  A = *(dptr);
  B = *(dptr + Binc);
  C = *(dptr + Cinc);
  D = *(dptr + Dinc);
  E = *(dptr + Einc);
  F = *(dptr + Finc);
  G = *(dptr + Ginc);
  H = *(dptr + Hinc); 
      
  found_intersection = FALSE;
  
  while ( found_intersection == FALSE )
    {

    // We've taken another step
    steps_this_ray++;
	  
    if ( ( A >= isovalue || B >= isovalue || C >= isovalue ||
	   D >= isovalue || E >= isovalue || F >= isovalue ||
	   G >= isovalue || H >= isovalue) 
	 && ( A <= isovalue || B <= isovalue || C <= isovalue ||
	      D <= isovalue || E <= isovalue || F <= isovalue ||
	      G <= isovalue || H <= isovalue ) )
      {
      trilin_origin[0] = ray_start[0] - voxel_x;
      trilin_origin[1] = ray_start[1] - voxel_y;
      trilin_origin[2] = ray_start[2] - voxel_z;
	  
      trilin_line_intersection
	( trilin_origin, ray_increment,
	  (double) A, (double) B, (double) C, (double) D,
	  (double) E, (double) F, (double) G, (double) H,
	  (double) isovalue, &line_info );
      
      if ( line_info.num_intersections > 0 )
	{
	      
	for (loop=0;  loop<line_info.num_intersections;
	     loop++)
	  {
	  point_x = line_info.local_position[loop][0] + voxel_x; 
	  point_y = line_info.local_position[loop][1] + voxel_y;
	  point_z = line_info.local_position[loop][2] + voxel_z;
	  
	  if ((VTK_In_Range(point_x, ((float)(voxel_x) - 0.001 ), ((float)(voxel_x) + 1.001))) &&
	      (VTK_In_Range(point_y, ((float)(voxel_y) - 0.001 ), ((float)(voxel_y) + 1.001))) &&
	      (VTK_In_Range(point_z, ((float)(voxel_z) - 0.001 ), ((float)(voxel_z) + 1.001))))
	    {
	    break;
	    }
	  } 
	      
	if ( loop < line_info.num_intersections )
	  {
	  found_intersection = TRUE;
	  
	  // Store the color in volumeRed, volumeGreen, volumeBlue
	  // This may come from the color value for this isosurface,
	  // or from the texture (or a blend of these two)
	  if ( volumeInfo->RGBDataPointer )
	    {
	    texcoord[0] = 
	      ( point_x * volumeInfo->DataSpacing[0] + volumeInfo->DataOrigin[0] -
		volumeInfo->RGBDataOrigin[0] ) / volumeInfo->RGBDataSpacing[0];
	    texcoord[1] = 
	      ( point_y * volumeInfo->DataSpacing[1] + volumeInfo->DataOrigin[1] -
		volumeInfo->RGBDataOrigin[1] ) / volumeInfo->RGBDataSpacing[1];
	    texcoord[2] = 
	      ( point_z * volumeInfo->DataSpacing[2] + volumeInfo->DataOrigin[2] -
		volumeInfo->RGBDataOrigin[2] ) / volumeInfo->RGBDataSpacing[2];
	    if ( texcoord[0] >= 0 && texcoord[0] < volumeInfo->RGBDataSize[0] &&
		 texcoord[1] >= 0 && texcoord[1] < volumeInfo->RGBDataSize[1] &&
		 texcoord[2] >= 0 && texcoord[2] < volumeInfo->RGBDataSize[2] )
	      {
	      offset = 
		( (int)texcoord[0] * volumeInfo->RGBDataIncrement[0] + 
		  (int)texcoord[1] * volumeInfo->RGBDataIncrement[1] +
		  (int)texcoord[2] * volumeInfo->RGBDataIncrement[2] );

	      // Compute our offset in the texel, and use that to trilinearly
	      // interpolate a color value
	      x = texcoord[0] - (float)((int)texcoord[0]);
	      y = texcoord[1] - (float)((int)texcoord[1]);
	      z = texcoord[2] - (float)((int)texcoord[2]);
	      t1 = 1.0 - x;
	      t2 = 1.0 - y;
	      t3 = 1.0 - z;
	      
	      tA = t1*t2*t3;
	      tB = x*t2*t3;
	      tC = t1*y*t3;
	      tD = x*y*t3;
	      tE = t1*t2*z;
	      tF = x*z*t2;
	      tG = t1*y*z;
	      tH = x*z*y;
	      
	      rgbBinc = volumeInfo->RGBDataIncrement[0];
	      rgbCinc = volumeInfo->RGBDataIncrement[1];
	      rgbDinc = volumeInfo->RGBDataIncrement[0] + volumeInfo->RGBDataIncrement[1];
	      rgbEinc = volumeInfo->RGBDataIncrement[2];
	      rgbFinc = volumeInfo->RGBDataIncrement[2] + volumeInfo->RGBDataIncrement[0];
	      rgbGinc = volumeInfo->RGBDataIncrement[2] + volumeInfo->RGBDataIncrement[1];
	      rgbHinc = volumeInfo->RGBDataIncrement[2] + volumeInfo->RGBDataIncrement[1] +
		        volumeInfo->RGBDataIncrement[0];

	      rgbptr = volumeInfo->RGBDataPointer + offset;
	      volumeRed   = 
		tA * (float)*(rgbptr           ) / 255.0 +
		tB * (float)*(rgbptr + rgbBinc ) / 255.0 +
		tC * (float)*(rgbptr + rgbCinc ) / 255.0 +
		tD * (float)*(rgbptr + rgbDinc ) / 255.0 +
		tE * (float)*(rgbptr + rgbEinc ) / 255.0 +
		tF * (float)*(rgbptr + rgbFinc ) / 255.0 +
		tG * (float)*(rgbptr + rgbGinc ) / 255.0 +
		tH * (float)*(rgbptr + rgbHinc ) / 255.0;

	      rgbptr = volumeInfo->RGBDataPointer + offset + 1;
	      volumeGreen   = 
		tA * (float)*(rgbptr           ) / 255.0 +
		tB * (float)*(rgbptr + rgbBinc ) / 255.0 +
		tC * (float)*(rgbptr + rgbCinc ) / 255.0 +
		tD * (float)*(rgbptr + rgbDinc ) / 255.0 +
		tE * (float)*(rgbptr + rgbEinc ) / 255.0 +
		tF * (float)*(rgbptr + rgbFinc ) / 255.0 +
		tG * (float)*(rgbptr + rgbGinc ) / 255.0 +
		tH * (float)*(rgbptr + rgbHinc ) / 255.0;

	      rgbptr = volumeInfo->RGBDataPointer + offset + 2;
	      volumeBlue   = 
		tA * (float)*(rgbptr           ) / 255.0 +
		tB * (float)*(rgbptr + rgbBinc ) / 255.0 +
		tC * (float)*(rgbptr + rgbCinc ) / 255.0 +
		tD * (float)*(rgbptr + rgbDinc ) / 255.0 +
		tE * (float)*(rgbptr + rgbEinc ) / 255.0 +
		tF * (float)*(rgbptr + rgbFinc ) / 255.0 +
		tG * (float)*(rgbptr + rgbGinc ) / 255.0 +
		tH * (float)*(rgbptr + rgbHinc ) / 255.0;

	      volumeRed = volumeRed * volumeInfo->RGBTextureCoefficient +
		volumeInfo->Color[0] * (1.0 - volumeInfo->RGBTextureCoefficient);

	      volumeGreen = volumeGreen * volumeInfo->RGBTextureCoefficient +
		volumeInfo->Color[1] * (1.0 - volumeInfo->RGBTextureCoefficient);

	      volumeBlue = volumeBlue * volumeInfo->RGBTextureCoefficient +
		volumeInfo->Color[2] * (1.0 - volumeInfo->RGBTextureCoefficient);
	      }
	    else
	      {
	      volumeRed   = volumeInfo->Color[0];
	      volumeGreen = volumeInfo->Color[1];
	      volumeBlue  = volumeInfo->Color[2];
	      }
	    }
	  else
	    {
	    volumeRed   = volumeInfo->Color[0];
	    volumeGreen = volumeInfo->Color[1];
	    volumeBlue  = volumeInfo->Color[2];
	    }

	  if ( volumeInfo->Shading )
	    {
	    // Get diffuse shading table pointers
	    red_d_shade = volumeInfo->RedDiffuseShadingTable;
	    green_d_shade = volumeInfo->GreenDiffuseShadingTable;
	    blue_d_shade = volumeInfo->BlueDiffuseShadingTable;
	    
	    
	    // Get diffuse shading table pointers
	    red_s_shade = volumeInfo->RedSpecularShadingTable;
	    green_s_shade = volumeInfo->GreenSpecularShadingTable;
	    blue_s_shade = volumeInfo->BlueSpecularShadingTable;
	    
	    // Get a pointer to the encoded normals for this volume
	    encoded_normals = volumeInfo->EncodedNormals;
	    
	    // Get the opacity transfer function which maps scalar input values
	    
	    // Compute the values for the first pass through the loop
	    offset = voxel_z * zinc + voxel_y * yinc + voxel_x;
	    dptr = data_ptr + offset;
	    nptr = encoded_normals + offset;
	    
	    // Compute our offset in the voxel, and use that to trilinearly
	    // interpolate a value
	    x = point_x - voxel_x;
	    y = point_y - voxel_y;
	    z = point_z - voxel_z;
	    
	    t1 = 1.0 - x;
	    t2 = 1.0 - y;
	    t3 = 1.0 - z;
	    
	    tA = t1*t2*t3;
	    tB = x*t2*t3;
	    tC = t1*y*t3;
	    tD = x*y*t3;
	    tE = t1*t2*z;
	    tF = x*z*t2;
	    tG = t1*y*z;
	    tH = x*z*y;
	    
	    // Compute pixel_value;
	    // Do trilinear interpolation of shadings of pixel corners
	    // Do it for red, green, and blue components
	    red_shaded_value =
	      tA * ( red_d_shade[ *(nptr) ] * volumeRed
		     + red_s_shade[ *(nptr) ] );
	    red_shaded_value +=
	      tB * ( red_d_shade[ *(nptr + Binc) ] * volumeRed 
		     + red_s_shade[ *(nptr + Binc) ] );
	    red_shaded_value +=
	      tC * ( red_d_shade[ *(nptr + Cinc) ] * volumeRed 
		     + red_s_shade[ *(nptr + Cinc) ] );
	    red_shaded_value +=
	      tD * ( red_d_shade[ *(nptr + Dinc) ] * volumeRed 
		     + red_s_shade[ *(nptr + Dinc) ] );
	    red_shaded_value +=
	      tE * ( red_d_shade[ *(nptr + Einc) ] * volumeRed 
		     + red_s_shade[ *(nptr + Einc) ] );
	    red_shaded_value +=
	      tF * ( red_d_shade[ *(nptr + Finc) ] * volumeRed 
		     + red_s_shade[ *(nptr + Finc) ] );
	    red_shaded_value +=
	      tG * ( red_d_shade[ *(nptr + Ginc) ] * volumeRed 
		     + red_s_shade[ *(nptr + Ginc) ] );
	    red_shaded_value +=
	      tH * ( red_d_shade[ *(nptr + Hinc) ] * volumeRed 
		     + red_s_shade[ *(nptr + Hinc) ] );
	    
	    green_shaded_value =  
	      tA * ( green_d_shade[ *(nptr) ] * volumeGreen 
		     + green_s_shade[ *(nptr) ] );
	    green_shaded_value +=  
	      tB * ( green_d_shade[ *(nptr + Binc) ] * volumeGreen 
		     + green_s_shade[ *(nptr + Binc) ] );
	    green_shaded_value +=  
	      tC * ( green_d_shade[ *(nptr + Cinc) ] * volumeGreen 
		     + green_s_shade[ *(nptr + Cinc) ] );
	    green_shaded_value +=  
	      tD * ( green_d_shade[ *(nptr + Dinc) ] * volumeGreen 
		     + green_s_shade[ *(nptr + Dinc) ] );
	    green_shaded_value +=  
	      tE * ( green_d_shade[ *(nptr + Einc) ] * volumeGreen 
		     + green_s_shade[ *(nptr + Einc) ] );
	    green_shaded_value +=  
	      tF * ( green_d_shade[ *(nptr + Finc) ] * volumeGreen 
		     + green_s_shade[ *(nptr + Finc) ] );
	    green_shaded_value +=  
	      tG * ( green_d_shade[ *(nptr + Ginc) ] * volumeGreen 
		     + green_s_shade[ *(nptr + Ginc) ] );
	    green_shaded_value +=  
	      tH * ( green_d_shade[ *(nptr + Hinc) ] * volumeGreen 
		     + green_s_shade[ *(nptr + Hinc) ] );
	    
	    blue_shaded_value =  
	      tA * ( blue_d_shade[ *(nptr) ] * volumeBlue 
		     + blue_s_shade[ *(nptr) ] );
	    blue_shaded_value +=  
	      tB * ( blue_d_shade[ *(nptr + Binc) ] * volumeBlue 
		     + blue_s_shade[ *(nptr + Binc) ] );
	    blue_shaded_value +=  
	      tC * ( blue_d_shade[ *(nptr + Cinc) ] * volumeBlue 
		     + blue_s_shade[ *(nptr + Cinc) ] );
	    blue_shaded_value +=  
	      tD * ( blue_d_shade[ *(nptr + Dinc) ] * volumeBlue 
		     + blue_s_shade[ *(nptr + Dinc) ] );
	    blue_shaded_value +=  
	      tE * ( blue_d_shade[ *(nptr + Einc) ] * volumeBlue 
		     + blue_s_shade[ *(nptr + Einc) ] );
	    blue_shaded_value +=  
	      tF * ( blue_d_shade[ *(nptr + Finc) ] * volumeBlue 
		     + blue_s_shade[ *(nptr + Finc) ] );
	    blue_shaded_value +=  
	      tG * ( blue_d_shade[ *(nptr + Ginc) ] * volumeBlue 
		     + blue_s_shade[ *(nptr + Ginc) ] );
	    blue_shaded_value +=  
	      tH * ( blue_d_shade[ *(nptr + Hinc) ] * volumeBlue 
		     + blue_s_shade[ *(nptr + Hinc) ] );
	    
	    rayInfo->Color[0] = 
	      ( red_shaded_value > 1.0 ) ? 1.0 : red_shaded_value;
	    rayInfo->Color[1] = 
	      ( green_shaded_value > 1.0 ) ? 1.0 : green_shaded_value;
	    rayInfo->Color[2] = 
	      ( blue_shaded_value > 1.0 ) ? 1.0 : blue_shaded_value;
	    rayInfo->Color[3] = 1.0;
	    }
	  else
	    {
	      // No shading
	      rayInfo->Color[0] = volumeRed;
	      rayInfo->Color[1] = volumeGreen;
	      rayInfo->Color[2] = volumeBlue;
	      rayInfo->Color[3] = 1.0;
	    }
	  }
	}
      }
    if ( found_intersection == FALSE )
      {	
      if (tmax_x < tmax_y)
	{
	if (tmax_x < tmax_z)
	  {
	  voxel_x += tstep_x;
		  
	  if (voxel_x < 0 || voxel_x >= x_voxels-1 
	      || voxel_x == end_voxel_x )
	    {
	    found_intersection = TRUE;
	    }
	  else
	    {
	    tmax_x += tdelta_x;
	    dptr += tstep_x * xinc;
	    if (tstep_x > 0)
	      {
	      A = B;
	      C = D;
	      E = F;
	      G = H;
	      B = *(dptr + Binc);
	      D = *(dptr + Dinc);
	      F = *(dptr + Finc);
	      H = *(dptr + Hinc);
	      }
	    else
	      {
	      B = A;
	      D = C;
	      F = E;
	      H = G;
	      A = *(dptr);
	      C = *(dptr + Cinc);
	      E = *(dptr + Einc);
	      G = *(dptr + Ginc);
	      } 
	    }
	  }
	else
	  {
	  voxel_z += tstep_z;
	  
	  if (voxel_z < 0 ||  voxel_z >= z_voxels-1 
	      || voxel_z == end_voxel_z )
	    {
	    found_intersection = TRUE;
	    }
	  else
	    {
	    tmax_z += tdelta_z;
	    dptr += tstep_z * zinc;
	    if (tstep_z > 0)
	      {
	      A = E;
	      B = F;
	      C = G;
	      D = H;
	      E = *(dptr + Einc);
	      F = *(dptr + Finc);
	      G = *(dptr + Ginc);
	      H = *(dptr + Hinc);
	      }
	    else
	      {
	      E = A;
	      F = B;
	      G = C;
	      H = D;
	      A = *(dptr);
	      B = *(dptr + Binc);
	      C = *(dptr + Cinc);
	      D = *(dptr + Dinc);
	      }
	    }
	  }
	}
      else
	{
	if (tmax_y < tmax_z)
	  {
	  voxel_y += tstep_y;
	  
	  if (voxel_y < 0 ||  voxel_y >= y_voxels-1 
	      || voxel_y == end_voxel_y )
	    {
	    found_intersection = TRUE;
	    }
	  else
	    {
	    tmax_y += tdelta_y;
	    dptr += tstep_y * yinc;
	    if (tstep_y > 0)
	      {
	      A = C;
	      B = D;
	      E = G;
	      F = H;
	      C = *(dptr + Cinc);
	      D = *(dptr + Dinc);
	      G = *(dptr + Ginc);
	      H = *(dptr + Hinc);
	      }
	    else
	      {
	      C = A;
	      D = B;
	      G = E;
	      H = F;
	      A = *(dptr);
	      B = *(dptr + Binc);
	      E = *(dptr + Einc);
	      F = *(dptr + Finc);
	      }
	    }
	  }
	else
	  {
	    voxel_z += tstep_z;
	    
	    if (voxel_z < 0 || voxel_z >= z_voxels-1 
		|| voxel_z == end_voxel_z )
	      {
	      found_intersection = TRUE;
	      }
	    else
	      {
		tmax_z += tdelta_z;
		dptr += tstep_z * zinc;
		if (tstep_z > 0)
		  {
		  A = E;
		  B = F;
		  C = G;
		  D = H;
		  E = *(dptr + Einc);
		  F = *(dptr + Finc);
		  G = *(dptr + Ginc);
		  H = *(dptr + Hinc);
		  }
		else
		  {
		  E = A;
		  F = B;
		  G = C;
		  H = D;
		  A = *(dptr);
		  B = *(dptr + Binc);
		  C = *(dptr + Cinc);
		  D = *(dptr + Dinc);
		  }
	      }
	  }
	}    
      }
    }

  rayInfo->NumberOfStepsTaken = steps_this_ray;
}

// Construct a new vtkVolumeRayCastIsosurfaceFunction with a default ramp.
// This ramp is best suited for unsigned char data and should
// probably be modified before rendering any other data type.
// The ParcBuildValue is set to LinearRampRange[0] + 1, ensuring
// that the Parc structure will be built during the first render.
vtkVolumeRayCastIsosurfaceFunction::vtkVolumeRayCastIsosurfaceFunction()
{
  this->IsoValue		= 0;
}

// Destruct the vtkVolumeRayCastIsosurfaceFunction
vtkVolumeRayCastIsosurfaceFunction::~vtkVolumeRayCastIsosurfaceFunction()
{
}

// This is called from RenderAnImage (in vtkDepthPARCMapper.cxx)
// It uses the integer data type flag that is passed in to
// determine what type of ray needs to be cast (which is handled
// by a templated function. 
void vtkVolumeRayCastIsosurfaceFunction::CastRay( VTKRayCastRayInfo *rayInfo,
						  VTKRayCastVolumeInfo *volumeInfo )
{
  void *data_ptr;

  data_ptr = volumeInfo->ScalarDataPointer;
  
  // Cast the ray for the data type and shading/interpolation type
  if ( volumeInfo->InterpolationType == VTK_NEAREST_INTERPOLATION )
    {
      // Nearest neighbor
      switch ( volumeInfo->ScalarDataType )
	{
	case VTK_UNSIGNED_CHAR:
	  CastRay_NN 
	    ( this, (unsigned char *)data_ptr, rayInfo, volumeInfo ); 
	  break;
	case VTK_UNSIGNED_SHORT:
	  CastRay_NN
	    ( this, (unsigned short *)data_ptr, rayInfo, volumeInfo );
	  break;
	}
    }
  else if ( volumeInfo->InterpolationType == VTK_LINEAR_INTERPOLATION )
    {
      // Trilinear interpolation
      switch ( volumeInfo->ScalarDataType )
	{
	case VTK_UNSIGNED_CHAR:
	  CastRay_Trilin
	    ( this, (unsigned char *)data_ptr, rayInfo, volumeInfo );
	  break;
	case VTK_UNSIGNED_SHORT:
	  CastRay_Trilin
	    ( this, (unsigned short *)data_ptr, rayInfo, volumeInfo );
	  break;
	}
    }
}

float vtkVolumeRayCastIsosurfaceFunction::GetZeroOpacityThreshold(vtkVolume *)
{
  return( this->IsoValue );
}

void vtkVolumeRayCastIsosurfaceFunction::SpecificFunctionInitialize( 
				  vtkRenderer *vtkNotUsed(ren), 
				  vtkVolume *vol,
				  VTKRayCastVolumeInfo *volumeInfo,
				  vtkVolumeRayCastMapper *vtkNotUsed(mapper) )
{
  vtkVolumeProperty  *volume_property;

  volume_property = vol->GetProperty();

  if ( volume_property->GetColorChannels() == 1 )
    {
    volumeInfo->Color[0] = 
      volume_property->GetGrayTransferFunction()->GetValue( this->IsoValue );
    volumeInfo->Color[1] = volumeInfo->Color[0];
    volumeInfo->Color[2] = volumeInfo->Color[0];
    }
  else if ( volume_property->GetColorChannels() == 3 )
    {
    volumeInfo->Color[0] = 
      volume_property->GetRGBTransferFunction()->GetRedValue( this->IsoValue );
    volumeInfo->Color[1] = 
      volume_property->GetRGBTransferFunction()->GetGreenValue( this->IsoValue );
    volumeInfo->Color[2] = 
      volume_property->GetRGBTransferFunction()->GetBlueValue( this->IsoValue );
    }
}

// Print method for vtkVolumeRayCastIsosurfaceFunction
void vtkVolumeRayCastIsosurfaceFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkVolumeRayCastFunction::PrintSelf(os,indent);

  os << indent << "Isosurface Value: " << this->IsoValue << "\n";
}


