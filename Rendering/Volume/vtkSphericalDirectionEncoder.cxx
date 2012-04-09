/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphericalDirectionEncoder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSphericalDirectionEncoder.h"
#include "vtkObjectFactory.h"

#include "vtkMath.h"
#include "vtkTransform.h"

vtkStandardNewMacro(vtkSphericalDirectionEncoder);

float vtkSphericalDirectionEncoder::DecodedGradientTable[65536 * 3];
int   vtkSphericalDirectionEncoder::DecodedGradientTableInitialized = 0;

// Construct the object. Initialize the index table which will be
// used to map the normal into a patch on the recursively subdivided
// sphere.
vtkSphericalDirectionEncoder::vtkSphericalDirectionEncoder()
{
  this->InitializeDecodedGradientTable();
}

// Destruct a vtkSphericalDirectionEncoder - free up any memory used
vtkSphericalDirectionEncoder::~vtkSphericalDirectionEncoder()
{
}
 
// Encode n into a 2 byte value. The first byte will be theta - the
// rotation angle around the z axis. The second (high order) byte is
// phi - the elevation of the vector. 256 values are used for theta,
// but only 255 values for phi, leaving room for a "zero normal" code
int vtkSphericalDirectionEncoder::GetEncodedDirection( float n[3] )
{
  if ( n[0] == 0.0 && n[1] == 0.0 && n[2] == 0.0 )
    {
    return ( 255 * 256 );
    }
  
  float theta, phi;
  
  // Need to handle this separately since some atan2 implementations
  // don't handle a zero denominator
  if ( n[0] == 0 )
    {
    theta = ( n[1] > 0 ) ? 90.0 : 270.0 ;
    }
  else
    {
    theta = vtkMath::DegreesFromRadians( atan2( n[1], n[0] ) );
    theta = ( theta < 0.0 )    ? ( theta + 360.0 ) : theta;
    theta = ( theta >= 360.0 ) ? ( theta - 360.0 ) : theta;
    }
  
  phi = vtkMath::DegreesFromRadians( asin( n[2] ) );
  phi = phi > 90.5 ? ( phi-360 ) : phi;
 
  int lowByte, highByte;
  
  lowByte  = static_cast<int>( theta * 255.0 / 359.0 + 0.5 );
  highByte = static_cast<int>( ( phi + 90.0 ) * 254.0 / 180.0 + 0.5 );
  
  lowByte = lowByte < 0   ? 0   : lowByte;
  lowByte = lowByte > 255 ? 255 : lowByte;

  highByte = highByte < 0   ? 0   : highByte;
  highByte = highByte > 254 ? 254 : highByte;

  return ( lowByte + highByte * 256 );
}
  
float *vtkSphericalDirectionEncoder::GetDecodedGradient( int value )
{
  return &(vtkSphericalDirectionEncoder::DecodedGradientTable[value*3]);
}

// This is the table that maps the encoded gradient back into
// a float triple. 
void vtkSphericalDirectionEncoder::InitializeDecodedGradientTable()
{
  if ( vtkSphericalDirectionEncoder::DecodedGradientTableInitialized )
    {
    return;
    }
  
  float theta, phi;
  int   i, j;
  
  vtkTransform *transformPhi = vtkTransform::New();
  vtkTransform *transformTheta = vtkTransform::New();
  
  float v1[3] = {1,0,0};
  float v2[3], v3[3];

  float *ptr = vtkSphericalDirectionEncoder::DecodedGradientTable;
  
  for ( j = 0; j < 256; j++ )
    {
    phi = -89.5 + j * ( 179.0 / 254.0 );
    
      transformPhi->Identity();
      transformPhi->RotateY( -phi );
      transformPhi->TransformPoint( v1, v2 );
      
    for ( i = 0; i < 256; i++ )
      {
      if ( j < 255 )
        {
        theta = i * (359.0 / 255.0);
        
        transformTheta->Identity();
        transformTheta->RotateZ( theta );
        transformTheta->TransformPoint( v2, v3 );
        }
      else
        {
        v3[0] = 0.0;
        v3[1] = 0.0;
        v3[2] = 0.0;
        }
      
      *(ptr++) = v3[0];
      *(ptr++) = v3[1];
      *(ptr++) = v3[2];
      }
    }
  
  transformPhi->Delete();
  transformTheta->Delete();
  
  vtkSphericalDirectionEncoder::DecodedGradientTableInitialized = 1;
}

// Print the vtkSphericalDirectionEncoder
void vtkSphericalDirectionEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number of encoded directions: " << 
    this->GetNumberOfEncodedDirections() << endl;
}

