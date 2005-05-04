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

// .NAME vtkSphericalDirectionEncoder - A direction encoder based on spherical coordinates
// .SECTION Description
// vtkSphericalDirectionEncoder is a direction encoder which uses spherical
// coordinates for mapping (nx, ny, nz) into an azimuth, elevation pair.
//
// .SECTION see also
// vtkDirectionEncoder

#ifndef __vtkSphericalDirectionEncoder_h
#define __vtkSphericalDirectionEncoder_h

#include "vtkDirectionEncoder.h"

class VTK_VOLUMERENDERING_EXPORT vtkSphericalDirectionEncoder : public vtkDirectionEncoder
{
public:
  vtkTypeRevisionMacro(vtkSphericalDirectionEncoder,vtkDirectionEncoder);
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

  float DecodedGradientTable[65536*3];

  // Description:
  // Initialize the table at startup
  void InitializeDecodedGradientTable();

private:
  vtkSphericalDirectionEncoder(const vtkSphericalDirectionEncoder&);  // Not implemented.
  void operator=(const vtkSphericalDirectionEncoder&);  // Not implemented.
}; 


#endif


