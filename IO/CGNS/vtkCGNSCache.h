/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCGNSCache.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkCGNSCache
 *
 *     store an object in a container with its CGNS path key
 *
 *
 * @par Thanks:
 * Thanks to Mickael Philit
 */

#ifndef vtkCGNSCache_h
#define vtkCGNSCache_h

#include "vtkSmartPointer.h"

#include <iterator>
#include <sstream>
#include <string>
#include <unordered_map>

namespace CGNSRead
{
// No priority and no size limit right now

template <typename CacheDataType>
class vtkCGNSCache
{
public:
  vtkCGNSCache();

  vtkSmartPointer<CacheDataType> Find(const std::string& query);

  void Insert(const std::string& key, const vtkSmartPointer<CacheDataType>& data);

  void ClearCache();

  void SetCacheSizeLimit(int size);
  int GetCacheSizeLimit();

private:
  vtkCGNSCache(const vtkCGNSCache&) = delete;
  void operator=(const vtkCGNSCache&) = delete;

  typedef std::unordered_map<std::string, vtkSmartPointer<CacheDataType>> CacheMapper;
  CacheMapper CacheData;

  typename CacheMapper::iterator LastCacheAccess;

  int cacheSizeLimit;
};

template <typename CacheDataType>
vtkCGNSCache<CacheDataType>::vtkCGNSCache()
  : CacheData()
{
  this->cacheSizeLimit = -1;
  this->LastCacheAccess = CacheData.end();
}

template <typename CacheDataType>
void vtkCGNSCache<CacheDataType>::SetCacheSizeLimit(int size)
{
  this->cacheSizeLimit = size;
}

template <typename CacheDataType>
int vtkCGNSCache<CacheDataType>::GetCacheSizeLimit()
{
  return this->cacheSizeLimit;
}

template <typename CacheDataType>
vtkSmartPointer<CacheDataType> vtkCGNSCache<CacheDataType>::Find(const std::string& query)
{
  typename CacheMapper::iterator iter;
  iter = this->CacheData.find(query);
  if (iter == this->CacheData.end())
    return vtkSmartPointer<CacheDataType>(nullptr);
  this->LastCacheAccess = iter;
  return iter->second;
}

template <typename CacheDataType>
void vtkCGNSCache<CacheDataType>::Insert(
  const std::string& key, const vtkSmartPointer<CacheDataType>& data)
{

  if (this->cacheSizeLimit > 0 &&
    this->CacheData.size() >= static_cast<size_t>(this->cacheSizeLimit))
  {
    // Make some room by removing last accessed/inserted item
    this->CacheData.erase(this->LastCacheAccess);
  }
  this->CacheData[key] = data;
  this->LastCacheAccess = this->CacheData.find(key);
}

template <typename CacheDataType>
void vtkCGNSCache<CacheDataType>::ClearCache()
{
  this->CacheData.clear();
}
}
#endif // vtkCGNSCache_h
// VTK-HeaderTest-Exclude: vtkCGNSCache.h
