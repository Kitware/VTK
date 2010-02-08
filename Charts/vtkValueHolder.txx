/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkValueHolder.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkValueHolderDef
//
// .SECTION Description
//

#ifndef __vtkValueHolderDef_h
#define __vtkValueHolderDef_h

#include "vtkMark.h"

template <typename T>
void vtkValueHolder<T>::Update(vtkMark* m)
{
  if (!this->Dirty)
    {
    return;
    }
  vtkDataElement d = m->GetData().GetData(m);
  vtkIdType numChildren = 1;
  if(d.IsValid())
    {
    numChildren = d.GetNumberOfChildren();
    }

  this->Cache.resize(numChildren);
  if (this->Value.IsConstant())
    {
    for (vtkIdType i = 0; i < numChildren; ++i)
      {
      this->Cache[i] = this->Value.GetConstant();
      }
    }
  else
    {
    for (vtkIdType i = 0; i < numChildren; ++i)
      {
      m->SetIndex(i);
      vtkDataElement e = d.GetChild(i);
      this->Cache[i] = this->Value.GetFunction()(m, e);
      }
    }
  this->Dirty = false;
}


#endif // __vtkValueHolderDef_h
