/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEncodedGradientShader.cxx
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
#include "vtkEncodedGradientShader.h"
#include "vtkVolume.h"
#include "vtkRenderer.h"
#include "vtkEncodedGradientEstimator.h"

vtkEncodedGradientShader::vtkEncodedGradientShader()
{
  int i;

  for ( i = 0; i < 6; i++ )
    this->ShadingTable[i] = NULL;

  this->ShadingTableSize = 0;
}

vtkEncodedGradientShader::~vtkEncodedGradientShader()
{
  int i;

  for ( i=0; i<6; i++ )
    {
    if ( this->ShadingTable[i] ) delete [] this->ShadingTable[i];
    }
}

void vtkEncodedGradientShader::UpdateShadingTable( vtkRenderer *ren, 
						   vtkVolume *vol,
						   vtkEncodedGradientEstimator *gradest)
{
  float                 light_direction[3], material[4], light_color[3];
  float                 light_position[3], light_focal_point[3];
  float                 light_intensity, view_direction[3];
  float                 camera_position[3], camera_focal_point[3], mag;
  vtkLightCollection    *light_collection;
  vtkLight              *light;
  float                 norm;
  int                   update_flag;
  vtkVolumeProperty     *volume_property;
  vtkTransform          *transform;
  vtkMatrix4x4          *m;
  float                 in[4], out[4];

  transform = vtkTransform::New();
  m = vtkMatrix4x4::New();

  vol->GetMatrix(m);
  transform->SetMatrix(*m);
  transform->Inverse();

  volume_property = vol->GetVolumeProperty();

  material[0] = volume_property->GetAmbient();
  material[1] = volume_property->GetDiffuse();
  material[2] = volume_property->GetSpecular();
  material[3] = volume_property->GetSpecularPower();

  // Set up the lights for traversal
  light_collection = ren->GetLights();
  light_collection->InitTraversal();
    
  update_flag = 0;

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

  memcpy( in, view_direction, 3*sizeof(float) );
  in[3] = 1.0;
  transform->MultiplyPoint( in, out );
  memcpy( view_direction, out, 3*sizeof(float) );

  // Loop through all lights and compute a shading table. For
  // the first light, pass in an update_flag of 0, which means
  // overwrite the shading table.  For each light after that, pass
  // in an update flag of 1, which means add to the shading table.
  // All lights are forced to be directional light sources
  // regardless of what they really are
  while ( (light = light_collection->GetNextItem()) != NULL  )
    { 
    // Get the light color, position, focal point, and intensity
    light->GetColor( light_color );
    light->GetPosition( light_position );
    light->GetFocalPoint( light_focal_point );
    light_intensity = light->GetIntensity( );
    

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
      
    memcpy( in, light_direction, 3*sizeof(float) );
    transform->MultiplyPoint( in, out );
    memcpy( light_direction, out, 3*sizeof(float) );

    // Build / Add to the shading table
    this->BuildShadingTable( light_direction, light_color, 
					     light_intensity, view_direction,
					     material, gradest, update_flag );
      
    update_flag = 1;
    }

  transform->Delete();
  m->Delete();
}


