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

#include <vtkstd/vector>
#include <vtkstd/string>
#include <vtkstd/algorithm>

vtkCxxRevisionMacro(vtkDataArraySelection, "1.13");
vtkStandardNewMacro(vtkDataArraySelection);

class vtkDataArraySelectionArrayNamesType: public vtkstd::vector<vtkstd::string> {};
class vtkDataArraySelectionArraySettingsType: public vtkstd::vector<int> {};

//----------------------------------------------------------------------------
vtkDataArraySelection::vtkDataArraySelection()
{
  this->ArrayNames = new vtkDataArraySelectionArrayNamesType;
  this->ArraySettings = new vtkDataArraySelectionArraySettingsType;
}

//----------------------------------------------------------------------------
vtkDataArraySelection::~vtkDataArraySelection()
{
  delete this->ArraySettings;
  delete this->ArrayNames;
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
  vtkDebugMacro("Enabling array \"" << name << "\".");
  vtkstd::vector<vtkstd::string>::iterator i =
    vtkstd::find(this->ArrayNames->begin(), this->ArrayNames->end(), vtkstd::string(name));
  if(i != this->ArrayNames->end())
    {
    int& setting = (*this->ArraySettings)[i-this->ArrayNames->begin()];
    if(!setting)
      {
      setting = 1;
      this->Modified();
      }
    }
  else
    {
    this->ArrayNames->push_back(name);
    this->ArraySettings->push_back(1);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::DisableArray(const char* name)
{
  vtkDebugMacro("Disabling array \"" << name << "\".");
  vtkstd::vector<vtkstd::string>::iterator i =
    vtkstd::find(this->ArrayNames->begin(), this->ArrayNames->end(), vtkstd::string(name));
  if(i != this->ArrayNames->end())
    {
    int& setting = (*this->ArraySettings)[i - this->ArrayNames->begin()];
    if(setting)
      {
      setting = 0;
      this->Modified();
      }
    }
  else
    {
    this->ArrayNames->push_back(name);
    this->ArraySettings->push_back(0);
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkDataArraySelection::ArrayIsEnabled(const char* name)
{
  // Check if there is a specific entry for this array.
  vtkstd::vector<vtkstd::string>::iterator i =
    vtkstd::find(this->ArrayNames->begin(), this->ArrayNames->end(), vtkstd::string(name));
  if(i != this->ArrayNames->end())
    {
    return (*this->ArraySettings)[i - this->ArrayNames->begin()];
    }
  
  // The array does not have an entry.  Assume it is disabled.
  return 0;
}

//----------------------------------------------------------------------------
int vtkDataArraySelection::ArrayExists(const char* name)
{
  // Check if there is a specific entry for this array.
  vtkstd::vector<vtkstd::string>::iterator i =
    vtkstd::find(this->ArrayNames->begin(), this->ArrayNames->end(), vtkstd::string(name));
  return i != this->ArrayNames->end();
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::EnableAllArrays()
{
  vtkDebugMacro("Enabling all arrays.");
  int modified = 0;
  for(vtkstd::vector<int>::iterator i = this->ArraySettings->begin();
      i != this->ArraySettings->end(); ++i)
    {
    if(!*i)
      {
      *i = 1;
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
  vtkDebugMacro("Disabling all arrays.");
  int modified = 0;
  for(vtkstd::vector<int>::iterator i = this->ArraySettings->begin();
      i != this->ArraySettings->end(); ++i)
    {
    if(*i)
      {
      *i = 0;
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
  return static_cast<int>(this->ArrayNames->size());
}

//----------------------------------------------------------------------------
const char* vtkDataArraySelection::GetArrayName(int index)
{
  vtkstd::vector<vtkstd::string>::iterator i = this->ArrayNames->begin()+index;
  if(i >= this->ArrayNames->begin() && i < this->ArrayNames->end())
    {
    return i->c_str();
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkDataArraySelection::GetArraySetting(int index)
{
  vtkstd::vector<int>::iterator i = this->ArraySettings->begin()+index;
  if(i >= this->ArraySettings->begin() && i < this->ArraySettings->end())
    {
    return *i;
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::RemoveAllArrays()
{
  vtkDebugMacro("Removing all arrays.");
  if(this->GetNumberOfArrays() > 0)
    {
    this->ArrayNames->erase(this->ArrayNames->begin(),
                            this->ArrayNames->end());
    this->ArraySettings->erase(this->ArraySettings->begin(),
                               this->ArraySettings->end());
    this->Modified();
    }
}

//----------------------------------------------------------------------------
int vtkDataArraySelection::AddArray(const char* name)
{
  vtkDebugMacro("Adding array \"" << name << "\".");
  
  // This function is called only by the filter owning the selection.
  // It should not call Modified() because array settings are not
  // changed.
  if(this->ArrayExists(name))
    {
    return 0;
    }
  this->ArrayNames->push_back(name);
  this->ArraySettings->push_back(1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::SetArrays(const char* const* names, int numArrays)
{
  this->SetArraysWithDefault(names, numArrays, 1);
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::SetArraysWithDefault(const char* const* names,
                                                 int numArrays,
                                                 int defaultStatus)
{
  // This function is called only by the filter owning the selection.
  // It should not call Modified() because array settings are not
  // changed.

  vtkDebugMacro("Settings arrays to given list of " << numArrays
                << " arrays.");

  // Create a new map for this set of arrays.
  vtkDataArraySelectionArrayNamesType* newNames =
    new vtkDataArraySelectionArrayNamesType;
  vtkDataArraySelectionArraySettingsType* newSettings =
    new vtkDataArraySelectionArraySettingsType;
  
  newNames->reserve(numArrays);
  newSettings->reserve(numArrays);
  
  // Fill with settings for all arrays.
  int i;
  for(i=0;i < numArrays; ++i)
    {
    // Add this array.
    newNames->push_back(names[i]);
    
    // Fill in the setting.  Use the old value if available.
    // Otherwise, default to on.
    vtkstd::vector<vtkstd::string>::iterator it =
      vtkstd::find(this->ArrayNames->begin(), this->ArrayNames->end(),
                   vtkstd::string(names[i]));
    int setting = defaultStatus?1:0;
    if(it != this->ArrayNames->end())
      {
      setting = (*this->ArraySettings)[it - this->ArrayNames->begin()];
      }
    newSettings->push_back(setting);
    }
  
  // Delete the old map and save the new one.
  delete this->ArrayNames;
  delete this->ArraySettings;
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
  
  vtkDebugMacro("Copying arrays and settings from " << selections << ".");

  this->RemoveAllArrays();
  
  this->ArrayNames->insert(this->ArrayNames->begin(),
                           selections->ArrayNames->begin(),
                           selections->ArrayNames->end());
  this->ArraySettings->insert(this->ArraySettings->begin(),
                              selections->ArraySettings->begin(),
                              selections->ArraySettings->end());
  this->Modified();
}
