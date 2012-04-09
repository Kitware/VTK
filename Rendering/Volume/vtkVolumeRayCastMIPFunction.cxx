/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeRayCastMIPFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVolumeRayCastMIPFunction.h"
#include "vtkVolumeRayCastMapper.h"
#include "vtkVolume.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkStandardNewMacro(vtkVolumeRayCastMIPFunction);

// This is the templated function that actually casts a ray and computes
// the maximum value.  It is valid for unsigned char and unsigned short,
template <class T>
void vtkCastMaxScalarValueRay( T *data_ptr, vtkVolumeRayCastDynamicInfo *dynamicInfo,
                               vtkVolumeRayCastStaticInfo *staticInfo )
{
  float     triMax, triValue;
  int       max = 0;;
  float     max_opacity;
  int       loop;
  vtkIdType xinc, yinc, zinc;
  int       voxel[3], prev_voxel[3];
  float     ray_position[3];
  T         A, B, C, D, E, F, G, H;
  float     t00, t01, t10, t11, t0, t1;
  vtkIdType Binc, Cinc, Dinc, Einc, Finc, Ginc, Hinc;
  float     xoff, yoff, zoff;
  T         *dptr;
  int       num_steps;
  float     *ray_increment;
  float     *grayArray, *RGBArray;
  float     *scalarArray;
  T         nnValue, nnMax;

  num_steps = dynamicInfo->NumberOfStepsToTake;
  ray_increment = dynamicInfo->TransformedIncrement;

  grayArray = staticInfo->Volume->GetGrayArray();
  RGBArray = staticInfo->Volume->GetRGBArray();
  scalarArray = staticInfo->Volume->GetScalarOpacityArray();

  xinc = staticInfo->DataIncrement[0];
  yinc = staticInfo->DataIncrement[1];
  zinc = staticInfo->DataIncrement[2];

  // Initialize the ray position and voxel location
  memcpy( ray_position, dynamicInfo->TransformedStart, 3*sizeof(float) );

  // If we have nearest neighbor interpolation
  if ( staticInfo->InterpolationType == VTK_NEAREST_INTERPOLATION )
    {
    voxel[0] = vtkRoundFuncMacro( ray_position[0] );
    voxel[1] = vtkRoundFuncMacro( ray_position[1] );
    voxel[2] = vtkRoundFuncMacro( ray_position[2] );

    // Access the value at this voxel location
    nnMax = *(data_ptr + voxel[2] * zinc +
              voxel[1] * yinc + voxel[0] );

    // Increment our position and compute our voxel location
    ray_position[0] += ray_increment[0];
    ray_position[1] += ray_increment[1];
    ray_position[2] += ray_increment[2];
    voxel[0] = vtkRoundFuncMacro( ray_position[0] );
    voxel[1] = vtkRoundFuncMacro( ray_position[1] );
    voxel[2] = vtkRoundFuncMacro( ray_position[2] );

    // For each step along the ray
    for ( loop = 1; loop < num_steps; loop++ )
      {
      // Access the value at this voxel location
      nnValue = *(data_ptr + voxel[2] * zinc +
                  voxel[1] * yinc + voxel[0] );

      // If this is greater than the max, this is the new max.
      if ( nnValue > nnMax )
        {
        nnMax = nnValue;
        }

      // Increment our position and compute our voxel location
      ray_position[0] += ray_increment[0];
      ray_position[1] += ray_increment[1];
      ray_position[2] += ray_increment[2];
      voxel[0] = vtkRoundFuncMacro( ray_position[0] );
      voxel[1] = vtkRoundFuncMacro( ray_position[1] );
      voxel[2] = vtkRoundFuncMacro( ray_position[2] );
      }
    max = (int)nnMax;
    }
  // We are using trilinear interpolation
  else if ( staticInfo->InterpolationType == VTK_LINEAR_INTERPOLATION )
    {
    voxel[0] = vtkFloorFuncMacro( ray_position[0] );
    voxel[1] = vtkFloorFuncMacro( ray_position[1] );
    voxel[2] = vtkFloorFuncMacro( ray_position[2] );

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

    // Compute our offset in the voxel, and use that to trilinearly
    // interpolate a value
    xoff = ray_position[0] - (float) voxel[0];
    yoff = ray_position[1] - (float) voxel[1];
    zoff = ray_position[2] - (float) voxel[2];
    vtkTrilinFuncMacro( triMax, xoff, yoff, zoff, A, B, C, D, E, F, G, H );

    // Keep the voxel location so that we know when we've moved into a
    // new voxel
    memcpy( prev_voxel, voxel, 3*sizeof(int) );

    // Increment our position and compute our voxel location
    ray_position[0] += ray_increment[0];
    ray_position[1] += ray_increment[1];
    ray_position[2] += ray_increment[2];
    voxel[0] = vtkFloorFuncMacro( ray_position[0] );
    voxel[1] = vtkFloorFuncMacro( ray_position[1] );
    voxel[2] = vtkFloorFuncMacro( ray_position[2] );

    // For each step along the ray
    for ( loop = 1; loop < num_steps; loop++ )
      {
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

        memcpy( prev_voxel, voxel, 3*sizeof(int) );
        }

      // Compute our offset in the voxel, and use that to trilinearly
      // interpolate a value
      xoff = ray_position[0] - (float) voxel[0];
      yoff = ray_position[1] - (float) voxel[1];
      zoff = ray_position[2] - (float) voxel[2];
      vtkTrilinFuncMacro( triValue, xoff, yoff, zoff, A, B, C, D, E, F, G, H );

      // If this value is greater than max, it is the new max
      if ( triValue > triMax )
        {
        triMax = triValue;
        }

      // Increment our position and compute our voxel location
      ray_position[0] += ray_increment[0];
      ray_position[1] += ray_increment[1];
      ray_position[2] += ray_increment[2];
      voxel[0] = vtkFloorFuncMacro( ray_position[0] );
      voxel[1] = vtkFloorFuncMacro( ray_position[1] );
      voxel[2] = vtkFloorFuncMacro( ray_position[2] );
      }
    max = (int)triMax;
    }

  if ( max < 0 )
    {
    max = 0;
    }
  else if ( max > staticInfo->Volume->GetArraySize() - 1 )
    {
    max = (int)(staticInfo->Volume->GetArraySize() - 1);
    }

  dynamicInfo->ScalarValue = max;
  max_opacity = scalarArray[max];

  // Set the return pixel value.
  if( staticInfo->ColorChannels == 1 )
    {
    dynamicInfo->Color[0] = max_opacity * grayArray[max];
    dynamicInfo->Color[1] = max_opacity * grayArray[max];
    dynamicInfo->Color[2] = max_opacity * grayArray[max];
    dynamicInfo->Color[3] = max_opacity;
    }
  else if ( staticInfo->ColorChannels == 3 )
    {
    dynamicInfo->Color[0] = max_opacity * RGBArray[max*3];
    dynamicInfo->Color[1] = max_opacity * RGBArray[max*3+1];
    dynamicInfo->Color[2] = max_opacity * RGBArray[max*3+2];
    dynamicInfo->Color[3] = max_opacity;
    }

  dynamicInfo->NumberOfStepsTaken = num_steps;
}


