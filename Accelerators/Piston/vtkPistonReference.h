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
/**
 * @class   vtkPistonReference
 * @brief   Lower level handle on GPU resident data.
 *
 *
 * This class is internal storage for the vtkPistonDataObject class.
 * Essentially this is just a handle, in the form of an opaque void
 * pointer, with enough contextual information to determine what is
 * actually pointed to in order for to cast it back into a usable form.
 * The .cu files use this directly instead of vtkPistonDataObject
 * to keep the GPU/CPU code conceptually distinct.
 *
 * @sa
 * vtkPistonDataObject
*/

#ifndef vtkPistonReference_h
#define vtkPistonReference_h

namespace vtkpiston {
  void DeleteData(vtkPistonReference *tr);
  void DeepCopy(vtkPistonReference *self, vtkPistonReference *other);
}

class vtkPistonReference {
public:

    vtkPistonReference() : mtime(0), type(-1), data(NULL)
    {
      VTK_LEGACY_BODY(vtkPistonReference::vtkPistonReference, "VTK 6.3");
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

    vtkMTimeType mtime; //creation time of the data
    int type; //description of what data holds
    void *data; //the payload on GPU
};

#endif /* vtkPistonReference_h */
// VTK-HeaderTest-Exclude: vtkPistonReference.h
