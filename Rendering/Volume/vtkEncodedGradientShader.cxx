/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkEncodedGradientShader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEncodedGradientShader.h"

#include "vtkCamera.h"
#include "vtkDirectionEncoder.h"
#include "vtkEncodedGradientEstimator.h"
#include "vtkLight.h"
#include "vtkLightCollection.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkTransform.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"

#include <math.h>

vtkStandardNewMacro(vtkEncodedGradientShader);

vtkEncodedGradientShader::vtkEncodedGradientShader()
{
  int i, j;

  for ( j = 0; j < VTK_MAX_SHADING_TABLES; j++ )
    {
      this->ShadingTableVolume[j] = NULL;
      this->ShadingTableSize[j] = 0;
      for ( i = 0; i < 6; i++ )
        {
        this->ShadingTable[j][i] = NULL;
        }
    }

  this->ZeroNormalDiffuseIntensity  = 0.0;
  this->ZeroNormalSpecularIntensity = 0.0;
  this->ActiveComponent             = 0;
}

vtkEncodedGradientShader::~vtkEncodedGradientShader()
{
  int i, j;

  for ( j = 0; j < VTK_MAX_SHADING_TABLES; j++ )
    {
    for ( i=0; i<6; i++ )
      {
      if ( this->ShadingTable[j][i] )
        {
        delete [] this->ShadingTable[j][i];
        }
      }
    }
}

float *vtkEncodedGradientShader::GetRedDiffuseShadingTable( vtkVolume *vol )
{
  int                   index;

  for ( index = 0; index < VTK_MAX_SHADING_TABLES; index++ )
    {
    if ( this->ShadingTableVolume[index] == vol )
      {
      break;
      }
    }

  if ( index == VTK_MAX_SHADING_TABLES )
    {
    vtkErrorMacro( << "No shading table found for that volume!" );
    return NULL;
    }

  return this->ShadingTable[index][0];
}

float *vtkEncodedGradientShader::GetGreenDiffuseShadingTable( vtkVolume *vol )
{
  int                   index;

  for ( index = 0; index < VTK_MAX_SHADING_TABLES; index++ )
    {
    if ( this->ShadingTableVolume[index] == vol )
      {
      break;
      }
    }

  if ( index == VTK_MAX_SHADING_TABLES )
    {
    vtkErrorMacro( << "No shading table found for that volume!" );
    return NULL;
    }

  return this->ShadingTable[index][1];
}

float *vtkEncodedGradientShader::GetBlueDiffuseShadingTable( vtkVolume *vol )
{
  int                   index;

  for ( index = 0; index < VTK_MAX_SHADING_TABLES; index++ )
    {
    if ( this->ShadingTableVolume[index] == vol )
      {
      break;
      }
    }

  if ( index == VTK_MAX_SHADING_TABLES )
    {
    vtkErrorMacro( << "No shading table found for that volume!" );
    return NULL;
    }

  return this->ShadingTable[index][2];
}

float *vtkEncodedGradientShader::GetRedSpecularShadingTable( vtkVolume *vol )
{
  int                   index;

  for ( index = 0; index < VTK_MAX_SHADING_TABLES; index++ )
    {
    if ( this->ShadingTableVolume[index] == vol )
      {
      break;
      }
    }

  if ( index == VTK_MAX_SHADING_TABLES )
    {
    vtkErrorMacro( << "No shading table found for that volume!" );
    return NULL;
    }

  return this->ShadingTable[index][3];
}

float *vtkEncodedGradientShader::GetGreenSpecularShadingTable( vtkVolume *vol )
{
  int                   index;

  for ( index = 0; index < VTK_MAX_SHADING_TABLES; index++ )
    {
    if ( this->ShadingTableVolume[index] == vol )
      {
      break;
      }
    }

  if ( index == VTK_MAX_SHADING_TABLES )
    {
    vtkErrorMacro( << "No shading table found for that volume!" );
    return NULL;
    }

  return this->ShadingTable[index][4];
}

float *vtkEncodedGradientShader::GetBlueSpecularShadingTable( vtkVolume *vol )
{
  int                   index;

  for ( index = 0; index < VTK_MAX_SHADING_TABLES; index++ )
    {
    if ( this->ShadingTableVolume[index] == vol )
      {
      break;
      }
    }

  if ( index == VTK_MAX_SHADING_TABLES )
    {
    vtkErrorMacro( << "No shading table found for that volume!" );
    return NULL;
    }

  return this->ShadingTable[index][5];
}

