/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayVolumeCache.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOSPRayVolumeCache.h"

//------------------------------------------------------------------------------
vtkOSPRayVolumeCache::vtkOSPRayVolumeCache()
{
  this->Size = 0;
};

//------------------------------------------------------------------------------
vtkOSPRayVolumeCache::~vtkOSPRayVolumeCache()
{
  this->Empty();
};

//------------------------------------------------------------------------------
void vtkOSPRayVolumeCache::AddToCache(double tstep, OSPVolume payload)
{
  if (static_cast<int>(this->Contents.size()) >= this->Size)
  {
    return;
  }
  OSPVolume content = this->GetFromCache(tstep);
  ospRelease(content);
  this->Contents[tstep] = payload;
}

//------------------------------------------------------------------------------
OSPVolume vtkOSPRayVolumeCache::GetFromCache(double tstep)
{
  auto ret = this->Contents.find(tstep);
  if (ret != this->Contents.end())
  {
    return ret->second;
  }
  return nullptr;
}

//------------------------------------------------------------------------------
void vtkOSPRayVolumeCache::Empty()
{
  for (auto itr : this->Contents)
  {
    ospRelease(itr.second);
  }
  this->Contents.clear();
  this->Size = 0;
}

//------------------------------------------------------------------------------
int vtkOSPRayVolumeCache::GetSize()
{
  return this->Size;
}

//------------------------------------------------------------------------------
void vtkOSPRayVolumeCache::SetSize(int sz)
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
