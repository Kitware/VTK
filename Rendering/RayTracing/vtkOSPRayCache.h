/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayCache.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOSPRayCache
 * @brief   temporal cache ospray structures to speed flipbooks
 *
 * A temporal cache of templated objects that are created on the first
 * playthrough and reused afterward to speed up animations. Cache is
 * first come first serve. In other words the first 'Size' Set()
 * calls will succeed, later calls will be silently ignored. Decreasing
 * the size of the cache frees all previously held contents.
 *
 * This class is internal.
 */

#ifndef vtkOSPRayCache_h
#define vtkOSPRayCache_h

#include "vtkRenderingRayTracingModule.h" // For export macro
#include "vtkSystemIncludes.h"            //dll warning suppression
#include <map>                            // for stl
#include <memory>

#include "RTWrapper/RTWrapper.h" // for handle types

template <class T>
class VTKRENDERINGRAYTRACING_EXPORT vtkOSPRayCache
{
public:
  vtkOSPRayCache() { this->Size = 0; }

  ~vtkOSPRayCache() { this->Empty(); }

  /**
   * Insert a new object into the cache.
   */
  void Set(double tstep, std::shared_ptr<T> payload)
  {
    if (this->Contents.size() >= this->Size)
    {
      return;
    }
    this->Contents[tstep] = payload;
  }

  /**
   * Obtain an object from the cache.
   * Return nullptr if none present at tstep.
   */
  std::shared_ptr<T> Get(double tstep)
  {
    auto ret = this->Contents.find(tstep);
    if (ret != this->Contents.end())
    {
      return ret->second;
    }
    return nullptr;
  }

  //@{
  /**
   * Set/Get the number of slots available in the cache.
   * Default is 0.
   */
  void SetSize(size_t sz)
  {
    if (sz == this->Size)
    {
      return;
    }
    if (sz < this->Size)
    {
      this->Empty();
    }
    this->Size = sz;
  }
  size_t GetSize() { return this->Size; }
  //@}

  /**
   * Query whether cache contains tstep
   */
  bool Contains(double tstep) { return this->Get(tstep) != nullptr; }

  /**
   * Check if the cache has space left.
   */
  bool HasRoom() { return this->Contents.size() < this->Size; }

private:
  // deletes all of the content in the cache
  void Empty()
  {
    this->Contents.clear();
    this->Size = 0;
  }

  size_t Size;

  std::map<double, std::shared_ptr<T> > Contents;
};

class vtkOSPRayCacheItemObject
{
public:
  vtkOSPRayCacheItemObject(RTW::Backend* be, OSPObject obj)
    : backend(be)
  {
    object = obj;
  }
  ~vtkOSPRayCacheItemObject() { ospRelease(object); }
  OSPObject object{ nullptr };
  size_t size{ 0 };
  RTW::Backend* backend = nullptr;
};

#endif // vtkOSPRayCache_h
// VTK-HeaderTest-Exclude: vtkOSPRayCache.h
