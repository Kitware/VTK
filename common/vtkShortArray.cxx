/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShortArray.cxx
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
#include "vtkShortArray.h"

// Description:
// Allocate memory for this array. Delete old storage only if necessary.
int vtkShortArray::Allocate(const int sz, const int ext)
{
  if ( sz > this->Size || this->Array == NULL )
    {
    delete [] this->Array;

    this->Size = ( sz > 0 ? sz : 1);
    if ( (this->Array = new short[this->Size]) == NULL ) return 0;
    }

  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;

  return 1;
}

// Description:
// Release storage and reset array to initial state.
void vtkShortArray::Initialize()
{
  if ( this->Array != NULL )
    {
    delete [] this->Array;
    this->Array = NULL;
    }
  this->Size = 0;
  this->MaxId = -1;
}

// Description:
// Construct with specified storage size and extend value.
vtkShortArray::vtkShortArray(const int sz, const int ext)
{
  this->Size = ( sz > 0 ? sz : 1);
  this->Array = new short[this->Size];
  this->Extend = ( ext > 0 ? ext : 1);
  this->MaxId = -1;
}

vtkShortArray::~vtkShortArray()
{
  delete [] this->Array;
}

// Description:
// Construct array from another array. Copy each element of other array.
vtkShortArray::vtkShortArray(const vtkShortArray& sa)
{
  int i;

  this->MaxId = sa.MaxId;
  this->Size = sa.Size;
  this->Extend = sa.Extend;

  this->Array = new short[this->Size];
  for (i=0; i<this->MaxId; i++)
    this->Array[i] = sa.Array[i];

}

// Description:
// Deep copy of another array.
vtkShortArray& vtkShortArray::operator=(const vtkShortArray& sa)
{
  int i;

  if ( this != &sa )
    {
    delete [] this->Array;

    this->MaxId = sa.MaxId;
    this->Size = sa.Size;
    this->Extend = sa.Extend;

    this->Array = new short[this->Size];
    for (i=0; i<=this->MaxId; i++)
      this->Array[i] = sa.Array[i];
    }
  return *this;
}

// Description:
// Append one array onto the end of this array.
void vtkShortArray::operator+=(const vtkShortArray& sa)
{
  int i, sz;

  if ( this->Size <= (sz = this->MaxId + sa.MaxId + 2) ) this->Resize(sz);

  for (i=0; i<=sa.MaxId; i++)
    {
    this->Array[this->MaxId+1+i] = sa.Array[i];
    }
  this->MaxId += sa.MaxId + 1;
}

void vtkShortArray::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "Array: " << this->Array << "\n";
  os << indent << "Size: " << this->Size << "\n";
  os << indent << "MaxId: " << this->MaxId << "\n";
  os << indent << "Extend size: " << this->Extend << "\n";
}

//
// Private function does "reallocate"
//
short *vtkShortArray::Resize(const int sz)
{
  short *newArray;
  int newSize;

  if ( sz > this->Size ) newSize = this->Size + 
    this->Extend*(((sz-this->Size)/this->Extend)+1);
  else if (sz == this->Size)
    return this->Array;
  else newSize = sz;

  if ( (newArray = new short[newSize]) == NULL )
    {
    vtkErrorMacro(<< "Cannot allocate memory\n");
    return 0;
    }

  memcpy(newArray, this->Array, 
         (sz < this->Size ? sz : this->Size) * sizeof(short));

  this->Size = newSize;
  delete [] this->Array;
  this->Array = newArray;

  return this->Array;
}