// This is the templated function that actually casts a ray and computes
// the maximum value.  It is valid for unsigned char and unsigned short,
template <class T>
void vtkCastMaxOpacityRay( T *data_ptr, vtkVolumeRayCastDynamicInfo *dynamicInfo,
                        vtkVolumeRayCastStaticInfo *staticInfo )
{
  float     max;
  float     opacity;
  float     value;
  int       max_value = 0;
  int       loop;
  int       xinc, yinc, zinc;
  int       voxel[3];
  int       prev_voxel[3];
  float     ray_position[3];
  T         A, B, C, D, E, F, G, H;
  float     t00, t01, t10, t11, t0, t1;
  int       Binc, Cinc, Dinc, Einc, Finc, Ginc, Hinc;
  float     xoff, yoff, zoff;
  T         *dptr;
  int       steps_this_ray = 0;
  float     *SOTF;
  int       num_steps;
  float     *ray_start, *ray_increment;
  float     *grayArray, *RGBArray;

  num_steps = dynamicInfo->NumberOfStepsToTake;
  ray_start = dynamicInfo->TransformedStart;
  ray_increment = dynamicInfo->TransformedIncrement;


  SOTF = staticInfo->Volume->GetScalarOpacityArray();
  grayArray = staticInfo->Volume->GetGrayArray();
  RGBArray = staticInfo->Volume->GetRGBArray();

  // Set the max value.  This will not always be correct and should be fixed
  max = -999999.0;

  xinc = staticInfo->DataIncrement[0];
  yinc = staticInfo->DataIncrement[1];
  zinc = staticInfo->DataIncrement[2];

  // Initialize the ray position and voxel location
  ray_position[0] = ray_start[0];
  ray_position[1] = ray_start[1];
  ray_position[2] = ray_start[2];

  // If we have nearest neighbor interpolation
  if ( staticInfo->InterpolationType == VTK_NEAREST_INTERPOLATION )
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

      if ( value < 0 )
        {
        value = 0;
        }
      else if ( value > staticInfo->Volume->GetArraySize() - 1 )
        {
        value = staticInfo->Volume->GetArraySize() - 1;
        }

      opacity = SOTF[(int)value];

      // If this is greater than the max, this is the new max.
      if ( opacity > max )
        {
        max = opacity;
        max_value = (int) value;
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
  // We are using trilinear interpolation
  else if ( staticInfo->InterpolationType == VTK_LINEAR_INTERPOLATION )
    {
    voxel[0] = vtkFloorFuncMacro( ray_position[0] );
    voxel[1] = vtkFloorFuncMacro( ray_position[1] );
    voxel[2] = vtkFloorFuncMacro( ray_position[2] );

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

      if ( value < 0 )
        {
        value = 0;
        }
      else if ( value > staticInfo->Volume->GetArraySize() - 1 )
        {
        value = staticInfo->Volume->GetArraySize() - 1;
        }

      opacity = SOTF[(int)value];

      // If this is greater than the max, this is the new max.
      if ( opacity > max )
        {
        max = opacity;
        max_value = (int) value;
        }

      // Increment our position and compute our voxel location
      ray_position[0] += ray_increment[0];
      ray_position[1] += ray_increment[1];
      ray_position[2] += ray_increment[2];
      voxel[0] = vtkFloorFuncMacro( ray_position[0] );
      voxel[1] = vtkFloorFuncMacro( ray_position[1] );
      voxel[2] = vtkFloorFuncMacro( ray_position[2] );
      }
    }

  dynamicInfo->ScalarValue = max;

  // Set the return pixel value.  The depth value is currently useless and
  // should be fixed.
  if( staticInfo->ColorChannels == 1 )
    {
    dynamicInfo->Color[0] = max * grayArray[max_value];
    dynamicInfo->Color[1] = max * grayArray[max_value];
    dynamicInfo->Color[2] = max * grayArray[max_value];
    dynamicInfo->Color[3] = max;
    }
  else if ( staticInfo->ColorChannels == 3 )
    {
    dynamicInfo->Color[0] = max * RGBArray[max_value*3];
    dynamicInfo->Color[1] = max * RGBArray[max_value*3+1];
    dynamicInfo->Color[2] = max * RGBArray[max_value*3+2];
    dynamicInfo->Color[3] = max;
    }


  dynamicInfo->NumberOfStepsTaken = steps_this_ray;
}

