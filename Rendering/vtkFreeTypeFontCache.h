/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFreeTypeFontCache.h
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
// .NAME vtkFreeTypeFontCache - 2D Text annotation support (font cache)
// .SECTION Description
// vtkFreeTypeFontCache provides 2D text annotation support for VTK
// using the FreeType and FTGL libraries. This is the font cache, internal
// use only.

#ifndef __vtkFreeTypeFontCache_h
#define __vtkFreeTypeFontCache_h

#include "vtkObject.h"

//----------------------------------------------------------------------------
// Cache by RBGA is nasty, but this is the way to go at the moment for pixmaps.
// This will cache a font for each new text property color, where each color
// component is discretized to 0..255.
// The reason for that is that pixmaps fonts use glDrawPixels() which does not
// honor glColor* settings. GL_*_SCALE and GL_*_BIAS could be used to
// scale and shift the color of each pixels, but this is awfully slow.

#define VTK_FTFC_CACHE_BY_RGBA 1

// Reorder most recently used

#define VTK_FTFC_REORDER 1

// Font cache capacity

#define VTK_FTFC_CACHE_CAPACITY 150

//----------------------------------------------------------------------------
// Singleton cleanup

class VTK_RENDERING_EXPORT vtkFreeTypeFontCacheCleanup
{
public:
  vtkFreeTypeFontCacheCleanup();
  ~vtkFreeTypeFontCacheCleanup();
};

//----------------------------------------------------------------------------
// Singleton font cache

class FTFont;
class vtkTextProperty;

class VTK_RENDERING_EXPORT vtkFreeTypeFontCache
{
public:

  // Cache entry

  struct Entry
  {
    int FontFamily;
    int Bold;
    int Italic;
    int AntiAliasing;
    int FontSize;
#if VTK_FTFC_CACHE_BY_RGBA
    unsigned char Red;
    unsigned char Green;
    unsigned char Blue;
    unsigned char Alpha;
#endif
    FTFont *Font;
    char *FaceFileName;
    float LargestAscender;
    float LargestDescender;
  };

  static vtkFreeTypeFontCache* GetInstance();
  static void SetInstance(vtkFreeTypeFontCache *instance);

  vtkFreeTypeFontCache::Entry* GetFont(vtkTextProperty *tprop, 
                                       int override_color = 0,
                                       unsigned char red = 0,
                                       unsigned char green = 0,
                                       unsigned char blue = 0);

private:

  vtkFreeTypeFontCache();
  ~vtkFreeTypeFontCache();

  static vtkFreeTypeFontCacheCleanup Cleanup;
  static vtkFreeTypeFontCache* Instance;

  void PrintEntry(int i, char *msg = 0);
  void ReleaseEntry(int i);

  void InitializeCache();
  void ReleaseCache();

  Entry *Entries[VTK_FTFC_CACHE_CAPACITY];
  int NumberOfEntries;
};

#endif
