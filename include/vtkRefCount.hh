/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRefCount.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkRefCount - subclasses of this object are reference counted
// .SECTION Description
// vtkRefCount is the base class for objects that are reference counted. 
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

// .SECTION See Also
// vtkLookupTable vtkTCoords vtkCellList vtkLinkList vtkNormals vtkPoints
// vtkScalars vtkTensors vtkUserDefined vtkVectors

#ifndef __vtkRefCount_hh
#define __vtkRefCount_hh

#include "vtkObject.hh"

class vtkRefCount : public vtkObject
{
public:
  vtkRefCount();
  void Delete();
  ~vtkRefCount();
  void PrintSelf(ostream& os, vtkIndent indent);
  char *GetClassName() {return "vtkRefCount";};

  void Register(vtkObject* o);
  void UnRegister(vtkObject* o);
  int  GetRefCount() {return this->RefCount;};
  void ReferenceCountingOff();

private:
  int RefCount;      // Number of uses of this object by other objects
  int ReferenceCounting; // Turn on/off reference counting mechanism
};

// Description:
// Turn off reference counting for this object. This allows you to create
// automatic reference counted objects and avoid warning messages when scope
// is existed. (Note: It is preferable to use the combination new/Delete() 
// to create and delete vtk objects.)
inline void vtkRefCount::ReferenceCountingOff()
{
  this->ReferenceCounting = 0;
}

#endif