// Construct a new vtkVolumeRayCastMIPFunction
vtkVolumeRayCastMIPFunction::vtkVolumeRayCastMIPFunction()
{
  this->MaximizeMethod = VTK_MAXIMIZE_SCALAR_VALUE;
}

// Destruct the vtkVolumeRayCastMIPFunction
vtkVolumeRayCastMIPFunction::~vtkVolumeRayCastMIPFunction()
{
}

// This is called from RenderAnImage (in vtkDepthPARCMapper.cxx)
// It uses the integer data type flag that is passed in to
// determine what type of ray needs to be cast (which is handled
// by a templated function.
void vtkVolumeRayCastMIPFunction::CastRay( vtkVolumeRayCastDynamicInfo *dynamicInfo,
                                           vtkVolumeRayCastStaticInfo *staticInfo)
{
  void *data_ptr;

  data_ptr = staticInfo->ScalarDataPointer;

  if ( this->MaximizeMethod == VTK_MAXIMIZE_SCALAR_VALUE )
    {
    switch ( staticInfo->ScalarDataType )
      {
      case VTK_UNSIGNED_CHAR:
        vtkCastMaxScalarValueRay( (unsigned char *)data_ptr, dynamicInfo,
                                  staticInfo );
        break;
      case VTK_UNSIGNED_SHORT:
        vtkCastMaxScalarValueRay( (unsigned short *)data_ptr, dynamicInfo,
                                  staticInfo );
        break;
      default:
        vtkWarningMacro ( << "Unsigned char and unsigned short are the only supported datatypes for rendering" );
        break;
      }
    }
  else
    {
    switch ( staticInfo->ScalarDataType )
      {
      case VTK_UNSIGNED_CHAR:
        vtkCastMaxOpacityRay( (unsigned char *)data_ptr, dynamicInfo, staticInfo );
        break;
      case VTK_UNSIGNED_SHORT:
        vtkCastMaxOpacityRay( (unsigned short *)data_ptr, dynamicInfo, staticInfo );
        break;
      default:
        vtkWarningMacro ( << "Unsigned char and unsigned short are the only supported datatypes for rendering" );
        break;
      }
    }
}

float vtkVolumeRayCastMIPFunction::GetZeroOpacityThreshold( vtkVolume *vtkNotUsed(vol) )
{
  return ( 1.0 );
}

// This is an update method that is called from Render (in
// vtkDepthPARCMapper.cxx).  It allows the specific mapper type to
// update any local caster variables.  In this case, nothing needs
// to be done here
void vtkVolumeRayCastMIPFunction::SpecificFunctionInitialize(
                                 vtkRenderer *vtkNotUsed(ren),
                                 vtkVolume *vtkNotUsed(vol),
                                 vtkVolumeRayCastStaticInfo *staticInfo,
                                 vtkVolumeRayCastMapper *vtkNotUsed(mapper) )
{
  staticInfo->MIPFunction = 1;
  staticInfo->MaximizeOpacity = (this->MaximizeMethod == VTK_MAXIMIZE_OPACITY);
}

// Description:
// Return the maximize method as a descriptive character string.
const char *vtkVolumeRayCastMIPFunction::GetMaximizeMethodAsString(void)
{
  if( this->MaximizeMethod == VTK_MAXIMIZE_SCALAR_VALUE )
    {
    return "Maximize Scalar Value";
    }
  if( this->MaximizeMethod == VTK_MAXIMIZE_OPACITY )
    {
    return "Maximize Opacity";
    }
  else
    {
    return "Unknown";
    }
}

// Print method for vtkVolumeRayCastMIPFunction
void vtkVolumeRayCastMIPFunction::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Maximize Method: " << this->GetMaximizeMethodAsString()
     << "\n";
}
