/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkValueHolder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkValueHolder
//
// .SECTION Description
//

#ifndef __vtkValueHolder_h
#define __vtkValueHolder_h

#include "vtkDataValue.h"

// Forward declarations.
class vtkMark;

template <typename T>
class vtkValueHolder
{
public:
  vtkValueHolder() : Dirty(true), Set(false) { }

  void UnsetValue(){Set = false;}
  void SetValue(vtkValue<T> v){Dirty = true; Set = true; Value = v;}
  vtkValue<T>& GetValue(){return Value;}

  T* GetArray(vtkMark* m)
    {
    this->Update(m);
    if (this->Cache.size() == 0)
      {
      return NULL;
      }
    return &(this->Cache[0]);
    }

  T GetConstant(vtkMark* m)
    {
    this->Update(m);
    if (this->Cache.size() > 0)
      {
      return this->Cache[0];
      }
    return this->Value.GetConstant();
    }

  bool IsSet(){return this->Set;}

  bool IsDirty(){return this->Dirty;}
  void SetDirty(bool b){this->Dirty = b;}

  void Update(vtkMark* m);

protected:
  vtkValue<T> Value;
  vtkstd::vector<T> Cache; // STL required.
  bool Dirty;
  bool Set;
};

#endif // __vtkValueHolder_h
