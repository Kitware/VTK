/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDirectionEncoder.h
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

// .NAME vtkDirectionEncoder - encode a direction into a one or two byte value
//
// .SECTION Description
// Given a direction, encode it into an integer value. This value should
// be less than 65536, which is the maximum number of encoded directions
// supported by this superclass. A direction encoded is used to encode
// normals in a volume for use during volume rendering, and the largest
// amount of space that can be allocated per normal is 2 bytes.
// This is an abstract superclass - see the subclasses for specific 
// implementation details.
//
// .SECTION see also
// vtkRecursiveSphereDirectionEncoder

#ifndef __vtkDirectionEncoder_h
#define __vtkDirectionEncoder_h

#include "vtkObject.h"

class VTK_EXPORT vtkDirectionEncoder : public vtkObject
{
public:
  const char *GetClassName() {return "vtkDirectionEncoder";};

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

}; 


#endif







