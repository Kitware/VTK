/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationInternals.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkInformationInternals
 * @brief   internal structure for vtkInformation
 *
 * vtkInformationInternals is used in internal implementation of
 * vtkInformation. This should only be accessed by friends
 * and sub-classes of that class.
*/

#ifndef vtkInformationInternals_h
#define vtkInformationInternals_h

#include "vtkInformationKey.h"
#include "vtkObjectBase.h"

#define VTK_INFORMATION_USE_HASH_MAP
#ifdef VTK_INFORMATION_USE_HASH_MAP
# include <vtksys/hash_map.hxx>
#else
# include <map>
#endif

//----------------------------------------------------------------------------
class vtkInformationInternals
{
public:
  typedef vtkInformationKey* KeyType;
  typedef vtkObjectBase* DataType;
#ifdef VTK_INFORMATION_USE_HASH_MAP
  struct HashFun
  {
    size_t operator()(KeyType key) const
    {
      return static_cast<size_t>(key - KeyType(0));
    }
  };
  typedef vtksys::hash_map<KeyType, DataType, HashFun> MapType;
#else
  typedef std::map<KeyType, DataType> MapType;
#endif
  MapType Map;

#ifdef VTK_INFORMATION_USE_HASH_MAP
  vtkInformationInternals(): Map(33) {}
#endif

  ~vtkInformationInternals()
  {
    for(MapType::iterator i = this->Map.begin(); i != this->Map.end(); ++i)
    {
      if(vtkObjectBase* value = i->second)
      {
        value->UnRegister(0);
      }
    }
  }
};

#undef VTK_INFORMATION_USE_HASH_MAP

#endif
// VTK-HeaderTest-Exclude: vtkInformationInternals.h
