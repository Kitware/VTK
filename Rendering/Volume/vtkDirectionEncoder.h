/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDirectionEncoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkObject.h"

class VTKRENDERINGVOLUME_EXPORT vtkDirectionEncoder : public vtkObject
{
public:
  // Description:
  // Get the name of this class
  vtkTypeMacro(vtkDirectionEncoder,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

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
  vtkDirectionEncoder() {}
  ~vtkDirectionEncoder() {}
private:
  vtkDirectionEncoder(const vtkDirectionEncoder&);  // Not implemented.
  void operator=(const vtkDirectionEncoder&);  // Not implemented.
};


#endif







