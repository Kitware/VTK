/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRecursiveSphereDirectionEncoder.h
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

  vtkRecursiveSphereDirectionEncoder();
  ~vtkRecursiveSphereDirectionEncoder();
  const char *GetClassName() {return "vtkRecursiveSphereDirectionEncoder";};
  void PrintSelf( ostream& os, vtkIndent index );

// Description:
// Construct the object. Initialize the index table which will be
// used to map the normal into a patch on the recursively subdivided
// sphere.
  static vtkRecursiveSphereDirectionEncoder *New() {return new vtkRecursiveSphereDirectionEncoder;};


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
}; 


#endif

