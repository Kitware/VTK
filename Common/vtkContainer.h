/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContainer.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkContainer - a base class for templated containers
// .SECTION Description
// vtkContainer is a superclass for all container classes. 
// Since it does not provide any actuall data access methods, it
// is not templated, but it provides a set of method that can 
// be used on all containers. It also provide a simple reference 
// counting scheme.

// .SECTION Caveates
// Since vtkContainer and vtkAbstractList provide some pure 
// virtual methods, each object of type container will have
// v-tabe.
//
// The reference counting scheme used in vtkContainer is not
// used with debug leaks yet.

#include "vtkObject.h"

// Macro used to determine whether a class is the same class or
// a subclass of the named class.
//
#define vtkContainerTypeMacro(thisClass,superclass) \
  typedef superclass Superclass; \
  virtual const char *GetClassName() {return #thisClass;} \
  static int IsTypeOf(const char *type) \
  { \
    if ( !strcmp(#thisClass,type) ) \
      { \
      return 1; \
      } \
    return superclass::IsTypeOf(type); \
  } \
  virtual int IsA(const char *type) \
  { \
    return this->thisClass::IsTypeOf(type); \
  } \
  static thisClass* SafeDownCast(vtkObject *o) \
  { \
    if ( o && o->IsA(#thisClass) ) \
      { \
      return (thisClass *)o; \
      } \
    return NULL;\
  }

class VTK_COMMON_EXPORT vtkContainer
{
public:
  // Description:
  // Return the class name as a string.
  virtual const char* GetClassName() { return "vtkContainer"; }
  static int IsTypeOf(const char *type)
    {
    return !strcmp(type, "vtkContainer");
    }
  virtual int IsA(const char *type)
    {
    return this->vtkContainer::IsTypeOf(type);
    }  

  // Description:
  // Return the number of items currently held in this container. This
  // different from GetSize which is provided for some containers. GetSize
  // will return how many items the container can currently hold.
  virtual unsigned long GetNumberOfItems() = 0;
  
  // Description:
  // Removes all items from the container.
  virtual void RemoveAllItems() = 0;
  
  // Description:
  // The counterpart to New(), Delete simply calls UnRegister to lower the
  // reference count by one. It is no different than calling UnRegister.
  void Delete() { this->UnRegister(); }
  
  // Description:
  // Increase the reference count of this container.
  void Register();
  void Register(vtkObject *) { this->Register(); }
  
  // Description:
  // Decrease the reference count (release by another object). This has
  // the same effect as invoking Delete() (i.e., it reduces the reference
  // count by 1).
  void UnRegister();
  void UnRegister(vtkObject *) { this->UnRegister(); }

protected:
  unsigned long ReferenceCount;   
  vtkContainer();
  virtual ~vtkContainer();
};
