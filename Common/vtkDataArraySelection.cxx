/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArraySelection.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataArraySelection.h"
#include "vtkObjectFactory.h"
#include "vtkVector.txx"

vtkCxxRevisionMacro(vtkDataArraySelection, "1.2");
vtkStandardNewMacro(vtkDataArraySelection);

//----------------------------------------------------------------------------
vtkDataArraySelection::vtkDataArraySelection()
{
  this->ArrayNames = ArrayNamesType::New();
  this->ArraySettings = ArraySettingsType::New();
}

//----------------------------------------------------------------------------
vtkDataArraySelection::~vtkDataArraySelection()
{
  this->ArraySettings->Delete();
  this->ArrayNames->Delete();
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Number of Arrays: " << this->GetNumberOfArrays() << "\n";
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::EnableArray(const char* name)
{
  vtkIdType pos=0;
  if(this->ArrayNames->FindItem(name, pos) == VTK_OK)
    {
    int value = 0;
    this->ArraySettings->GetItemNoCheck(pos, value);
    if(!value)
      {
      this->ArraySettings->SetItemNoCheck(pos, 1);
      this->Modified();
      }
    return;
    }
  this->ArrayNames->AppendItem(name);
  this->ArraySettings->AppendItem(1);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::DisableArray(const char* name)
{
  vtkIdType pos=0;
  if(this->ArrayNames->FindItem(name, pos) == VTK_OK)
    {
    int value = 0;
    this->ArraySettings->GetItemNoCheck(pos, value);
    if(value)
      {
      this->ArraySettings->SetItemNoCheck(pos, 0);
      this->Modified();
      }
    return;
    }
  this->ArrayNames->AppendItem(name);
  this->ArraySettings->AppendItem(0);
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkDataArraySelection::ArrayIsEnabled(const char* name)
{
  // Check if there is a specific entry for this array.
  vtkIdType pos=0;
  int result=0;
  if((this->ArrayNames->FindItem(name, pos) == VTK_OK) &&
     (this->ArraySettings->GetItem(pos, result) == VTK_OK))
    {
    return result;
    }
  
  // The array does not have an entry.  Assume it is disabled.
  return 0;
}

//----------------------------------------------------------------------------
int vtkDataArraySelection::ArrayExists(const char* name)
{
  // Check if there is a specific entry for this array.
  vtkIdType pos=0;
  int result=0;
  if( this->ArrayNames->FindItem(name, pos) == VTK_OK )
    {
    return 1;
    }
  
  // The array does not have an entry. 
  return 0;
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::EnableAllArrays()
{
  int modified = 0;
  vtkIdType i;
  for(i=0;i < this->ArraySettings->GetNumberOfItems();++i)
    {
    int value = 0;
    this->ArraySettings->GetItemNoCheck(i, value);
    if(!value)
      {
      this->ArraySettings->SetItemNoCheck(i, 1);
      modified = 1;
      }
    }
  if(modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::DisableAllArrays()
{
  int modified = 0;
  vtkIdType i;
  for(i=0;i < this->ArraySettings->GetNumberOfItems();++i)
    {
    int value = 0;
    this->ArraySettings->GetItemNoCheck(i, value);
    if(value)
      {
      this->ArraySettings->SetItemNoCheck(i, 0);
      modified = 1;
      }
    }
  if(modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkDataArraySelection::GetNumberOfArrays()
{
  return this->ArrayNames->GetNumberOfItems();
}

//----------------------------------------------------------------------------
const char* vtkDataArraySelection::GetArrayName(int index)
{
  const char* n = 0;
  if(this->ArrayNames->GetItem(index, n) == VTK_OK)
    {
    return n;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkDataArraySelection::GetArraySetting(int index)
{
  int n = 0;
  if(this->ArraySettings->GetItem(index, n) == VTK_OK)
    {
    return n;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::RemoveAllArrays()
{
  if(this->GetNumberOfArrays() > 0)
    {
    this->ArrayNames->RemoveAllItems();
    this->ArraySettings->RemoveAllItems();
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::SetArrays(const char* const* names, int numArrays)
{
  // This function is called only by the filter owning the selection.
  // It should not call Modified() because array settings are not
  // changed.
  
  // Create a new map for this set of arrays.
  ArrayNamesType* newNames = ArrayNamesType::New();
  ArraySettingsType* newSettings = ArraySettingsType::New();
  
  // Allocate.
  newNames->SetSize(numArrays);
  newSettings->SetSize(numArrays);
  newNames->ResizeOn();
  newSettings->ResizeOn();
  
  // Fill with settings for all arrays.
  int i;
  for(i=0;i < numArrays; ++i)
    {
    // Add this array.
    newNames->AppendItem(names[i]);
    
    // Fill in the setting.
    vtkIdType pos=0;
    int result=0;
    if((this->ArrayNames->FindItem(names[i], pos) == VTK_OK) &&
       (this->ArraySettings->GetItem(pos, result) == VTK_OK))
      {
      // Copy the old setting for this array.
      newSettings->AppendItem(result);
      }
    else
      {
      // No setting existed, default to on.
      newSettings->AppendItem(1);
      }
    }
  
  // Delete the old map and save the new one.
  this->ArrayNames->Delete();
  this->ArraySettings->Delete();
  this->ArrayNames = newNames;
  this->ArraySettings = newSettings;
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::CopySelections(vtkDataArraySelection* selections)
{
  if(this == selections)
    {
    return;
    }
  this->RemoveAllArrays();
  
  const char* name=0;
  int setting=0;
  vtkIdType i;
  for(i=0;i < selections->ArrayNames->GetNumberOfItems();++i)
    {
    selections->ArrayNames->GetItemNoCheck(i, name);
    selections->ArraySettings->GetItemNoCheck(i, setting);
    this->ArrayNames->AppendItem(name);
    this->ArraySettings->AppendItem(setting);
    }
  this->Modified();
}
