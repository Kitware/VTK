/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationKey.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInformationKey - Superclass for vtkInformation keys.
// .SECTION Description
// vtkInformationKey is the superclass for all keys used to access the
// map represented by vtkInformation.  The vtkInformation::Set and
// vtkInformation::Get methods of vtkInformation are accessed by
// information keys.  A key is a pointer to an instance of a subclass
// of vtkInformationKey.  The type of the subclass determines the
// overload of Set/Get that is selected.  This ensures that the type
// of value stored in a vtkInformation instance corresponding to a
// given key matches the type expected for that key.

#ifndef __vtkInformationKey_h
#define __vtkInformationKey_h

#include "vtkObjectBase.h"
#include "vtkObject.h" // Need vtkTypeRevisionMacro

class vtkInformation;

class VTK_FILTERING_EXPORT vtkInformationKey : public vtkObjectBase
{
public:
  vtkTypeRevisionMacro(vtkInformationKey,vtkObjectBase);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Prevent normal vtkObject reference counting behavior.
  virtual void Register(vtkObjectBase*);

  // Description:
  // Prevent normal vtkObject reference counting behavior.
  virtual void UnRegister(vtkObjectBase*);

  // Description:
  // Get the name of the key.  This is not the type of the key, but
  // the name of the key instance.
  const char* GetName();

  // Description:
  // Get the location of the key.  This is the name of the class in
  // which the key is defined.
  const char* GetLocation();

  // Description:
  // Key instances are static data that need to be created and
  // destroyed.  The constructor and destructor must be public.  The
  // name of the static instance and the class in which it is defined
  // should be passed to the constructor.  They must be string
  // literals because the strings are not copied.
  vtkInformationKey(const char* name, const char* location);
  ~vtkInformationKey();

  // Description:
  // Copy the entry associated with this key from one information
  // object to another.  If there is no entry in the first information
  // object for this key, the value is removed from the second.
  virtual void Copy(vtkInformation* from, vtkInformation* to)=0;

  // Description:
  // Remove this key from the given information object.
  void Remove(vtkInformation* info);

protected:
  const char* Name;
  const char* Location;

  // Set/Get the value associated with this key instance in the given
  // information object.
  void SetAsObjectBase(vtkInformation* info, vtkObjectBase* value);
  vtkObjectBase* GetAsObjectBase(vtkInformation* info);

  // Helper for debug leaks support.
  void ConstructClass(const char*);

private:
  vtkInformationKey(const vtkInformationKey&);  // Not implemented.
  void operator=(const vtkInformationKey&);  // Not implemented.
};

#endif
