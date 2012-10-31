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

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObjectBase.h"
#include "vtkObject.h" // Need vtkTypeMacro

class vtkInformation;

class VTKCOMMONCORE_EXPORT vtkInformationKey : public vtkObjectBase
{
public:
  vtkTypeMacro(vtkInformationKey,vtkObjectBase);
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
  virtual void ShallowCopy(vtkInformation* from, vtkInformation* to)=0;

  // Description:
  // Duplicate (new instance created) the entry associated with this key from
  // one information object to another (new instances of any contained
  // vtkInformation and vtkInformationVector objects are created).
  // Default implementation simply calls ShallowCopy().
  virtual void DeepCopy(vtkInformation *from, vtkInformation *to)
    { this->ShallowCopy(from, to); }

  // Description:
  // Check whether this key appears in the given information object.
  virtual int Has(vtkInformation* info);

  // Description:
  // Remove this key from the given information object.
  virtual void Remove(vtkInformation* info);

  // Description:
  // Report a reference this key has in the given information object.
  virtual void Report(vtkInformation* info, vtkGarbageCollector* collector);

  // Description:
  // Print the key's value in an information object to a stream.
  void Print(vtkInformation* info);
  virtual void Print(ostream& os, vtkInformation* info);

protected:
  const char* Name;
  const char* Location;

  // Set/Get the value associated with this key instance in the given
  // information object.
  void SetAsObjectBase(vtkInformation* info, vtkObjectBase* value);
  const vtkObjectBase* GetAsObjectBase(vtkInformation* info) const;
  vtkObjectBase* GetAsObjectBase(vtkInformation* info);

  // Report the object associated with this key instance in the given
  // information object to the collector.
  void ReportAsObjectBase(vtkInformation* info,
                          vtkGarbageCollector* collector);

  // Helper for debug leaks support.
  void ConstructClass(const char*);

private:
  vtkInformationKey(const vtkInformationKey&);  // Not implemented.
  void operator=(const vtkInformationKey&);  // Not implemented.
};

// Macros to define an information key instance in a C++ source file.
// The corresponding method declaration must appear in the class
// definition in the header file.
#define vtkInformationKeyMacro(CLASS, NAME, type)             \
 vtkInformation##type##Key* CLASS::NAME()                     \
   {                                                          \
   static vtkInformation##type##Key* CLASS##_##NAME =         \
        new vtkInformation##type##Key(#NAME, #CLASS);         \
   return CLASS##_##NAME;                                     \
   }
#define vtkInformationKeyRestrictedMacro(CLASS, NAME, type, required)   \
 vtkInformation##type##Key* CLASS::NAME()                               \
   {                                                                    \
   static vtkInformation##type##Key* CLASS##_##NAME =                   \
     new vtkInformation##type##Key(#NAME, #CLASS, required);            \
   return CLASS##_##NAME;                                               \
   }

#endif
