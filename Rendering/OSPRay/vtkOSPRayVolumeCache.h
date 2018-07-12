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
/**
 * @class   vtkOSPRayVolumeCache
 * @brief   temporal cache ospray structures to speed flipbooks
 *
 * A temporal cache of ospray volumes that are created on the first
 * playthrough and reused afterward to speed up animations. Cache is
 * first come first serve. In other words the first 'Size' AddToCache
 * calls will succeed, later calls will be silently ignored. Decreasing
 * the size of the cache frees all previously held contents.
*/

#ifndef vtkOSPRayVolumeCache_h
#define vtkOSPRayVolumeCache_h

#include "vtkRenderingOSPRayModule.h" // For export macro
#include "vtkSystemIncludes.h" //dll warning suppression
#include <map> // for stl

#include "ospray/ospray.h" // for ospray handle types

class VTKRENDERINGOSPRAY_EXPORT vtkOSPRayVolumeCache {
public:
  vtkOSPRayVolumeCache();
  ~vtkOSPRayVolumeCache();

  /**
   * Insert a new volume into the cache.
   */
  void AddToCache(double tstep, OSPVolume payload);

  /**
   * Obtain a volume from the cache.
   * Return nullptr if none present at tstep.
   */
  OSPVolume GetFromCache(double tstep);

  //@{
  /**
   * Set/Get the number of slots available in the cache.
   * Default is 0.
   */
  void SetSize(int);
  int GetSize();
  //@}

private:

  // deletes all of the content in the cache
  void Empty();
  int Size;

  std::map<double, OSPVolume> Contents;
};

#endif //vtkOSPRayVolumeCache_h
// VTK-HeaderTest-Exclude: vtkOSPRayVolumeCache.h
