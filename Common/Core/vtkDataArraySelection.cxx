/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataArraySelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataArraySelection.h"
#include "vtkObjectFactory.h"

#include <vector>
#include <string>

vtkStandardNewMacro(vtkDataArraySelection);

class vtkDataArraySelectionInternals
{
public:
  std::vector<std::string> ArrayNames;
  std::vector<int> ArraySettings;
};

//----------------------------------------------------------------------------
vtkDataArraySelection::vtkDataArraySelection()
{
  this->Internal = new vtkDataArraySelectionInternals;
}

//----------------------------------------------------------------------------
vtkDataArraySelection::~vtkDataArraySelection()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Number of Arrays: " << this->GetNumberOfArrays() << "\n";
  vtkIndent nindent = indent.GetNextIndent();
  int cc;
  for ( cc = 0; cc < this->GetNumberOfArrays(); cc ++ )
  {
    os << nindent << "Array: " << this->GetArrayName(cc) << " is: "
      << (this->GetArraySetting(cc)?"enabled":"disabled")
      << " (" << this->ArrayIsEnabled(this->GetArrayName(cc)) << ")" <<  endl;
  }
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::EnableArray(const char* name)
{
  vtkDebugMacro("Enabling array \"" << name << "\".");
  int index = this->GetArrayIndex(name);
  if(index >= 0)
  {
    if(!this->Internal->ArraySettings[index])
    {
      this->Internal->ArraySettings[index] = 1;
      this->Modified();
    }
  }
  else
  {
    this->Internal->ArrayNames.push_back(name);
    this->Internal->ArraySettings.push_back(1);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::DisableArray(const char* name)
{
  vtkDebugMacro("Disabling array \"" << name << "\".");
  int index = this->GetArrayIndex(name);
  if(index >= 0)
  {
    if(this->Internal->ArraySettings[index])
    {
      this->Internal->ArraySettings[index] = 0;
      this->Modified();
    }
  }
  else
  {
    this->Internal->ArrayNames.push_back(name);
    this->Internal->ArraySettings.push_back(0);
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkDataArraySelection::ArrayIsEnabled(const char* name)
{
  // Check if there is a specific entry for this array.
  int index = this->GetArrayIndex(name);
  if(index >= 0)
  {
    return this->Internal->ArraySettings[index];
  }

  // The array does not have an entry.  Assume it is disabled.
  return 0;
}

//----------------------------------------------------------------------------
int vtkDataArraySelection::ArrayExists(const char* name)
{
  // Check if there is a specific entry for this array.
  return (this->GetArrayIndex(name) >= 0)? 1:0;
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::EnableAllArrays()
{
  vtkDebugMacro("Enabling all arrays.");
  int modified = 0;
  for(std::vector<int>::iterator i = this->Internal->ArraySettings.begin();
      i != this->Internal->ArraySettings.end(); ++i)
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
  for(std::vector<int>::iterator i = this->Internal->ArraySettings.begin();
      i != this->Internal->ArraySettings.end(); ++i)
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
  return static_cast<int>(this->Internal->ArrayNames.size());
}

//----------------------------------------------------------------------------
int vtkDataArraySelection::GetNumberOfArraysEnabled()
{
  int numArrays = 0;
  for(std::vector<int>::iterator i = this->Internal->ArraySettings.begin();
      i != this->Internal->ArraySettings.end(); ++i)
  {
    if(*i)
    {
      numArrays++;
    }
  }
  return numArrays;
}

//----------------------------------------------------------------------------
const char* vtkDataArraySelection::GetArrayName(int index)
{
  if(index >= 0 && index < this->GetNumberOfArrays())
  {
    return this->Internal->ArrayNames[index].c_str();
  }
  return 0;
}

//----------------------------------------------------------------------------
int vtkDataArraySelection::GetArrayIndex(const char* name)
{
  for(unsigned int i=0; i < this->Internal->ArrayNames.size(); ++i)
  {
    if(this->Internal->ArrayNames[i] == name)
    {
      return i;
    }
  }
  return -1;
}

//----------------------------------------------------------------------------
int vtkDataArraySelection::GetEnabledArrayIndex(const char* name)
{
  int index = 0;
  for(unsigned int i=0; i < this->Internal->ArrayNames.size(); ++i)
  {
    if(this->Internal->ArrayNames[i] == name)
    {
      return index;
    }
    else if(this->Internal->ArraySettings[i])
    {
      ++index;
    }
  }
  return -1;
}

//----------------------------------------------------------------------------
int vtkDataArraySelection::GetArraySetting(int index)
{
  if(index >= 0 && index < this->GetNumberOfArrays())
  {
    return this->Internal->ArraySettings[index];
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::RemoveAllArrays()
{
  vtkDebugMacro("Removing all arrays.");
  if(this->GetNumberOfArrays() > 0)
  {
    this->Internal->ArrayNames.erase(this->Internal->ArrayNames.begin(),
                                     this->Internal->ArrayNames.end());
    this->Internal->ArraySettings.erase(this->Internal->ArraySettings.begin(),
                                        this->Internal->ArraySettings.end());
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
  this->Internal->ArrayNames.push_back(name);
  this->Internal->ArraySettings.push_back(1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::RemoveArrayByIndex(int index)
{
  if(index >= 0 && index < this->GetNumberOfArrays())
  {
    this->Internal->ArrayNames.erase(
      this->Internal->ArrayNames.begin()+index
      );
    this->Internal->ArraySettings.erase(
      this->Internal->ArraySettings.begin()+index
      );
  }
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::RemoveArrayByName(const char *name)
{
  this->RemoveArrayByIndex(this->GetArrayIndex(name));
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
  vtkDataArraySelectionInternals* newInternal =
    new vtkDataArraySelectionInternals;

  newInternal->ArrayNames.reserve(numArrays);
  newInternal->ArraySettings.reserve(numArrays);

  // Fill with settings for all arrays.
  int i;
  for(i=0;i < numArrays; ++i)
  {
    // Add this array.
    newInternal->ArrayNames.push_back(names[i]);

    // Fill in the setting.  Use the old value if available.
    // Otherwise, use the given default.
    int setting = defaultStatus?1:0;
    int index = this->GetArrayIndex(names[i]);
    if(index >= 0)
    {
      setting = this->Internal->ArraySettings[index];
    }
    newInternal->ArraySettings.push_back(setting);
  }

  // Delete the old map and save the new one.
  delete this->Internal;
  this->Internal = newInternal;
}

//----------------------------------------------------------------------------
void vtkDataArraySelection::CopySelections(vtkDataArraySelection* selections)
{
  if(this == selections)
  {
    return;
  }

  int needUpdate = 0;
  int i;
  const char* arrayName;
  if (this->GetNumberOfArrays() != selections->GetNumberOfArrays())
  {
    needUpdate = 1;
  }
  else
  {
    for (i = 0; i < this->GetNumberOfArrays(); i++)
    {
      arrayName = this->GetArrayName(i);
      if (!selections->ArrayExists(arrayName))
      {
        needUpdate = 1;
        break;
      }
      if (selections->ArrayIsEnabled(arrayName) !=
          this->ArrayIsEnabled(arrayName))
      {
        needUpdate = 1;
        break;
      }
    }
  }

  if (!needUpdate)
  {
    return;
  }

  vtkDebugMacro("Copying arrays and settings from " << selections << ".");

  this->RemoveAllArrays();

  this->Internal->ArrayNames.insert(this->Internal->ArrayNames.begin(),
                                     selections->Internal->ArrayNames.begin(),
                                     selections->Internal->ArrayNames.end());
  this->Internal->ArraySettings.insert(
    this->Internal->ArraySettings.begin(),
    selections->Internal->ArraySettings.begin(),
    selections->Internal->ArraySettings.end()
    );
  this->Modified();
}
