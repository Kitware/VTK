/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObject.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkObject - abstract base class for most of the vtk objects
// .SECTION Description
// vtkObject is the base class for many objects in the visualization 
// toolkit. vtkObject provides methods for tracking modification times, 
// debugging, and printing. Most objects created within the vtk 
// framework should be a subclass of vtkObject or one of its children.
// The few exceptions tend to be very small helper classes that usually
// never get instantiated or situations where multiple inheritance
// gets in the way. 
// vtkObject also performs reference counting:
// Objects that are reference counted exist as long as another object
// uses them. Once the last reference to a reference counted object is 
// removed, the object will spontaneously destruct. Typically only data
// objects that are passed between objects are reference counted.

// .SECTION Caveats
// Note: in vtk objects should always be created with combinations of 
// new/Delete() methods. This does not work when objects are allocated
// off the stack (i.e., automatic objects). Automatic objects,
// when automatically deleted (by exiting scope), will cause warnings to
// occur. You should not create vtkObjects in this manner.


#ifndef __vtkObject_h
#define __vtkObject_h

#include "vtkIndent.h"
#include "vtkTimeStamp.h"
#include "vtkSetGet.h"

class vtkSubjectHelper;
class vtkCommand;

class VTK_EXPORT vtkObject 
{
public:
  // Description:
  // Return the class name as a string. This method is defined
  // in all subclasses of vtkObject with the vtkTypeMacro found
  // in vtkSetGet.h.
  virtual const char *GetClassName() {return "vtkObject";};

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
  // Will cast the supplied object to vtkObject* is this is a safe operation
  // (i.e., a safe downcast); otherwise NULL is returned. This method is
  // defined in all subclasses of vtkObject with the vtkTypeMacro found in
  // vtkSetGet.h.
  static vtkObject *SafeDownCast(vtkObject *o);

  // Description:
  // Delete a vtk object. 
  // This method should always be used to delete an object 
  // when the new operator was used to create it. Using the C++ delete method
  // will not work with reference counting.
  virtual void Delete(); //delete a vtk object.

  // Description:
  // Create an object with Debug turned off, modified time initialized 
  // to zero, and reference counting on.
  static vtkObject *New() {return new vtkObject;};

#ifdef _WIN32
  // avoid dll boundary problems
  void* operator new( size_t tSize, const char *, int);
  void* operator new( size_t tSize );
  void operator delete( void* p );
#endif 
  
  // Description:
  // Turn debugging output on.
  virtual void DebugOn();

  // Description:
  // Turn debugging output off.
  virtual void DebugOff();
  
  // Description:
  // Get the value of the debug flag.
  unsigned char GetDebug();
  
  
  // Description:
  // Set the value of the debug flag. A non-zero value turns debugging on.
  void SetDebug(unsigned char debugFlag);
  
  // Description:
  // This method is called when vtkErrorMacro executes. It allows 
  // the debugger to break on error.
  static void BreakOnError();
  
  // Description: 
  // Return this objects modified time.
  virtual unsigned long GetMTime();

  // Description:
  // Update the modification time for this object. Many filters rely on
  // the modification time to determine if they need to recompute their
  // data.
  virtual void Modified();
  
  // Description:
  // Print an object to an ostream.
  void Print(ostream& os);

  // Description:
  // Methods invoked by print to print information about
  // the object including superclasses. Typically not called by the
  // user (use Print() instead) but used in the hierarchical print
  // process to combine the output of several classes.
  virtual void PrintSelf(ostream& os, vtkIndent indent);
  virtual void PrintHeader(ostream& os, vtkIndent indent);
  virtual void PrintTrailer(ostream& os, vtkIndent indent);

  // Description:
  // This is a global flag that controls whether any debug, warning
  // or error messages are displayed.
  static void SetGlobalWarningDisplay(int val);
  static void GlobalWarningDisplayOn(){vtkObject::SetGlobalWarningDisplay(1);};
  static void GlobalWarningDisplayOff() 
    {vtkObject::SetGlobalWarningDisplay(0);};
  static int  GetGlobalWarningDisplay();
  
  // Description:
  // Increase the reference count (mark as used by another object).
  void Register(vtkObject* o);

  // Description:
  // Decrease the reference count (release by another object).
  virtual void UnRegister(vtkObject* o);

  int  GetReferenceCount() {return this->ReferenceCount;};

  // Description:
  // Sets the reference count (use with care)
  void SetReferenceCount(int);

  // Description:
  // Allow people to add/remove/invoke observers (callbacks) to any VTK object
  // This is an implementation of the subject/observer design pattern. An 
  // observer is added by specifying an event to respond to and a vtkCommand
  // to execute. It returns an unsigned long tag which can be used later to
  // remove the event or retrieve the command.
  //BTX
  unsigned long AddObserver(unsigned long event, vtkCommand *);
  unsigned long AddObserver(const char *event, vtkCommand *);
  vtkCommand *GetCommand(unsigned long tag);
  void InvokeEvent(unsigned long event, void *callData);
  void InvokeEvent(const char *event, void *callData);
  //ETX
  void RemoveObserver(unsigned long tag);
  int HasObserver(unsigned long event);
  int HasObserver(const char *event);
  
protected:
  vtkObject(); 
  virtual ~vtkObject(); 
  vtkObject(const vtkObject&) {};
  void operator=(const vtkObject&) {};

  unsigned char Debug;         // Enable debug messages
  vtkTimeStamp MTime; // Keep track of modification time
  int ReferenceCount;      // Number of uses of this object by other objects
  vtkSubjectHelper *SubjectHelper;

private:
  //BTX
  friend VTK_EXPORT ostream& operator<<(ostream& os, vtkObject& o);
  //ETX
};

inline void vtkObject::Modified()
{
  this->MTime.Modified();
}

#endif

