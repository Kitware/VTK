/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStack.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkStack - create and manipulate lists of objects
// .SECTION Description
// vtkStack is a general object for creating and manipulating lists
// of objects. vtkStack also serves as a base class for lists of
// specific types of objects.

#ifndef __vtkStack_h
#define __vtkStack_h

#include "vtkObject.h"

//BTX begin tcl exclude
class vtkStackElement //;prevents pick-up by man page generator
{
 public:
  vtkStackElement():Item(NULL),Next(NULL) {};
  vtkObject *Item;
  vtkStackElement *Next;
};
//ETX end tcl exclude

class VTK_EXPORT vtkStack : public vtkObject
{
public:
  static vtkStack *New();

  vtkTypeMacro(vtkStack,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add an object to the top of the stack. Does not prevent duplicate entries.
  void Push(vtkObject *);

  // Description:
  // Remove an object from the top of the list.
  vtkObject *Pop();

  // Description:
  // Return the number of objects in the stack.
  vtkObject *GetTop();

  // Description:
  // Return the number of objects in the stack.
  int  GetNumberOfItems();

protected:
  vtkStack();
  ~vtkStack();
  vtkStack(const vtkStack&);
  void operator=(const vtkStack&);

  int NumberOfItems;
  vtkStackElement *Top;
  vtkStackElement *Bottom;
};

#endif
