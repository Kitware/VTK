/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReferenceCount.h
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
// .NAME vtkReferenceCount - subclasses of this object are reference counted
// .SECTION Description
// vtkReferenceCount is the base class for objects that are reference counted. 
// Objects that are reference counted exist as long as another object
// uses them. Once the last reference to a reference counted object is 
// removed, the object will spontaneously destruct. 

// .SECTION Caveats
// Note: in vtk objects are generally created with combinations of 
// New/Delete() methods. This works great until you want to allocate
// objects off the stack (i.e., automatic objects). Automatic objects,
// when automatically deleted (by exiting scope), will cause warnings to
// occur. You can avoid this by turing reference counting off (i.e., use
// the method ReferenceCountingOff()), but we strongly encourage you
// to not do this. Instead allocate your objects with New().

// .SECTION See Also
// vtkDataArray vtkLookupTable vtkTCoords vtkCellTypes vtkCellLinks
// vtkNormals vtkPoints vtkScalars vtkTensors vtkField vtkVectors

#ifndef __vtkReferenceCount_h
#define __vtkReferenceCount_h

#include "vtkObject.h"

class VTK_EXPORT vtkReferenceCount : public vtkObject
{
public:
  // Description:
  // Construct with initial reference count = 1 and reference counting on.
  vtkReferenceCount();

  // Description:
  // Overload vtkObject's Delete() method. For reference counted objects the
  // Delete() method simply unregisters the use of the object. This may or
  // may not result in the destruction of the object, depending upon whether 
  // another object is referencing it.
  void Delete();

  // Description:
  // Destructor for reference counted objects. Reference counted objects should 
  // almost always use the combination of new/Delete() to create and delete 
  // objects. Automatic reference counted objects (i.e., creating them on the 
  // stack) are not encouraged. However, if you desire to do this, you will 
  // have to use the ReferenceCountingOff() method to avoid warning messages 
  // when the objects are automatically deleted upon scope termination.
  ~vtkReferenceCount();

  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkReferenceCount *New() {return new vtkReferenceCount;};
  const char *GetClassName() {return "vtkReferenceCount";};

  // Description:
  // Increase the reference count (mark as used by another object).
  void Register(vtkObject* o);

  // Description:
  // Decrease the reference count (release by another object).
  virtual void UnRegister(vtkObject* o);

  // Description:
  // Return the current reference count of this object.
  int  GetReferenceCount() {return this->ReferenceCount;};

  // Description:
  // Turn off reference counting for this object. This allows you to create
  // automatic reference counted objects and avoid warning messages when scope
  // is existed. (Note: It is preferable to use the combination New/Delete() 
  // to create and delete vtk objects.)
  void ReferenceCountingOff() { this->ReferenceCounting = 0;};

  // Description:
  // Sets the reference count (use with care)
  void SetReferenceCount(int);
  
private:
  int ReferenceCount;      // Number of uses of this object by other objects
  int ReferenceCounting; // Turn on/off reference counting mechanism
};


#endif

