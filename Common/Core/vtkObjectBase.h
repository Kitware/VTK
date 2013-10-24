/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObjectBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkObjectBase - abstract base class for most VTK objects
// .SECTION Description
// vtkObjectBase is the base class for all reference counted classes
// in the VTK. These classes include vtkCommand classes, vtkInformationKey
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
// vtkObject vtkCommand vtkInformationKey

#ifndef __vtkObjectBase_h
#define __vtkObjectBase_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkAtomicInt.h"
#include "vtkIndent.h"
#include "vtkSystemIncludes.h"

class vtkGarbageCollector;
class vtkGarbageCollectorToObjectBaseFriendship;
class vtkWeakPointerBase;
class vtkWeakPointerBaseToObjectBaseFriendship;

class VTKCOMMONCORE_EXPORT vtkObjectBase
{
  // Description:
  // Return the class name as a string. This method is overridden
  // in all subclasses of vtkObjectBase with the vtkTypeMacro found
  // in vtkSetGet.h.
  virtual const char* GetClassNameInternal() const { return "vtkObjectBase"; }
public:

#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
  // Avoid windows name mangling.
# define GetClassNameA GetClassName
# define GetClassNameW GetClassName
#endif

  // Description:
  // Return the class name as a string.
  const char* GetClassName() const;

#ifdef VTK_WORKAROUND_WINDOWS_MANGLE
# undef GetClassNameW
# undef GetClassNameA
  //BTX
  // Define possible mangled names.
  const char* GetClassNameA() const;
  const char* GetClassNameW() const;
  //ETX
#endif

  // Description:
  // Return 1 if this class type is the same type of (or a subclass of)
  // the named class. Returns 0 otherwise. This method works in
  // combination with vtkTypeMacro found in vtkSetGet.h.
  static int IsTypeOf(const char *name);

  // Description:
  // Return 1 if this class is the same type of (or a subclass of)
  // the named class. Returns 0 otherwise. This method works in
  // combination with vtkTypeMacro found in vtkSetGet.h.
  virtual int IsA(const char *name);

  // Description:
  // Delete a VTK object.  This method should always be used to delete
  // an object when the New() method was used to create it. Using the
  // C++ delete method will not work with reference counting.
  virtual void Delete();

  // Description:
  // Delete a reference to this object.  This version will not invoke
  // garbage collection and can potentially leak the object if it is
  // part of a reference loop.  Use this method only when it is known
  // that the object has another reference and would not be collected
  // if a full garbage collection check were done.
  virtual void FastDelete();

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
  virtual void Register(vtkObjectBase* o);

  // Description:
  // Decrease the reference count (release by another object). This
  // has the same effect as invoking Delete() (i.e., it reduces the
  // reference count by 1).
  virtual void UnRegister(vtkObjectBase* o);

  // Description:
  // Return the current reference count of this object.
  int  GetReferenceCount()
  {
    return this->ReferenceCount;
  }

  // Description:
  // Sets the reference count. (This is very dangerous, use with care.)
  void SetReferenceCount(int);

  // Description:
  // Legacy.  Do not call.
  void PrintRevisions(ostream&) {}

protected:
  vtkObjectBase();
  virtual ~vtkObjectBase();

  virtual void CollectRevisions(ostream&) {} // Legacy; do not use!

  vtkAtomicInt<vtkTypeInt32> ReferenceCount;
  vtkWeakPointerBase **WeakPointers;

  // Internal Register/UnRegister implementation that accounts for
  // possible garbage collection participation.  The second argument
  // indicates whether to participate in garbage collection.
  virtual void RegisterInternal(vtkObjectBase*, int check);
  virtual void UnRegisterInternal(vtkObjectBase*, int check);

  // See vtkGarbageCollector.h:
  virtual void ReportReferences(vtkGarbageCollector*);

private:
  //BTX
  friend VTKCOMMONCORE_EXPORT ostream& operator<<(ostream& os, vtkObjectBase& o);
  friend class vtkGarbageCollectorToObjectBaseFriendship;
  friend class vtkWeakPointerBaseToObjectBaseFriendship;
  //ETX
protected:
//BTX
  vtkObjectBase(const vtkObjectBase&) {}
  void operator=(const vtkObjectBase&) {}
//ETX
};

#endif

// VTK-HeaderTest-Exclude: vtkObjectBase.h
