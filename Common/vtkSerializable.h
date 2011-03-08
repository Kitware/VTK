/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSerializable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSerializable -- Pure abstract base class for serializable objects.
//
// .SECTION Description
// Defines an interface that needs to be implemented by serializable objects.

#ifndef VTKSERIALIZABLE_H_
#define VTKSERIALIZABLE_H_

#include "vtkObject.h"

class VTK_COMMON_EXPORT vtkSerializable
{
public:

  // Description:
  // Serializes this instance in to a byte-stream.
  // Implemented by concrete classes.
  virtual void Serialize( unsigned char*& buffer, vtkIdType &bufferSize )=0;

  // Description:
  // De-serializes this instance from a byte-stream.
  // Implemented by concrete classes.
  virtual void Deserialize(
      unsigned char* buffer, const vtkIdType &bufferSize )=0;

protected:
  vtkSerializable();
  virtual ~vtkSerializable();
};

#endif /* VTKSERIALIZABLE_H_ */