void vtkEncodedGradientShader::UpdateShadingTable(
  vtkRenderer *ren,
  vtkVolume *vol,
  vtkEncodedGradientEstimator *gradest)
{
  double                 lightDirection[3], material[4];
  double                 lightAmbientColor[3];
  double                 lightDiffuseColor[3];
  double                 lightSpecularColor[3];
  double                 lightPosition[3], lightFocalPoint[3];
  double                 lightIntensity, viewDirection[3];
  double                 cameraPosition[3], cameraFocalPoint[3], mag;
  vtkLightCollection    *lightCollection;
  vtkLight              *light;
  double                 norm;
  int                    update_flag;
  vtkVolumeProperty     *property;
  vtkTransform          *transform;
  vtkMatrix4x4          *m;
  double                 in[4], out[4], zero[4];
  int                    index;

  // Figure out which shading table we are working with
  // First search through all existing ones, then if one
  // is not found, use the first available index
  for ( index = 0; index < VTK_MAX_SHADING_TABLES; index++ )
    {
    if ( this->ShadingTableVolume[index] == vol )
      {
      break;
      }
    }

  if ( index == VTK_MAX_SHADING_TABLES )
    {
    for ( index = 0; index < VTK_MAX_SHADING_TABLES; index++ )
      {
      if ( this->ShadingTableVolume[index] == NULL )
        {
        this->ShadingTableVolume[index] = vol;
        break;
        }
      }
    }
  if ( index == VTK_MAX_SHADING_TABLES )
    {
    vtkErrorMacro( << "Too many shading tables!\n" <<
      "Increase limit VTK_MAX_SHADING_TABLES and recompile!" );
    return;
    }

  transform = vtkTransform::New();
  m = vtkMatrix4x4::New();

  vol->GetMatrix(m);
  transform->SetMatrix(m);
  transform->Inverse();

  property = vol->GetProperty();

  material[0] = property->GetAmbient(this->ActiveComponent);
  material[1] = property->GetDiffuse(this->ActiveComponent);
  material[2] = property->GetSpecular(this->ActiveComponent);
  material[3] = property->GetSpecularPower(this->ActiveComponent);


  update_flag = 0;

  ren->GetActiveCamera()->GetPosition( cameraPosition );
  ren->GetActiveCamera()->GetFocalPoint( cameraFocalPoint );

  viewDirection[0] =  cameraFocalPoint[0] - cameraPosition[0];
  viewDirection[1] =  cameraFocalPoint[1] - cameraPosition[1];
  viewDirection[2] =  cameraFocalPoint[2] - cameraPosition[2];

  mag = sqrt(static_cast<double>(viewDirection[0] * viewDirection[0] +
                                 viewDirection[1] * viewDirection[1] +
                                 viewDirection[2] * viewDirection[2] ) );

  if ( mag )
    {
    viewDirection[0] /= mag;
    viewDirection[1] /= mag;
    viewDirection[2] /= mag;
    }

  memcpy( in, viewDirection, 3*sizeof(double) );
  in[3] = 1.0;
  transform->MultiplyPoint( in, out );
  viewDirection[0] = out[0] / out[3];
  viewDirection[1] = out[1] / out[3];
  viewDirection[2] = out[2] / out[3];

  in[0] = 0.0;
  in[1] = 0.0;
  in[2] = 0.0;
  transform->MultiplyPoint( in, zero );
  zero[0] /= zero[3];
  zero[1] /= zero[3];
  zero[2] /= zero[3];
  viewDirection[0] -= zero[0];
  viewDirection[1] -= zero[1];
  viewDirection[2] -= zero[2];

  // Loop through all lights and compute a shading table. For
  // the first light, pass in an update_flag of 0, which means
  // overwrite the shading table.  For each light after that, pass
  // in an update flag of 1, which means add to the shading table.
  // All lights are forced to be directional light sources
  // regardless of what they really are

  // Set up the lights for traversal
  lightCollection = ren->GetLights();

  // In rare cases there are no lights
  vtkLight *artificialLight=NULL;
  if ( lightCollection->GetNumberOfItems() == 0 )
    {
    artificialLight = vtkLight::New();
    artificialLight->SetIntensity(0.0);
    lightCollection->AddItem(artificialLight);
    }

  vtkCollectionSimpleIterator sit;
  lightCollection->InitTraversal(sit);
  while ( (light = lightCollection->GetNextLight(sit)) != NULL  )
    {
    if ( ! light->GetSwitch() )
      {
      continue;
      }

    // Get the light color, position, focal point, and intensity
    light->GetAmbientColor(lightAmbientColor);
    light->GetDiffuseColor(lightDiffuseColor);
    light->GetSpecularColor(lightSpecularColor);
    light->GetTransformedPosition( lightPosition );
    light->GetTransformedFocalPoint( lightFocalPoint );
    lightIntensity = light->GetIntensity( );


    // Compute the light direction and normalize it
    lightDirection[0] = lightFocalPoint[0] - lightPosition[0];
    lightDirection[1] = lightFocalPoint[1] - lightPosition[1];
    lightDirection[2] = lightFocalPoint[2] - lightPosition[2];

    norm = sqrt(static_cast<double>( lightDirection[0] * lightDirection[0] +
                                     lightDirection[1] * lightDirection[1] +
                                     lightDirection[2] * lightDirection[2] ) );

    lightDirection[0] /= -norm;
    lightDirection[1] /= -norm;
    lightDirection[2] /= -norm;

    memcpy( in, lightDirection, 3*sizeof(double) );
    transform->MultiplyPoint( in, out );
    lightDirection[0] = out[0] / out[3] - zero[0];
    lightDirection[1] = out[1] / out[3] - zero[1];
    lightDirection[2] = out[2] / out[3] - zero[2];

    // Build / Add to the shading table
    this->BuildShadingTable(index, lightDirection, lightAmbientColor,
                            lightDiffuseColor,lightSpecularColor,
                            lightIntensity, viewDirection,
                            material, ren->GetTwoSidedLighting(),
                            gradest, update_flag );

    update_flag = 1;
    }//while there is a light in the list of lights

  if ( artificialLight )
    {
    lightCollection->RemoveItem(artificialLight);
    artificialLight->Delete();
    }

  transform->Delete();
  m->Delete();
}