// Build a shading table for a light with the given direction and
// color, for a material of the given type. material[0] = ambient,
// material[1] = diffuse, material[2] = specular, material[3] = 
// specular exponent.  If update_flag is 0, the table is overwritten
// with the new values.  If update_flag is 1, the new intensity values
// are added into the table.  This way multiple light sources can
// be handled.
//
void vtkEncodedGradientShader::BuildShadingTable( float light_direction[3],
					  float light_color[3],
					  float light_intensity,
					  float view_direction[3],
					  float material[4],
					  vtkEncodedGradientEstimator *gradest,
					  int   update_flag )
{
  float    lx, ly, lz; 
  float    n_dot_l;   
  int      i;
  float    *nptr;
  float    *sdr_ptr;
  float    *sdg_ptr;
  float    *sdb_ptr;
  float    *ssr_ptr;
  float    *ssg_ptr;
  float    *ssb_ptr;
  float    Ka, Es, Kd_intensity, Ks_intensity;
  float    half_x, half_y, half_z;
  float    mag, n_dot_h, specular_value;
  int      norm_size;

  // Move to local variables
  lx = light_direction[0];
  ly = light_direction[1];
  lz = light_direction[2];

  half_x = lx - view_direction[0];
  half_y = ly - view_direction[1];
  half_z = lz - view_direction[2];

  mag = sqrt( (double)(half_x*half_x + half_y*half_y + half_z*half_z ) );
  
  if( mag != 0.0 )
    {
    half_x /= mag;
    half_y /= mag;
    half_z /= mag;
    }

  Ka = material[0];
  Es = material[3];
  Kd_intensity = material[1] * light_intensity;
  Ks_intensity = material[2] * light_intensity;

  nptr = gradest->GetDirectionEncoder()->GetDecodedGradientTable();

  norm_size = gradest->GetDirectionEncoder()->GetNumberOfEncodedDirections();

  if ( this->ShadingTableSize != norm_size )
    {
    for ( i=0; i<6; i++ )
      {
      if ( this->ShadingTable[i] ) delete [] this->ShadingTable[i];
      this->ShadingTable[i] = new float[norm_size];
      }
      this->ShadingTableSize = norm_size;
    }
  
  sdr_ptr = this->ShadingTable[0];
  sdg_ptr = this->ShadingTable[1];
  sdb_ptr = this->ShadingTable[2];

  ssr_ptr = this->ShadingTable[3];
  ssg_ptr = this->ShadingTable[4];
  ssb_ptr = this->ShadingTable[5];

  // For each possible normal, compute the intensity of light at
  // a location with that normal, and the given lighting and
  // material properties
  for ( i = 0; i < norm_size; i++ )
    {
    if ( ( *(nptr+0) == 0.0 ) && 
	 ( *(nptr+1) == 0.0 ) && 
	 ( *(nptr+2) == 0.0 ) )
      {
      *(sdr_ptr) = 0.0;
      *(sdg_ptr) = 0.0;
      *(sdb_ptr) = 0.0;

      *(ssr_ptr) = 0.0;
      *(ssg_ptr) = 0.0;
      *(ssb_ptr) = 0.0;
      }
    else
      {
      // The dot product between the normal and the light vector
      n_dot_l = (*(nptr+0) * lx + *(nptr+1) * ly + *(nptr+2) * lz);
    
      // If we are updating, then begin by adding in ambient
      if ( update_flag )
	{
	*(sdr_ptr) += Ka * light_color[0];
	*(sdg_ptr) += Ka * light_color[1];
	*(sdb_ptr) += Ka * light_color[2];
	}
      // Otherwise begin by setting the value to the ambient contribution
      else
	{
	*(sdr_ptr) = Ka * light_color[0];
	*(sdg_ptr) = Ka * light_color[1];
	*(sdb_ptr) = Ka * light_color[2];
	*(ssr_ptr) = 0.0;
	*(ssg_ptr) = 0.0;
	*(ssb_ptr) = 0.0;
	}

      // If there is some diffuse contribution, add it in
      if ( n_dot_l > 0 )
	{
	*(sdr_ptr) += (Kd_intensity * n_dot_l * light_color[0]);
	*(sdg_ptr) += (Kd_intensity * n_dot_l * light_color[1]);
	*(sdb_ptr) += (Kd_intensity * n_dot_l * light_color[2]);
	
	n_dot_h = (*(nptr+0) * half_x + *(nptr+1) * half_y + *(nptr+2) * half_z);
	if ( n_dot_h > 0.001 )
	  {
	  specular_value = Ks_intensity * pow( (double)n_dot_h, (double)Es );
	  *(ssr_ptr) += specular_value * light_color[0];
	  *(ssg_ptr) += specular_value * light_color[1];
	  *(ssb_ptr) += specular_value * light_color[2];
	  }      
	}
      // Increment all the pointers
      nptr += 3;
      sdr_ptr++;
      sdg_ptr++;
      sdb_ptr++;
      ssr_ptr++;
      ssg_ptr++;
      ssb_ptr++;
      }
    }
}


// Print the vtkEncodedGradientShader
void vtkEncodedGradientShader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);
}

