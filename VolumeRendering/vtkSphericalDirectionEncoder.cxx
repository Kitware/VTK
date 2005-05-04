/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

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
#include "vtkSphericalDirectionEncoder.h"
#include "vtkObjectFactory.h"

#include "vtkMath.h"
#include "vtkTransform.h"

vtkCxxRevisionMacro(vtkSphericalDirectionEncoder, "1.1");
vtkStandardNewMacro(vtkSphericalDirectionEncoder);

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
    return ( 255*256 );
    }
  
  float theta, phi;
  
  theta = (vtkMath::RadiansToDegrees())*atan2( n[1], n[0] );
  theta = (theta<0.0)?(theta+360.0):(theta);
  theta = (theta>=360.0)?(theta-360.0):(theta);
  
  phi = (vtkMath::RadiansToDegrees())*asin( n[2] );
  phi = (phi > 90.5)?(phi-360):(phi);
  
  int lowByte, highByte;
  
  lowByte  = static_cast<int>(theta * 255.0 / 359.0 + 0.5);
  highByte = static_cast<int>((phi+90.0) * 254.0 / 180.0 + 0.5);
  
  lowByte = (lowByte<0)?(0):(lowByte);
  lowByte = (lowByte>255)?(255):(lowByte);

  highByte = (highByte<0)?(0):(highByte);
  highByte = (highByte>254)?(254):(highByte);

  return (lowByte + highByte*256);
}
  
float *vtkSphericalDirectionEncoder::GetDecodedGradient( int value )
{
  return &(this->DecodedGradientTable[value*3]);
}

// This is the table that maps the encoded gradient back into
// a float triple. 
void vtkSphericalDirectionEncoder::InitializeDecodedGradientTable()
{
  float theta, phi;
  int   i, j;
  
  vtkTransform *transformPhi = vtkTransform::New();
  vtkTransform *transformTheta = vtkTransform::New();
  
  float v1[3] = {1,0,0};
  float v2[3], v3[3];

  float *ptr = this->DecodedGradientTable;
  
  for ( j = 0; j < 256; j++ )
    {
    phi = -89.5 + j * (179.0 / 254.0 );
    
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
}

// Print the vtkSphericalDirectionEncoder
void vtkSphericalDirectionEncoder::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number of encoded directions: " << 
    this->GetNumberOfEncodedDirections() << endl;
}