// Build a shading table for a light with the given direction and
// color, for a material of the given type. material[0] = ambient,
// material[1] = diffuse, material[2] = specular, material[3] =
// specular exponent.  If the ambient flag is 1, then ambient
// illumination is added. If not, then this means we are calculating
// the "other side" of two sided lighting, so no ambient intensity
// is added in. If update_flag is 0, the table is overwritten
// with the new values.  If update_flag is 1, the new intensity values
// are added into the table.  This way multiple light sources can
// be handled. There is one shading table per volume, and the index
// value indicates which index table is to be updated
void vtkEncodedGradientShader::BuildShadingTable( int index,
                                                  double lightDirection[3],
                                                  double lightAmbientColor[3],
                                                  double lightDiffuseColor[3],
                                                  double lightSpecularColor[3],
                                                  double lightIntensity,
                                                  double viewDirection[3],
                                                  double material[4],
                                                  int   twoSided,
                                                  vtkEncodedGradientEstimator *gradest,
                                                  int   updateFlag )
{
  double   lx, ly, lz;
  double    n_dot_l;
  double    n_dot_v;
  int      i;
  float    *nptr;
  float    *sdr_ptr;
  float    *sdg_ptr;
  float    *sdb_ptr;
  float    *ssr_ptr;
  float    *ssg_ptr;
  float    *ssb_ptr;
  double    Ka, Es, Kd_intensity, Ks_intensity;
  double   half_x, half_y, half_z;
  double    mag, n_dot_h, specular_value;
  int      norm_size;

  // Move to local variables
  lx = lightDirection[0];
  ly = lightDirection[1];
  lz = lightDirection[2];

  half_x = lx - viewDirection[0];
  half_y = ly - viewDirection[1];
  half_z = lz - viewDirection[2];

  mag = sqrt( static_cast<double>(half_x*half_x + half_y*half_y
                                  + half_z*half_z ) );

  if( mag != 0.0 )
    {
    half_x /= mag;
    half_y /= mag;
    half_z /= mag;
    }

  Ka = material[0] * lightIntensity;
  Es = material[3];
  Kd_intensity = material[1] * lightIntensity;
  Ks_intensity = material[2] * lightIntensity;

  nptr = gradest->GetDirectionEncoder()->GetDecodedGradientTable();

  norm_size = gradest->GetDirectionEncoder()->GetNumberOfEncodedDirections();

  if ( this->ShadingTableSize[index] != norm_size )
    {
    for ( i=0; i<6; i++ )
      {
      if ( this->ShadingTable[index][i] )
        {
        delete [] this->ShadingTable[index][i];
        }
      this->ShadingTable[index][i] = new float[norm_size];
      }
      this->ShadingTableSize[index] = norm_size;
    }

  sdr_ptr = this->ShadingTable[index][0];
  sdg_ptr = this->ShadingTable[index][1];
  sdb_ptr = this->ShadingTable[index][2];

  ssr_ptr = this->ShadingTable[index][3];
  ssg_ptr = this->ShadingTable[index][4];
  ssb_ptr = this->ShadingTable[index][5];

  // For each possible normal, compute the intensity of light at
  // a location with that normal, and the given lighting and
  // material properties
  for ( i = 0; i < norm_size; i++ )
    {
    // If we have a zero normal, treat it specially
    if ( ( *(nptr+0) == 0.0 ) &&
         ( *(nptr+1) == 0.0 ) &&
         ( *(nptr+2) == 0.0 ) )
      {
      // If we are not updating, initial everything to 0.0
      if ( !updateFlag )
        {
        *(sdr_ptr) = 0.0;
        *(sdg_ptr) = 0.0;
        *(sdb_ptr) = 0.0;

        *(ssr_ptr) = 0.0;
        *(ssg_ptr) = 0.0;
        *(ssb_ptr) = 0.0;
        }

      // Now add in ambient
      *(sdr_ptr) += static_cast<float>(Ka * lightAmbientColor[0]);
      *(sdg_ptr) += static_cast<float>(Ka * lightAmbientColor[1]);
      *(sdb_ptr) += static_cast<float>(Ka * lightAmbientColor[2]);

      // Add in diffuse
      *(sdr_ptr) += static_cast<float>
        (Kd_intensity * this->ZeroNormalDiffuseIntensity*lightDiffuseColor[0]);
      *(sdg_ptr) += static_cast<float>
        (Kd_intensity * this->ZeroNormalDiffuseIntensity*lightDiffuseColor[1]);
      *(sdb_ptr) += static_cast<float>
        (Kd_intensity * this->ZeroNormalDiffuseIntensity*lightDiffuseColor[2]);

      // Add in specular
      *(ssr_ptr) += static_cast<float>(
        this->ZeroNormalSpecularIntensity*lightSpecularColor[0]);
      *(ssg_ptr) += static_cast<float>(
        this->ZeroNormalSpecularIntensity*lightSpecularColor[1]);
      *(ssb_ptr) += static_cast<float>(
        this->ZeroNormalSpecularIntensity*lightSpecularColor[2]);
      }
    else
      {
      // The dot product between the normal and the light vector
      // used for diffuse illumination
      n_dot_l = (*(nptr+0) * lx + *(nptr+1) * ly + *(nptr+2) * lz);

      // The dot product between the normal and the halfway vector
      // used for specular illumination
      n_dot_h = (*(nptr+0) * half_x + *(nptr+1) * half_y + *(nptr+2) * half_z);

      // Flip the normal if two sided lighting is on and the normal
      // is pointing away from the viewer
      if ( twoSided )
        {
        // The dot product between the normal and the view vector
        // used for two sided lighting
        n_dot_v = (*(nptr+0) * viewDirection[0] +
                   *(nptr+1) * viewDirection[1] +
                   *(nptr+2) * viewDirection[2]);

        if ( n_dot_v > 0.0 )
          {
          n_dot_l = -n_dot_l;
          n_dot_h = -n_dot_h;
          }
        }

      // If we are updating, then begin by adding in ambient
      if ( updateFlag )
        {
        *(sdr_ptr) += static_cast<float>(Ka * lightAmbientColor[0]);
        *(sdg_ptr) += static_cast<float>(Ka * lightAmbientColor[1]);
        *(sdb_ptr) += static_cast<float>(Ka * lightAmbientColor[2]);
        }
      // Otherwise begin by setting the value to the ambient contribution
      else
        {
        *(sdr_ptr) = static_cast<float>(Ka * lightAmbientColor[0]);
        *(sdg_ptr) = static_cast<float>(Ka * lightAmbientColor[1]);
        *(sdb_ptr) = static_cast<float>(Ka * lightAmbientColor[2]);
        *(ssr_ptr) = 0.0f;
        *(ssg_ptr) = 0.0f;
        *(ssb_ptr) = 0.0f;
        }

      // If there is some diffuse contribution, add it in
      if ( n_dot_l > 0 )
        {
        *(sdr_ptr) += static_cast<float>(
          Kd_intensity * n_dot_l * lightDiffuseColor[0]);
        *(sdg_ptr) += static_cast<float>(
          Kd_intensity * n_dot_l * lightDiffuseColor[1]);
        *(sdb_ptr) += static_cast<float>(
          Kd_intensity * n_dot_l * lightDiffuseColor[2]);

        if ( n_dot_h > 0.001 )
          {
          specular_value = Ks_intensity * pow(static_cast<double>(n_dot_h),
                                              static_cast<double>(Es) );
          *(ssr_ptr) += static_cast<float>(
            specular_value * lightSpecularColor[0]);
          *(ssg_ptr) += static_cast<float>(
            specular_value * lightSpecularColor[1]);
          *(ssb_ptr) += static_cast<float>(
            specular_value * lightSpecularColor[2]);
          }
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


// Print the vtkEncodedGradientShader
void vtkEncodedGradientShader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Zero Normal Diffuse Intensity: " <<
    this->ZeroNormalDiffuseIntensity << endl;

  os << indent << "Zero Normal Specular Intensity: " <<
    this->ZeroNormalSpecularIntensity << endl;
  os << indent << "ActiveComponent: " << this->ActiveComponent << endl;
}

