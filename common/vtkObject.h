/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObject.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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
// Note: in vtk objects are generally created with combinations of 
// new/Delete() methods. This works great until you want to allocate
// objects off the stack (i.e., automatic objects). Automatic objects,
// when automatically deleted (by exiting scope), will cause warnings to
// occur. You can avoid this by turing reference counting off (i.e., use
// the method ReferenceCountingOff()).


#ifndef __vtkObject_h
#define __vtkObject_h

#include <iostream.h>
#include "vtkIndent.h"
#include "vtkTimeStamp.h"
#include "vtkSetGet.h"


class VTK_EXPORT vtkObject 
{
public:
  vtkObject(); 
  virtual ~vtkObject(); 

  // Description:
  // Return the class name as a string.
  virtual const char *GetClassName() {return "vtkObject";};

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
  // Chaining method to print an object's instance variables, as well as
  // its superclasses.
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Print an object to an ostream.
  void Print(ostream& os);

  // Description:
  // Support methods for the printing process.
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
  
  // Reference Counting

  // Description:
  // Increase the reference count (mark as used by another object).
  void Register(vtkObject* o);


  // Description:
  // Decrease the reference count (release by another object).
  virtual void UnRegister(vtkObject* o);

  int  GetReferenceCount() {return this->ReferenceCount;};
  void ReferenceCountingOff();

  // Description:
  // Sets the reference count (use with care)
  void SetReferenceCount(int);




protected:
  unsigned char Debug;         // Enable debug messages
  vtkTimeStamp MTime; // Keep track of modification time
  int ReferenceCount;      // Number of uses of this object by other objects
  int ReferenceCounting; // Turn on/off reference counting mechanism

private:
  //BTX
  friend VTK_EXPORT ostream& operator<<(ostream& os, vtkObject& o);
  //ETX
};

inline void vtkObject::Modified()
{
  this->MTime.Modified();
}


// Description:
// Turn off reference counting for this object. This allows you to create
// automatic reference counted objects and avoid warning messages when scope
// is existed. (Note: It is preferable to use the combination new/Delete() 
// to create and delete vtk objects.)
inline void vtkObject::ReferenceCountingOff()
{
  this->ReferenceCounting = 0;
}


#endif

