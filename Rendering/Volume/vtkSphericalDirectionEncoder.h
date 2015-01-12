/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSphericalDirectionEncoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSphericalDirectionEncoder - A direction encoder based on spherical coordinates
// .SECTION Description
// vtkSphericalDirectionEncoder is a direction encoder which uses spherical
// coordinates for mapping (nx, ny, nz) into an azimuth, elevation pair.
//
// .SECTION see also
// vtkDirectionEncoder

#ifndef vtkSphericalDirectionEncoder_h
#define vtkSphericalDirectionEncoder_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkDirectionEncoder.h"

class VTKRENDERINGVOLUME_EXPORT vtkSphericalDirectionEncoder : public vtkDirectionEncoder
{
public:
  vtkTypeMacro(vtkSphericalDirectionEncoder,vtkDirectionEncoder);
  void PrintSelf( ostream& os, vtkIndent indent );

  // Description:
  // Construct the object. Initialize the index table which will be
  // used to map the normal into a patch on the recursively subdivided
  // sphere.
  static vtkSphericalDirectionEncoder *New();


  // Description:
  // Given a normal vector n, return the encoded direction
  int GetEncodedDirection( float n[3] );

  // Description:
  /// Given an encoded value, return a pointer to the normal vector
  float *GetDecodedGradient( int value );

  // Description:
  // Return the number of encoded directions
  int GetNumberOfEncodedDirections( void ) { return 65536; }

  // Description:
  // Get the decoded gradient table. There are
  // this->GetNumberOfEncodedDirections() entries in the table, each
  // containing a normal (direction) vector. This is a flat structure -
  // 3 times the number of directions floats in an array.
  float *GetDecodedGradientTable( void )
    {
      return &(this->DecodedGradientTable[0]);
    }


protected:
  vtkSphericalDirectionEncoder();
  ~vtkSphericalDirectionEncoder();

  static float DecodedGradientTable[65536*3];

  // Description:
  // Initialize the table at startup
  static void InitializeDecodedGradientTable();
  static int DecodedGradientTableInitialized;

private:
  vtkSphericalDirectionEncoder(const vtkSphericalDirectionEncoder&);  // Not implemented.
  void operator=(const vtkSphericalDirectionEncoder&);  // Not implemented.
};


#endif


