/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmf3ArraySelection.cxx
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkXdmf3ArraySelection.h"

//==============================================================================
void vtkXdmf3ArraySelection::Merge(const vtkXdmf3ArraySelection& other)
{
  vtkXdmf3ArraySelection::const_iterator iter = other.begin();
  for (; iter != other.end(); ++iter)
  {
    (*this)[iter->first] = iter->second;
  }
}

//--------------------------------------------------------------------------
void vtkXdmf3ArraySelection::AddArray(const char* name, bool status)
{
  (*this)[name] = status;
}

//--------------------------------------------------------------------------
bool vtkXdmf3ArraySelection::ArrayIsEnabled(const char* name)
{
  vtkXdmf3ArraySelection::iterator iter = this->find(name);
  if (iter != this->end())
  {
    return iter->second;
  }

  // don't know anything about this array, enable it by default.
  return true;
}

//--------------------------------------------------------------------------
bool vtkXdmf3ArraySelection::HasArray(const char* name)
{
  vtkXdmf3ArraySelection::iterator iter = this->find(name);
  return (iter != this->end());
}

//--------------------------------------------------------------------------
int vtkXdmf3ArraySelection::GetArraySetting(const char* name)
{
  return this->ArrayIsEnabled(name)? 1 : 0;
}

//--------------------------------------------------------------------------
void vtkXdmf3ArraySelection::SetArrayStatus(const char* name, bool status)
{
  this->AddArray(name, status);
}

//--------------------------------------------------------------------------
const char* vtkXdmf3ArraySelection::GetArrayName(int index)
{
  int cc=0;
  for (vtkXdmf3ArraySelection::iterator iter = this->begin();
      iter != this->end(); ++iter)
  {
    if (cc==index)
    {
      return iter->first.c_str();
    }
    cc++;
  }
  return NULL;
}

//--------------------------------------------------------------------------
int vtkXdmf3ArraySelection::GetNumberOfArrays()
{
  return static_cast<int>(this->size());
}
