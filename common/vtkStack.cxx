/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStack.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkStack.h"

// Description:
// Construct with empty stack.
vtkStack::vtkStack()
{
  this->NumberOfItems = 0;
  this->Top = NULL;
  this->Bottom = NULL;
}

vtkStack::~vtkStack()
{
  vtkStackElement *p;  

  for ( p=this->Top; p != NULL; p = p->Next )
    {
    delete p;
    }
}

// Description:
// Add an object to the top of the stack. Does not prevent duplicate entries.
void vtkStack::Push(vtkObject *a)
{
  vtkStackElement *elem;

  elem = new vtkStackElement;
  
  if ( this->Top == NULL )
    this->Bottom = elem;
  else
    elem->Next = this->Top;

  this->Top = elem;
  elem->Item = a;
  this->NumberOfItems++;
}

// Description:
// Remove an object from the top of the list.
vtkObject *vtkStack::Pop()
{
  vtkObject *item;
  vtkStackElement *next;
  
  if ( this->Top == NULL ) return NULL;

  item = this->Top->Item;
  next = this->Top->Next;
  delete this->Top;

  if ( this->Top == this->Bottom )
    this->Top = this->Bottom = NULL;
  else
    this->Top = next;

  this->NumberOfItems--;
  return item;
}

// Description:
// Return the number of objects in the stack.
vtkObject *vtkStack::GetTop()
{
  if ( this->Top != NULL )
    return this->Top->Item;
  else
    return NULL;
}

// Description:
// Return the number of objects in the stack.
int vtkStack::GetNumberOfItems()
{
  return this->NumberOfItems;
}

void vtkStack::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Number Of Items: " << this->NumberOfItems << "\n";
}








