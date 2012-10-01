/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPistonReference.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPistonReference - Lower level handle on GPU resident data.
//
// .SECTION Description
// This class is internal storage for the vtkPistonDataObject class.
// Essentially this is just a handle, in the form of an opaque void
// pointer, with enough contextual information to determine what is
// actually pointed to in order for to cast it back into a usable form.
// The .cu files use this directly instead of vtkPistonDataObject
// to keep the GPU/CPU code conceptually distinct.
//
// .SECTION See Also
// vtkPistonDataObject

#ifndef VTKPistonReference_H_
#define VTKPistonReference_H_

namespace vtkpiston {
  void DeleteData(vtkPistonReference *tr);
  void DeepCopy(vtkPistonReference *self, vtkPistonReference *other);
}

class vtkPistonReference {
public:

    vtkPistonReference() : mtime(0), type(-1), data(NULL)
    {
      //cerr << "TR(" << this << ") CREATE" << endl;
    }

    ~vtkPistonReference()
    {
      //cerr << "TR(" << this << ") DELETE" << endl;
      vtkpiston::DeleteData(this);
    }

    vtkPistonReference(vtkPistonReference *other)
    {
      //cerr << "TR(" << this << ") DEEP COPY" << endl;
      vtkpiston::DeepCopy(this, other);
    }

    unsigned long int mtime; //creation time of the data
    int type; //description of what data holds
    void *data; //the payload on GPU
};

#endif /* VTKPistonReference_H_ */
// VTK-HeaderTest-Exclude: vtkPistonReference.h
