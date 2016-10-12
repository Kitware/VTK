/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationKeyLookup.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkInformationKeyLookup.h"

#include "vtkInformationKey.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkInformationKeyLookup)

//------------------------------------------------------------------------------
void vtkInformationKeyLookup::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Registered Keys:\n";
  indent = indent.GetNextIndent();
  KeyMap &keys = Keys();
  for (KeyMap::iterator i = keys.begin(), iEnd = keys.end(); i != iEnd; ++i)
  {
    os << indent << i->first.first << "::" << i->first.second
       << " @" << i->second << " (" << i->second->GetClassName() << ")\n";
  }
}

//------------------------------------------------------------------------------
vtkInformationKey *vtkInformationKeyLookup::Find(const std::string &name,
                                                 const std::string &location)
{
  KeyMap &keys = Keys();
  KeyMap::iterator it = keys.find(std::make_pair(location, name));
  return it != keys.end() ? it->second : NULL;
}

//------------------------------------------------------------------------------
vtkInformationKeyLookup::vtkInformationKeyLookup()
{
}

//------------------------------------------------------------------------------
vtkInformationKeyLookup::~vtkInformationKeyLookup()
{
  // Keys are owned / cleaned up by the vtk*InformationKeyManagers.
}

//------------------------------------------------------------------------------
void vtkInformationKeyLookup::RegisterKey(vtkInformationKey *key,
                                          const std::string &name,
                                          const std::string &location)
{
  vtkInformationKeyLookup::Keys().insert(
        std::make_pair(std::make_pair(location, name), key));
}

//------------------------------------------------------------------------------
vtkInformationKeyLookup::KeyMap &vtkInformationKeyLookup::Keys()
{
  // Ensure that the map is initialized before using from other static
  // initializations:
  static vtkInformationKeyLookup::KeyMap keys;
  return keys;
}
