/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObjectBase.h
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
// .NAME vtkObjectBase - abstract base class for most VTK objects
// .SECTION Description
// vtkObjectBase is the base class for all reference counted classes
// in the VTK. These classes include vtkCommand classes, vtkContainer
// classes, and vtkObject classes.
//
// vtkObjectBase performs reference counting: objects that are
// reference counted exist as long as another object uses them. Once
// the last reference to a reference counted object is removed, the
// object will spontaneously destruct.
// 
// Constructor and destructor of the subclasses of vtkObjectBase
// should be protected, so that only New() and UnRegister() actually
// call them. Debug leaks can be used to see if there are any objects
// left with nonzero reference count.
//
// .SECTION Caveats
// Note: Objects of subclasses of vtkObjectBase should always be
// created with the New() method and deleted with the Delete()
// method. They cannot be allocated off the stack (i.e., automatic
// objects) because the constructor is a protected method.
//
// .SECTION See also
// vtkObject vtkCommand vtkContainer

#ifndef __vtkObjectBase_h
#define __vtkObjectBase_h

#include "vtkWin32Header.h"
#include "vtkIndent.h"
#include "vtkSystemIncludes.h"

class VTK_COMMON_EXPORT vtkObjectBase 
{
public:
  // Description:
  // Return the class name as a string. This method is defined
  // in all subclasses of vtkObjectBase with the vtkTypeRevisionMacro found
  // in vtkSetGet.h.
  virtual const char *GetClassName() const {return "vtkObjectBase";};

  // Description:
  // Delete a VTK object.  This method should always be used to delete
  // an object when the New() method was used to create it. Using the
  // C++ delete method will not work with reference counting.
  virtual void Delete();

  // Description:
  // Create an object with Debug turned off, modified time initialized 
  // to zero, and reference counting on.
  static vtkObjectBase *New() 
    {return new vtkObjectBase;}
  
#ifdef _WIN32
  // avoid dll boundary problems
  void* operator new( size_t tSize );
  void operator delete( void* p );
#endif 
  
  // Description:
  // Print an object to an ostream. This is the method to call
  // when you wish to see print the internal state of an object.
  void Print(ostream& os);

  // Description:
  // Methods invoked by print to print information about the object
  // including superclasses. Typically not called by the user (use
  // Print() instead) but used in the hierarchical print process to
  // combine the output of several classes.
  virtual void PrintSelf(ostream& os, vtkIndent indent);
  virtual void PrintHeader(ostream& os, vtkIndent indent);
  virtual void PrintTrailer(ostream& os, vtkIndent indent);

  // Description:
  // Increase the reference count (mark as used by another object).
  void Register(vtkObjectBase* o);

  // Description:
  // Decrease the reference count (release by another object). This has
  // the same effect as invoking Delete() (i.e., it reduces the reference
  // count by 1).
  virtual void UnRegister(vtkObjectBase* o);

  // Description:
  // Return the current reference count of this object.
  int  GetReferenceCount() 
    {return this->ReferenceCount;}

  // Description:
  // Sets the reference count. (This is very dangerous, use with care.)
  void SetReferenceCount(int);

protected:
  vtkObjectBase(); 
  virtual ~vtkObjectBase(); 

  virtual void CollectRevisions(ostream& os);
  
  int ReferenceCount;      // Number of uses of this object by other objects

private:
  //BTX
  friend VTK_COMMON_EXPORT ostream& operator<<(ostream& os, vtkObjectBase& o);
  //ETX

private:
  vtkObjectBase(const vtkObjectBase&);  // Not implemented.
  void operator=(const vtkObjectBase&);  // Not implemented.
};

#endif

