/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDirectionEncoder.h
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

// .NAME vtkDirectionEncoder - encode a direction into a one or two byte value
//
// .SECTION Description
// Given a direction, encode it into an integer value. This value should
// be less than 65536, which is the maximum number of encoded directions
// supported by this superclass. A direction encoded is used to encode
// normals in a volume for use during volume rendering, and the
// amount of space that is allocated per normal is 2 bytes.
// This is an abstract superclass - see the subclasses for specific 
// implementation details.
//
// .SECTION see also
// vtkRecursiveSphereDirectionEncoder

#ifndef __vtkDirectionEncoder_h
#define __vtkDirectionEncoder_h

#include "vtkObject.h"

class VTK_RENDERING_EXPORT vtkDirectionEncoder : public vtkObject
{
public:
  // Description:
  // Get the name of this class
  vtkTypeMacro(vtkDirectionEncoder,vtkObject);

  // Description:
  // Given a normal vector n, return the encoded direction
  virtual int GetEncodedDirection( float n[3] )=0;

  // Description:
  /// Given an encoded value, return a pointer to the normal vector
  virtual float *GetDecodedGradient( int value )=0;

  // Description:
  // Return the number of encoded directions
  virtual  int GetNumberOfEncodedDirections( void )=0;

  // Description:
  // Get the decoded gradient table. There are 
  // this->GetNumberOfEncodedDirections() entries in the table, each
  // containing a normal (direction) vector. This is a flat structure - 
  // 3 times the number of directions floats in an array.
  virtual float *GetDecodedGradientTable( void )=0;

protected:
  vtkDirectionEncoder() {};
  ~vtkDirectionEncoder() {};
private:
  vtkDirectionEncoder(const vtkDirectionEncoder&);  // Not implemented.
  void operator=(const vtkDirectionEncoder&);  // Not implemented.
}; 


#endif







