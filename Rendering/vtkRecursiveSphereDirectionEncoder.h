/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRecursiveSphereDirectionEncoder.h
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

// .NAME vtkRecursiveSphereDirectionEncoder - A direction encoder based on the recursive subdivision of an octahedron
// .SECTION Description
// vtkRecursiveSphereDirectionEncoder is a direction encoder which uses the 
// vertices of a recursive subdivision of an octahedron (with the vertices
// pushed out onto the surface of an enclosing sphere) to encode directions
// into a two byte value. 
//
// .SECTION see also
// vtkDirectionEncoder

#ifndef __vtkRecursiveSphereDirectionEncoder_h
#define __vtkRecursiveSphereDirectionEncoder_h

#include "vtkDirectionEncoder.h"

class VTK_EXPORT vtkRecursiveSphereDirectionEncoder : public vtkDirectionEncoder
{
public:
  vtkTypeMacro(vtkRecursiveSphereDirectionEncoder,vtkDirectionEncoder);
  void PrintSelf( ostream& os, vtkIndent index );

// Description:
// Construct the object. Initialize the index table which will be
// used to map the normal into a patch on the recursively subdivided
// sphere.
  static vtkRecursiveSphereDirectionEncoder *New();


  // Description:
  // Given a normal vector n, return the encoded direction  
  int GetEncodedDirection( float n[3] );
  
  // Description:
  /// Given an encoded value, return a pointer to the normal vector
  float *GetDecodedGradient( int value );

  // Description:
  // Return the number of encoded directions
  int GetNumberOfEncodedDirections( void );

  // Description:
  // Get the decoded gradient table. There are 
  // this->GetNumberOfEncodedDirections() entries in the table, each
  // containing a normal (direction) vector. This is a flat structure - 
  // 3 times the number of directions floats in an array.
  float *GetDecodedGradientTable( void );

  // Description:
  // Set / Get the recursion depth for the subdivision. This
  // indicates how many time one triangle on the initial 8-sided
  // sphere model is replaced by four triangles formed by connecting
  // triangle edge midpoints. A recursion level of 0 yields 8 triangles
  // with 6 unique vertices. The normals are the vectors from the
  // sphere center through the vertices. The number of directions
  // will be 11 since the four normals with 0 z values will be
  // duplicated in the table - once with +0 values and the other
  // time with -0 values, and an addition index will be used to
  // represent the (0,0,0) normal. If we instead choose a recursion 
  // level of 6 (the maximum that can fit within 2 bytes) the number
  // of directions is 16643, with 16386 unique directions and a 
  // zero normal.
  vtkSetClampMacro( RecursionDepth, int, 0, 6 );
  vtkGetMacro( RecursionDepth, int );					     

protected:
  vtkRecursiveSphereDirectionEncoder();
  ~vtkRecursiveSphereDirectionEncoder();
  vtkRecursiveSphereDirectionEncoder(const vtkRecursiveSphereDirectionEncoder&);
  void operator=(const vtkRecursiveSphereDirectionEncoder&);

  // How far to recursively divide the sphere
  int                     RecursionDepth;
  
  // The index table which maps (x,y) position in the rotated grid
  // to an encoded normal 
  //int                   IndexTable[2*NORM_SQR_SIZE - 1][2*NORM_SQR_SIZE -1];
  int                     *IndexTable;

  // This is a table that maps encoded normal (2 byte value) to a 
  // normal (dx, dy, dz)
  //float                 DecodedNormal[3*(1 + 2*(NORM_SQR_SIZE*NORM_SQR_SIZE+
  //                             (NORM_SQR_SIZE-1)*(NORM_SQR_SIZE-1)))];
  float                   *DecodedNormal;

  // Method to initialize the index table and variable that
  // stored the recursion depth the last time the table was
  // built
  void                  InitializeIndexTable( void );
  int                   IndexTableRecursionDepth;

  int                   OuterSize;
  int                   InnerSize;
  int                   GridSize;
}; 


#endif

